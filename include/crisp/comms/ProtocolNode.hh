#ifndef ProtocolNode_hh
#define ProtocolNode_hh 1


#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <crisp/util/SharedQueue.hh>
#include <crisp/comms/common.hh>
#include <crisp/comms/Message.hh>
#include <crisp/comms/MessageDispatcher.hh>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>


namespace crisp
{
  namespace comms
  {
    /** High-level communication manager.
     *
     * ProtocolNode uses one worker thread each for input and output operations, and provides
     * thread-safe out-of-band access to its message queues via <code>send</code> and
     * <code>receive</code>.
     *
     * Synchronization is tracked and managed within this class, and may be checked at any time
     * with the <code>has_sync</code> method.
     */
    template < typename _Socket = PROTOCOL_NODE_DEFAULT_SOCKET_TYPE >
    class ProtocolNode
    {
    public:
      typedef _Socket Socket;
      typedef std::mutex QueueMutex;
      typedef NodeRole Role;

      /** The role of this node.  This value is used to check validity of sent and received
       * messages.
       */
      Role role;


#ifndef SWIG
    protected:
      /** Send the given message, and advance to send_continue. */
      void send_do(Message&& msg);

      /** Send the next queued outgoing message, if available.  If no message is available, wait
       * for one to become available and send it.
       */
      void send_next(const boost::system::error_code& error_code,
		     size_t bytes_sent);

      /** Start the receive loop. */
      void receive_init();

      /** Ask the io_service to add an asynchronous read to its work queue.  The amount of data we
       *  request be read is set up by receive_init or receive_handler.
       */
      void receive_do();

      /** Handle the result of a read initiated by `receive_do`.
       */
      void receive_handler(const boost::system::error_code& error_code,
			   size_t bytes_received);


      /** Start the sync-send loop in its own thread.
       */
      void start_sync_send_loop();
      /** Cancel the sync-send timer and join the sync-send thread.
       */
      void stop_sync_send_loop();
      void send_sync(const boost::system::error_code& error);

      void
      receive_resync();


      void dispatch_received_loop();

      /** State variables container for an input or output thread. */
      struct IOState
      {
	template < typename _BaseHandler >
	inline
	IOState(ProtocolNode& _node, const _BaseHandler&& bh)
	  : node ( _node ),
	    thread ( ),
	    strand ( node.m_io_service ),
	    queue ( ),
	    handler ( strand.wrap(std::bind(bh, &node, std::placeholders::_1,
					    std::placeholders::_2)) )
	{}

	IOState(IOState&& state);
	IOState(const IOState&) = delete;
	~IOState();
	void run(std::function<void()> handler);

	ProtocolNode& node;		         /**< Node that owns this state object. */
	std::thread thread;		         /**< Thread object. */
	boost::asio::io_service::strand strand;  /**< I/O strand for the current thread. */
	size_t remaining_bytes;		         /**< Remaining bytes to be sent/received. */
	crisp::util::SharedQueue<Message> queue; /**< Message queue. */

	std::mutex mutex;
	std::condition_variable cv;

	/** Asynchronous I/O operation-complete handler. */
	std::function<void(const boost::system::error_code& error_code,
			   std::size_t bytes_sent)> handler;

      };

      friend struct IOState;

      /** Enum used to keep track of what step of the receive process we're on. */
      ENUM_CLASS(ReceiveStep, uint8_t,
		 READ_HEADER,
		 READ_TAIL_AND_ENQUEUE,
		 RESYNC
		 );

      /** I/O coordination and concurrency-management service object. */
      boost::asio::io_service& m_io_service;

      /** Socket abstraction. */
      _Socket m_socket;

      IOState
        m_send,			/**< send (output) state variables  */
	m_receive;		/**< receive (input) state variables */

      Message m_send_message;	/**< Message currently being sent. */
      boost::asio::streambuf
        m_send_buffer;		/**< Buffer containing pending send data. */

      MemoryEncodeBuffer m_receive_buffer; /**< Receive data buffer. */
      ReceiveStep m_receive_step;	/**< Most recently completed receive step. */

      std::string m_read_until_string; /**< Delimiter passed to calls to
					  boost::asio::async_read_until. */
      boost::asio::streambuf
      m_read_until_buffer;	/**< Buffer object passed to calls to
				   boost::asio::async_read_until.  Currently only used by
				   receive_resync(), and the only reason it's here is it must be
				   persistent.
				*/


      /**@begin sync Synchronization loop variables
       *@{
       */
      typedef boost::asio::deadline_timer SyncTimer;
      std::thread m_sync_thread;
      boost::asio::io_service::strand m_sync_strand;
      std::function<void(const boost::system::error_code&)> m_sync_handler;
      SyncTimer m_sync_timer;	/**< Timer used to trigger an outgoing sync message every few
				   seconds.  */
      /**@}*/

      bool m_synced;		/**< Indicates that no message-sync errors have occured
				   recently. */
      bool m_stopping;		/**< Flag checked by the client threads to see if they should
				   continue. */
      std::thread m_dispatch_threads[2]; /**< Callbacks threads. */
      boost::asio::io_service::strand m_dispatch_sent_strand;

      std::mutex m_wait_mutex;
      std::condition_variable m_wait_cv;
#endif
    public:
      Configuration configuration; /**< Node's interface configuration.  If the
                                    node is a master, this is the interface
                                    available on the remote node; otherwise this
                                    is the local configuration.  */

      /** Constructor.
       *
       * @param socket An open socket to be used for communication. 
       *
       * @param role Role -- master or slave -- to assume.
       *
       * @param device Device file to open.
       */
      ProtocolNode(Socket&& socket, Role role);

      ~ProtocolNode();


      /** Launch the I/O worker threads.
       *
       * @return Whether the call had any effect.
       */
      bool launch();

      /** Block execution until another thread halts the node.
       *
       */
      void wait();

      /** Cancel all I/O operations in progress and stop the I/O worker threads. */
      void halt();

      /** Check if the underlying socket is "open".
       *
       * @return @c true if the node has an open socket, @c false otherwise.
       */
      inline bool
      is_connected() const noexcept
      { return m_socket.is_open() > 0; }


      /** Check if the node is currently in-sync with its connected node;
       *
       * @note This method has no meaning at any time when `is_connected()` returns `false`.
       */
      inline bool
      has_sync() const noexcept
      { return is_connected() && m_synced; }


      /** Queue a message to be sent to the connected node.
       *
       * @param m Message to send.
       */
      void
      send(Message&& m);

      template < typename... Args >
      void send(Args... args)
      { m_send.queue.emplace(args...); }
      

      MessageDispatcher<ProtocolNode> dispatcher;

      bool
      operator <(const ProtocolNode& other) const;
    };
  }
}

#include <crisp/comms/bits/ProtocolNode.tcc>


#endif	/* ProtocolNode_hh */
