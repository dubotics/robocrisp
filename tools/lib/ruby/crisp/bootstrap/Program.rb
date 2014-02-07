require 'crisp/bootstrap/ProcessInfo'

module CRISP
  module Bootstrap
    # A program/program-mode combination.
    #
    # Currently unused.
    class ProgramModePair
      @program = nil
      @mode = nil

      # @!attribute [r]
      #   Program.
      #   @return [String]
      attr_reader :program

      # @!attribute [r]
      #   Mode within `program`.
      #   @return [String]
      attr_reader :mode

      def initialize(program, mode)
        program =
          if program.kind_of?(Program)
            program.name
          else
            program
          end
        mode =
          if mode.kind_of?(ProgramMode)
            mode.name
          else
            mode
          end

        @program = program
        @mode = mode
      end
    end

    # A "mode" in which a program can be run.  Mode specifications are used to
    # enable automatic matching and selection of program parameters (runtime
    # arguments) between e.g. a client and a server process.
    class ProgramMode
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

      def initialize(name, parameters, parameter_match)
        @name = name
        @parameters = parameters
        # @works_with = ww
        @parameter_match = parameter_match
      end

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
    end

    # A program launchable by the CRISP bootstrapper.
    class Program
      @name = nil
      @uuid = nil
      @binary = nil
      @modes = nil
      @parameter_match = nil

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

      # Initialize a new Program instance.
      # @param name Program's symbolic name.
      def initialize(name, uuid, binary, modes)
        @name = name
        @uuid = uuid
        @binary = binary
        @modes = modes

        # Replace the `parameter_match` property on modes that have it with a
        # reference to the actual mode that it names.
        @modes.each_value do |m|
          if not m.parameter_match.nil? and m.parameter_match.kind_of?(String)
            m.instance_variable_set(:@parameter_match, @modes[m.parameter_match.intern])
          end
        end
      end

      # Launch the program.
      #
      # @param [Symbol,ProgramMode] mode Mode in which to launch the program.
      #
      # @param [ProcessInfo,nil] matched_launch_data Result of launching a
      #   program/mode combination for the mode's `parameter_match` mode, if
      #   applicable.
      #
      # @return [ProcessInfo] A `ProcessInfo` instance corresponding to the
      #     newly-launched process.
      def launch(mode, launch_data = nil, *args)
        mode = mode.intern if mode.kind_of?(String)
        raise NotImplementedError.new("#{mode}: no such mode") if
          mode.kind_of?(Symbol) and not @modes.has_key?(mode)
        mode = @modes[mode] if mode.kind_of?(Symbol)

        if not launch_data.nil? and not launch_data.kind_of?(Hash)
          args.unshift(launch_data)
          launch_data = nil
        end
        
        # Determine which parameter values should be copied from the supplied
        # launch-data hash.
        match_param_names = mode.unmatched_parameters.collect { |p| p.name }
        matched_values = {}

        # Copy the values for matched parameters into their own hash.
        launch_data.parameters.each_pair { |k, v| matched_values[k] = v if match_param_names.include?(k) } if
          not launch_data.nil?

        $stderr.puts("\033[1;33mwarning:\033[0m No matched-mode launch data supplied; matched-mode parameters will be\n" +
                     "         selected from explicitly-given arguments") if
          launch_data.nil? and not match_param_names.empty?

        parameter_values = []

        break_index = nil
        break_reason = nil

        # Build the actual parameter-value list to provide to the process.
        mode.parameters.each_index { |pi|
          param = mode.parameters[pi]
          if matched_values.has_key?(param.name)
            parameter_values << matched_values[param.name]
          elsif args.empty?
            candidates = param.candidates
            if not candidates.nil? and num_candidates(candidates) == 1
              parameter_values << candidates[0]
            else
              break_index = pi
              break_reason = candidates.nil? ? :nocandidates : :toomanycandidates
              break
            end
          else
            parameter_values << args.shift
          end
          parameter_values[-1] = parameter_values[-1].to_s if not parameter_values[-1].kind_of?(String)
        }

        if break_index.nil?
          # Success!  Run the program.
          pid = Process.spawn(*(parameter_values.dup.unshift(@binary)))

          # Build the process information object.  If another mode depends on
          # the parameters used for this process, it will be able to retrieve
          # them from this object.
          parameters_hash = {}
          parameter_values.each_index { |pi|
            parameters_hash[mode.parameters[pi].name] = parameter_values[pi]
          }
          ProcessInfo.new(self, mode, parameter_values, parameters_hash, pid)
        else
          # Inform the user what went wrong.
          case break_reason
          when :nocandidates
            $stderr.puts("\033[1;31merror:\033[0m insufficient arguments supplied for mode \"#{mode.name}\"")
          when :toomanycandidates
            $stderr.puts("\033[1;31merror:\033[0m failed to automatically select unspecified arguments for mode \"#{mode.name}\"")
          end
          $stderr.puts("Remaining arguments are:")
          mode.parameters[break_index..-1].each { |p|
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
  end
end
