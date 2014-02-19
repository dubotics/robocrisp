require 'crisp/bootstrap/ProgramInvocationInfo'

module CRISP
  module Bootstrap
    # A "mode" in which a specific program can be run.  Mode specifications are
    # used to enable automatic matching and selection of program parameters
    # (runtime arguments) between e.g. a client and a server process.
    class ProgramMode
      @program = nil
      @name = nil
      @parameters = nil
      @works_with = nil
      @parameter_match = nil


      # @!attribute [r]
      #   Name of the mode.
      #   @return [String]
      attr_reader :name

      # @!attribute [r]
      #   List of parameters passed to the program that will cause it to operate
      #   in this mode.
      #   @return [Array<Parameter>]
      attr_reader :parameters

      # @!attribute [r]
      #   List of program/mode pairs that this program-mode supports.
      #   @return [Array<ProgramModePair>]
      # attr_reader :works_with


      # @!attribute [r]
      #   Mode from which to source parameter values for matching parameter
      #   names.  This is used to determine both those parameter values, and the
      #   actions required (e.g. launching other processes) to support running
      #   the program in this mode.
      #   @param
      attr_reader :parameter_match


      # @!attribute [r]
      #   Program that owns this mode.
      #   @return [Program]
      attr_reader :program


      # @!attribute [r]
      #   List of parameters that must be explicitly given.
      #   @return [Array<Parameter>]
      def unmatched_parameters()
        if @parameter_match.nil?
          @parameters
        else
          @parameters.select { |p| not @parameter_match.parameters.include?(p) }
        end
      end


      def initialize(program, name, parameters, parameter_match)
        @program = program
        @name = name
        @parameters = parameters
        # @works_with = ww
        @parameter_match = parameter_match
      end

      def inspect()
        "#<#{self.class}:#{'%#x' % self.object_id.abs} %s %s@parameters=%s>" %
          [name.inspect,
           if not parameter_match.nil?
             '(matches:%s) ' % parameter_match.name.inspect
           else
             ''
           end,
           parameters.inspect]
      end


      # Launch the program in this mode.
      #
      # @param [ProgramInvocationInfo,nil] launch_data Result of launching a
      #   program/mode combination for the mode's `parameter_match` mode, if
      #   applicable.
      #
      # @return [ProgramInvocationInfo] A `ProgramInvocationInfo` instance
      #     corresponding to the newly-launched process.
      def prepare(launch_data = nil, *args)
        if not launch_data.nil? and not launch_data.kind_of?(ProgramInvocationInfo)
          args.unshift(launch_data)
          launch_data = nil
        end
        
        # Determine which parameter values should be copied from the supplied
        # launch-data hash.
        match_param_names = unmatched_parameters.collect { |p| p.name }
        matched_values = {}

        # Copy the values for matched parameters into their own hash.
        launch_data.parameters.each_pair { |k, v| matched_values[k] = v if match_param_names.include?(k) } if
          not launch_data.nil?

        $stderr.puts("\033[1;33mwarning:\033[0m No matched-mode launch data supplied; matched-mode parameters will be\n" +
                     "         selected from explicitly-given arguments") if
          launch_data.nil? and not parameter_match.nil?

        parameter_values = []

        break_index = nil
        break_reason = nil

        # Build the actual parameter-value list to provide to the process.
        @parameters.each_index { |pi|
          param = @parameters[pi]
          if matched_values.has_key?(param.name) and param.subtype == 'matched'
            parameter_values << matched_values[param.name]
          else
            candidates = param.candidates
            if not param.default.nil?
              parameter_values << param.default
            elsif not candidates.nil? and num_candidates(candidates) == 1
              parameter_values << candidates[0]
            elsif not args.empty?
              parameter_values << args.shift
            else

              break_index = pi
              break_reason = candidates.nil? ? :nocandidates : :toomanycandidates
              break
            end
          end
          parameter_values[-1] = parameter_values[-1].to_s if not parameter_values[-1].kind_of?(String)
        }

        if break_index.nil?
          # Success! Build and return the invocation-info object.  If another
          # mode depends on the parameters used for this program/mode
          # invocation, it will be able to retrieve them from this.
          parameters_hash = {}
          parameter_values.each_index { |pi|
            parameters_hash[@parameters[pi].name] = parameter_values[pi]
          }
          ProgramInvocationInfo.new(@program, self, parameter_values, parameters_hash)
        else
          # Inform the user what went wrong.
          case break_reason
          when :nocandidates
            $stderr.puts("\033[1;31merror:\033[0m insufficient arguments supplied for mode \"#{@name}\"")
          when :toomanycandidates
            $stderr.puts("\033[1;31merror:\033[0m failed to automatically select unspecified arguments for mode \"#{@name}\"")
          end
          $stderr.puts("Remaining arguments are:")
          @parameters[break_index..-1].each { |p|
            candidates = p.candidates
            $stderr.puts("    %s (candidate%s: %s)" %
                         [ p.name, num_candidates(candidates) == 1 ? '' : 's',
                           candidates.nil? ? 'none' : format_candidate_values(candidates) ])
          }
          raise 'Argument selection failed.'
        end
      end

      def format_candidate_values(cnds)
        case cnds
        when Array
          cnds.join(', ')
        else
          cnds.to_s
        end
      end

      def num_candidates(cnds)
        case cnds
        when Array
          cnds.size
        when Range
          cnds.count
        end
      end
    end

    # A program launchable by the CRISP bootstrapper.
    class Program
      @name = nil
      @uuid = nil
      @binary = nil
      @modes = nil
      @default_mode = nil

      # @!attribute [r]
      #   Program's symbolic name
      #   @return [String]
      attr_reader :name

      # @!attribute [r]
      #   UUID for the program definition.
      #   @return [String]
      attr_reader :uuid

      # @!attribute [r]
      #   Path to the executable file
      #   @return [String]
      attr_reader :binary

      # @!attribute [r]
      #   Modes the program may be run in.
      #   @return [Array<ProgramMode>]
      attr_reader :modes

      # @!attribute [r]
      #   Reference to the program's default mode, if any.
      #  @return [ProgramMode]
      attr_reader :default_mode


      @@all_instances = {}

      def self.from_uuid(uuid)
        ObjectSpace._id2ref(@@all_instances[uuid])
      end

      # Initialize a new Program instance.
      # @param name Program's symbolic name.
      def initialize(name, uuid, binary, modes, default_mode)
        @@all_instances[uuid] = self.object_id

        @name = name
        @uuid = uuid
        @binary = binary
        @modes = modes
        @default_mode = default_mode

        # Replace the `parameter_match` property on modes that have it with a
        # reference to the actual mode that it names.
        # 
        # Make sure each mode's `program` instance variable is set to this
        # program.
        @modes.each_value do |m|
          if not m.parameter_match.nil? and m.parameter_match.kind_of?(String)
            m.instance_variable_set(:@parameter_match, @modes[m.parameter_match])
          end
          if m.program.nil? or m.program != self
            m.instance_variable_set(:@program, self)            
          end
        end
      end
    end
  end
end
