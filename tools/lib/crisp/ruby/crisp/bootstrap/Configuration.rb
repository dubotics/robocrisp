autoload(:YAML, 'yaml')
require 'crisp/bootstrap/Program'
require 'crisp/bootstrap/Parameter'

module CRISP
  module Bootstrap
    # Configuration holder and loader for the bootstrap server and programs.
    class Configuration
      @root = nil
      @programs = nil
      @transports = nil

      # @!attribute [r]
      #   Root for all relative paths specified within the configuration.
      #   @return String
      attr_reader :root

      # @!attribute [r]
      #   Hash containing declared programs keyed by name.
      #   @return [Hash<String,Program>]
      attr_reader :programs

      # @!attribute [r]
      #   List of enabled bootstrap transports.
      #   @return [Array<String>]
      attr_reader :transports

      # Initialize a new Configuration instance.
      #
      # @overload initialize()
      #   Initialize an empty Configuration instance. 
      #
      # @overload initialize(root, programs)
      #   Initialize a new Configuration instance by specifying the values directly.
      #
      #   @param [String] root Root directory for any relative program paths
      #     within the configuration.
      #   @param [Array<Program>] programs List of programs available within
      #     this configuration.
      #
      #
      # @overload initialize(path)
      #   Initialize a new Configuration instance by specifying the path to a YAML
      #   configuration file.
      #
      #   @param [String] path Path to the configuration file to load.
      def initialize(root = nil, programs = nil)
        if not programs.nil?
          @root = root
          @programs = programs
        elsif not root.nil?
          load(root)
        end
      end

      # Load a configuration file, merging or overwriting the currently-stored
      # values with the newly-loaded ones as appropriate.
      #
      # @param [String] path Path to the configuration file to load.
      #
      # @param [Symbol] mode How to interpret the loaded YAML data file.
      #
      #    * `:base` will treat it as the root of a configuration tree.
      #
      #    * `:program` will treat it as a hash containing a program definition.
      #
      # @param [Array] *args Additional arguments for non-`:base` load modes.
      def load(path, mode = :base, *args)
        buf = IO.read(path)

        lines = buf.split("\n")

        # Here, we prefix all keys with `!ruby/symbol' so we don't have to
        # "quote" both sides of hash-key indices.
        lines.each_index do |i|
          lines[i] = lines[i].gsub(/^([\t ]*)(-[ \t]+)?([-_a-zA-Z0-9]+)([ \t:])/) do |m|
            m = Regexp.last_match
            '%s%s!ruby/symbol %s%s' % [ m[1], m[2], m[3], m[4]]
          end
        end
        buf = lines.join("\n")
        conf = YAML.load(buf)
        @programs = {} if not instance_variable_defined?(:@programs)

        case mode
        when :program
          name = args[0] || File.basename(path).sub(/\.ya?ml$/, '')
          @programs[name] = Configuration.build_program(name, conf, File.dirname(path))

        when :base
          @root = File.dirname(path)

          raise "No programs specified in bootstrap configuration file \`#{root}'" if
            not conf[:programs] or not conf[:programs].size

          # Build the program instances
          conf[:programs].each_pair { |pname, obj|
            if obj.kind_of?(String)
              program_conf_path =
                if File.exist?(obj)
                  obj
                else
                  File.expand_path(obj, @root)
                end
              load(program_conf_path, :program, pname.to_s)
            else
              pname = pname.to_s
              @programs[pname] = Configuration.build_program(pname, obj, @root)
            end
          }
        end
      end
      
      # Build a Parameter object from a hash loaded from a YAML configuration
      # file.
      def self.build_parameter(name, hsh)
        Parameter.new(hsh[:name] || name, [ hsh[:type], hsh[:subtype] ].compact,
                      hsh[:default] || nil,
                      hsh[:optional] || false)
      end

      def self.build_parameter_array(ary)
        ary.collect do |phsh|
          name, values_hash =
            if phsh.size == 1
              raise 'Parameter-as-hash-instance has invalid type' unless
                phsh.values[0].kind_of?(Hash)
              [ phsh.keys[0].to_s, phsh.values[0] ]
            else
              [ phsh[:name], phsh ]
            end
          self.build_parameter(name, values_hash)
        end
      end

      # Build a ProgramMode object from a hash loaded from a YAML configuration
      # file.
      def self.build_mode(name, hsh, prepend_parameters = [])
        parameters =
          if not hsh[:parameters].nil?
            prepend_parameters.
              dup.
              concat(self.build_parameter_array(hsh[:parameters]))
          else
            prepend_parameters
          end
        ProgramMode.new(nil, name.to_s, parameters, hsh[:'parameter-match'])
      end

      # Build a Program object from a hash loaded from a YAML configuration
      # file.
      def self.build_program(name, hsh, root_path)
        common_parameters =
          if hsh.has_key?(:'common-parameters') and hsh[:'common-parameters'].kind_of?(Hash)
            self.build_parameter_array(hsh[:'common-parameters'])
          else
            []
          end

        modes = {}
        hsh[:modes].each_pair do |mname, mhsh|
          mname = mname.to_s
          modes[mname] = self.build_mode(mname, mhsh, common_parameters)
        end

        default_mode = nil
        if hsh.has_key?(:'default-mode')
          default_mode = modes[hsh[:'default-mode']]
        elsif modes.size == 1
          default_mode = modes.values[0]
        end

        Program.new(name, hsh[:uuid], File.expand_path(hsh[:binary], root_path),
                    modes, default_mode)
      end
    end                         # class Configuration
  end
end
