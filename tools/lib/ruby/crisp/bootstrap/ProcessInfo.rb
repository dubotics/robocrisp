require 'thread'

module CRISP
  module Bootstrap
    class ProcessInfo
      @program = nil
      @mode = nil
      @arguments = nil
      @parameters = nil
      @pid = nil
      @wait_callbacks = nil
      @wait_callbacks_mutex = nil
      @status = nil

      # @!attribute [r]
      #   A reference to the Bootstrap program-definition used to launch the
      #   process.
      #   @return [CRISP::Bootstrap::Program]
      attr_reader :program

      # @!attribute [r]
      #   Reference to the ProgramMode used to launch the process.
      #   @return [CRISP::Bootstrap::ProgramMode]
      attr_reader :mode

      # @!attribute [r]
      #   Command array passed to `Process.spawn` to create the process.
      #   @return [Array<String>]
      attr_reader :arguments

      # @!attribute [r]
      #   Named-parameter hash.  This can be used for e.g. launching a matching
      #   program.
      #   @return [Hash<String,String>]
      attr_reader :parameters

      # @!attribute [r]
      #   Process ID of the launched process.
      #   @return [Fixnum]
      attr_reader :pid

      def initialize(program, mode, arguments, parameters, pid)
        @program = program
        @mode = mode
        @arguments = arguments
        @parameters = parameters
        @pid = pid
        @wait_callbacks = []
        @wait_callbacks_mutex = Mutex.new
        @status = nil

        ObjectSpace.define_finalizer(self, proc { |id| begin; Process.detach(pid); rescue; end })

        # Wait for the process to finish in a separate thread.
        Thread.new do
          Process.wait(@pid)
          @status = $?;
          @wait_callbacks_mutex.synchronize {
            while not wait_callbacks.empty?
              @wait_callbacks.pop.call(@status)
            end
          }
        end
      end

      # Check if the process is currently running.
      def running?
        begin
          Process.getpgid(@pid)
          true
        rescue Errno::ESRCH
          false
        end
      end

      # Send a signal to the process.
      def kill(sig = :SIGINT)
        Process.kill(sig, @pid)
      end

      # Wait for the process to finish
      def wait(&async_cb)
        if block_given?
          if not @status.nil?
            async_cb.call(@status)
          else
            @wait_callbacks_mutex.synchronize { @wait_callbacks.push(async_cb) }
          end
          nil
        elsif not @status.nil?
          @status
        else
          Process.wait(@pid)
          @status = $?
        end
      end
    end
  end
end
