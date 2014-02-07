autoload(:Socket, 'socket')

module CRISP
  module Bootstrap
    # A parameter specification for a program in a CRISP bootstrap
    # configuration.
    class Parameter
      # Hash of candidate-value selection functions.
      @@selectors = Hash.new({})

      # Add a selector for 
      def self.add_selector(type, subtype = nil, &block)
        @@selectors[type] = {} if not @@selectors.has_key?(type)
        @@selectors[type][subtype] = block
      end

      @name = nil
      @type = nil
      @subtype = nil
      @optional = nil

      # @!attribute [r]
      #   Whether the parameter is optional.  If `false`, the parameter _must_
      #   be passed to the program to enable the mode for which it was
      #   specified.
      #   @return [Boolean]
      def optional?; @optional; end

      # @!attribute [r]
      #   Human-friendly name for the parameter.
      #   @return [String]
      attr_reader :name

      # @!attribute [r]
      #   Parameter type.  Used to determine selection method, etc.
      #   @return [Symbol]
      attr_reader :type

      # @!attribute [r]
      #   Parameter subtype.  If applicable, this will contain a value used to
      #   further specify how to select the parameter.
      #   @return [Symbol,nil]
      attr_reader :subtype

      # Initialize a new instance of Parameter.
      # @param [String] name Human-friendly name for the parameter.
      # @param [Array<Symbol>] types Parameter type (and optionally subtype).
      # @param [Boolean] optional Whether the parameter is optional.
      def initialize(name, types, optional = false)
        @name = name
        @type = types.shift
        @subtype = types.shift
        @optional = optional
      end

      # Get candidate values for this parameter.
      # @return 
      def candidates()
        if @@selectors.has_key?(@type)
          if @@selectors[@type].has_key?(@subtype)
            @@selectors[@type][@subtype].call()
          else
            @@selectors[@type][nil].call()
          end
        else
          nil
        end
      end

      def inspect()
        "#<#{self.class}:#{'%#x' % self.object_id.abs} %s>" %
          case type
          when 'literal'; 'literal:%s' % name
          else; "#{name.inspect} (#{[type,subtype].compact.join(':')})"
          end
      end
    end

    # Determine if a given `Addrinfo` instance specifies a "useful" IP address
    # for a server socket to bind to.
    # @param [Addrinfo] ai Address-info object.
    def self.is_server_ip(ai)
      not ai.ipv6_linklocal? and
        not ai.ipv6_loopback? and
        not ai.ipv4_loopback?
    end

    Parameter.add_selector('ip-address', 'server') do
      Socket.ip_address_list.select { |ai| is_server_ip(ai) }.
        push(Addrinfo.ip('0.0.0.0')).
        collect { |ai| ai.inspect_sockaddr }
    end

    Parameter.add_selector('ip-port') do
      Range.new(1000, 65535)
    end

    Parameter.add_selector('device-file', 'input-evdev') do
      base_dir = '/dev/input/by-id'
      Dir.entries(base_dir).
        select { |e| e =~ /-event-[a-z]+$/ }.
        collect { |e| File.join(base_dir, e) }
    end

  end                           # module Bootstrap
end
