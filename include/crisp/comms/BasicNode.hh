/** @file
 *
 * Declares template class BasicNode, which provides a Message-passing utility for IP protocols
 * based on Boost.Asio.
 */
#ifndef crisp_comms_BasicNode_hh
#define crisp_comms_BasicNode_hh 1


#include <crisp/comms/Configuration.hh>
#include <crisp/comms/Handshake.hh>
#include <crisp/comms/Message.hh>
#include <crisp/comms/MessageDispatcher.hh>
#include <crisp/comms/common.hh>

#include <crisp/util/Scheduler.hh>
#include <crisp/util/WorkerObject.hh>
#include <crisp/util/SharedQueue.hh>
#include <crisp/util/Buffer.hh>

#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/spawn.hpp>

#include <thread>

namespace crisp
{
  namespace comms
  {

    /** Basic Message node for use with Boost.Asio and IP-based protocols.
     */
    template < typename _Protocol >
    class BasicNode : protected crisp::util::WorkerObject
    {
    public:
      typedef _Protocol Protocol;
      typedef typename Protocol::socket Socket;

    private:
      friend class MessageDispatcher<BasicNode>;
      /** Connected socket used for communication. */
      Socket m_socket;

      /** Handle to the scheduled period-action used for synchronization.  */
      crisp::util::PeriodicAction* m_sync_action;

      /** Handle to the scheduled "halt" action, used to disconnect the node if
       *  a handshake isn't completed within five seconds of the node being
       *  launched.
       */
      crisp::util::ScheduledAction* m_halt_action;

      crisp::util::SharedQueue<Message>
        m_outgoing_queue,         /**< Outgoing message queue. */
        m_incoming_queue;         /**< Incoming message queue. */

      /** Whether the node's `halt` method has been called. */
      std::atomic_flag m_halting;

      /** Referent for `stopped`.  This is used to provide a way for users of BasicNode to
          determine whether or not the node has been shut down. */
      std::atomic<bool> m_stopped;

      std::thread m_dispatch_thread;

      std::mutex m_halt_mutex;
      std::condition_variable m_halt_cv;

    public:
      /** Public "node-is-halting-or-stopped" flag. */
      const std::atomic<bool>& stopped;

      /** A scheduler for managing periodic communication (and any user-set actions) on the node.
          Within BasicNode, this is used for producing synchronization messages.  */
      crisp::util::Scheduler scheduler;

      /** Node role specified at object construction. */
      const NodeRole role;

      /** Configuration object.
       *
       *   * When `role` is `NodeRole::MASTER`, this will contain the interface configuration
       *     declared by the connected slave node, once it has been received.
       *
       *   * When `role` is `NodeRole::SLAVE`, this contains the local interface
       *     configuration.
       */
      Configuration configuration;

      /** Message dispatcher and user-set callbacks container.  Provides  */
      MessageDispatcher<BasicNode> dispatcher;


      /** Initialize a node using the given socket and role.
       *
       * @param _socket Rvalue reference to the socket to be used.  The socket *must* be
       *     connected to the remote node.
       *
       * @param _role Role to request when performing the initial handshake with the remote
       *     node.
       */
      BasicNode(Socket&& _socket, NodeRole _role)
        : WorkerObject ( _socket.get_io_service(), 5 ),
          m_socket ( std::move(_socket) ),
          m_sync_action ( nullptr ),
          m_halt_action ( nullptr ),
          m_outgoing_queue ( ),
          m_incoming_queue ( ),
          m_halting ( ),
          m_stopped ( false ),
          m_dispatch_thread ( ),
          m_halt_mutex ( ),
          m_halt_cv ( ),
          stopped ( m_stopped ),
          scheduler ( m_io_service ),
          role ( _role ),
          configuration ( ),
          dispatcher ( *this )
      {
        m_halting.clear();
      }

      ~BasicNode()
      {
        halt(true);
      }


      void run()
      {
        launch();
        std::unique_lock<std::mutex> lock ( m_halt_mutex );
        m_halt_cv.wait(lock);

        if ( ! stopped )
          halt();
      }

      /** Launch worker threads to handle the node's IO. */
      bool
      launch()
      {
        if ( m_dispatch_thread.joinable() )
          return false;

        WorkerObject::launch();

        m_dispatch_thread = std::thread(std::bind(&BasicNode::dispatch_received_loop, this));

        boost::asio::spawn(m_io_service, std::bind(&BasicNode::receive_loop, this,
                                                   std::placeholders::_1));
        boost::asio::spawn(m_io_service, std::bind(&BasicNode::send_loop, this,
                                                   std::placeholders::_1));

        send(Handshake { PROTOCOL_VERSION, role });

        /* The default `handshake_response.received` handler will cancel this
           action on successful handshake sequence. */
        m_halt_action =
          & scheduler.set_timer(std::chrono::seconds(5),
                                [this](crisp::util::ScheduledAction&)
                                { fprintf(stderr, "[0x%x] Handshake not completed before timer expired.  Halting.\n",
                                          THREAD_ID);
                                  halt(); });

        return true;
      }


      /** Check if the node can be halted from within the current thread.  */
      bool
      can_halt() const
      {
        return WorkerObject::can_halt() && std::this_thread::get_id() != m_dispatch_thread.get_id();
      }


      /** Halt the node by closing the socket and waiting for its worker threads to finish.
       *
       * @param force_try Force an attempt to halt even if it looks like the function was called
       *   from the wrong thread.
       *
       * @return `true` if the node was able to halt, and `false` if the function was called
       *     from the wrong thread or the node was already halted.
       */
      bool
      halt(bool force_try = false)
      {
        if ( ! m_halting.test_and_set() )
          {
            m_stopped = true;

            if ( ! can_halt() && ! force_try )
              {               /* Can't halt from this thread. */
                m_stopped = false;
                m_halting.clear();

                std::unique_lock<std::mutex> lock ( m_halt_mutex );
                m_halt_cv.notify_all();

                return false;
              }

            fprintf(stderr, "[0x%x] Halting... ", THREAD_ID);

            if ( m_sync_action )
              m_sync_action->cancel();

            if ( m_halt_action )
              m_halt_action->cancel();

            m_sync_action = nullptr;
            m_halt_action = nullptr;

            m_socket.close();
            m_outgoing_queue.wake_all();
            m_incoming_queue.wake_all();

            m_io_service.poll();

            WorkerObject::halt();
            m_dispatch_thread.join();

            fprintf(stderr, "done.\n");
            m_halt_cv.notify_all();
            return true;
          }
        else
          return false;
      }

      /** Enqueue an outgoing message.  The message will be sent once all previously-queued outgoing
       * messages have been sent.
       *
       * @param m Message to send.
       */
      void send(const Message& m)
      {
        m_outgoing_queue.push(m);
      }

      /** Enqueue an outgoing message.  The message will be sent once all previously-queued outgoing
       * messages have been sent.
       *
       * @param m Message to send.
       */
      void send(Message&& m)
      {
        m_outgoing_queue.emplace(std::move(m));
      }


      /** Dispatches handlers for incoming messages.
       */
      void
      dispatch_received_loop()
      {
        fprintf(stderr, "[0x%x] Entered dispatch loop.\n", THREAD_ID);
        while ( ! stopped )
          {
            bool aborted;
            Message message ( m_incoming_queue.next(&aborted) );

            if ( ! aborted )
              dispatcher.dispatch(message, MessageDirection::INCOMING);
          }
        fprintf(stderr, "[0x%x] Exiting dispatch loop.\n", THREAD_ID);
      }

      /** Send outgoing messages until the socket is closed.
       *
       * @param yield A Boost.Asio `yield_context`, used to enable resuming of this method from
       *     asynchronous IO-completion handlers.
       */
      void send_loop(boost::asio::yield_context yield)
      {
        bool aborted;

        fprintf(stderr, "[0x%x] Entered send loop.\n", THREAD_ID);
        while ( ! m_stopped )
          {
            /* Fetch the next message to send, waiting if necessary. */
            Message message ( m_outgoing_queue.next(&aborted) );

            if ( aborted || m_stopped )
              break;
            else
              m_halting.clear();

            /* Encode it. */
            MemoryEncodeBuffer meb ( message.get_encoded_size() );
            EncodeResult er;
            if ( (er = message.encode(meb)) != EncodeResult::SUCCESS )
              { fprintf(stderr, "error encoding message: %s.\n", encode_result_to_string(er));
                continue; }

            /* Dump the message contents. */
            /* for ( size_t i = 0; i < meb.offset; ++i )
             *   fprintf(stderr, " %02hhX", meb.data[i]);
             * fputs("\n", stderr); */

            boost::system::error_code ec;
            boost::asio::async_write(m_socket, boost::asio::buffer(meb.data, meb.offset), yield[ec]);

            if ( ec )
              {
                if ( ec.value() != boost::asio::error::in_progress &&
                     ec.value() != boost::asio::error::try_again &&
                     ec.value() != boost::asio::error::operation_aborted )
                  {
                    fprintf(stderr, "error writing message: %s\n", strerror(ec.value()));
                  }
                break;
              }

            if ( ! stopped )
              {
                if ( ! ec  )    /* Dispatch the 'sent' handlers. */
                  m_io_service.post(std::bind(&MessageDispatcher<BasicNode>::dispatch, &dispatcher,
                                            message, MessageDirection::OUTGOING));
                else            /* Reinsert the message for another go later. */
                  m_outgoing_queue.emplace(std::move(message));
              }
          }
        fprintf(stderr, "[0x%x] Exiting send loop.\n", THREAD_ID);

        m_io_service.post(std::bind(&BasicNode::halt, this, false));
      }


      /** Read data from the socket until we find a valid sync message. */
      bool
      resync(boost::asio::yield_context& yield)
      {
        constexpr const char* sync_string = "\x04\x00\x02\x53\x59\x4E\x43";
        char buf[strlen(sync_string)];

        boost::system::error_code ec;
        boost::asio::streambuf read_buf;
        boost::asio::async_read_until(m_socket, read_buf, sync_string, yield[ec]);

        if ( ec )
          fprintf(stderr, "resync: read_until failed: %s\n", strerror(ec.value()));

        return ! static_cast<bool>(ec);
      }

      /** Read incoming messages and insert them into the incoming-message queue.
       *
       * @param yield A Boost.Asio `yield_context`, used to enable resuming of this method from
       *     asynchronous IO-completion handlers.
       */
      void
      receive_loop(boost::asio::yield_context yield)
      {
        fprintf(stderr, "[0x%x] Entered receive loop.\n", THREAD_ID);

        crisp::util::Buffer rdbuf ( BUFSIZ );

        while ( ! stopped )
          {
            Message m;
            boost::system::error_code ec;

            /* Read the header. */
            boost::asio::async_read(m_socket,
                                    boost::asio::buffer(&m.header, sizeof(m.header)),
                                    boost::asio::transfer_exactly(sizeof(m.header)),
                                    yield[ec]);
            if ( ec )
              {
                if ( ec.value() != boost::asio::error::operation_aborted )
                  fprintf(stderr, "error reading message header: %s\n", strerror(ec.value()));
                break;
              }


            /* Make sure our buffer is big enough. */
            size_t encoded_size ( m.get_encoded_size() );
            if ( rdbuf.length < encoded_size )
              rdbuf.resize(encoded_size);

            /* Copy the header data into the read buffer to make monolithic decoding easier
               later. */
            memcpy(rdbuf.data, &m.header, sizeof(m.header));

            /* Read the message body (and possibly checksum as well) if necessary. */
            const detail::MessageTypeInfo& mti ( detail::get_type_info(m.header.type) );
            if ( mti.has_body )
              {
                size_t 
                  n  = boost::asio::async_read(m_socket,
                                               boost::asio::buffer(rdbuf.data + sizeof(m.header),
                                                                   m.header.length),
                                               boost::asio::transfer_exactly(m.header.length),
                                               yield[ec]);
                if ( ec )
                  {
                    if ( ec.value() != boost::asio::error::operation_aborted )
                      fprintf(stderr, "error reading message body: %s\n", strerror(ec.value()));
                    break;
                  }
                else
                  assert(n == m.header.length);
              }

            /* Decode the message. */
            DecodeBuffer db ( rdbuf.data, rdbuf.length );

            /* Dump the message contents. */
            /* for ( size_t i = sizeof(m.header); i < sizeof(m.header) + m.header.length; ++i )
             *   fprintf(stderr, " %02hhX", mdata[i]);
             * fputs("\n", stderr); */

            m = Message::decode(db);

            if ( mti.has_checksum && ! m.checksum_ok() )
              {
                if ( ! resync(yield) )
                  break;
              }
            else
              m_incoming_queue.emplace(std::move(m));
          }
        fprintf(stderr, "[0x%x] Exiting receive loop.\n", THREAD_ID);

        m_io_service.post(std::bind(&BasicNode::halt, this, false));
      }
    };
  }
}

#endif  /* crisp_comms_BasicNode_hh */
