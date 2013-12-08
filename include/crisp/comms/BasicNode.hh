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

#include <crisp/util/PeriodicScheduler.hh>
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
    class BasicNode
    {
    public:
      typedef _Protocol Protocol;
      typedef typename Protocol::socket Socket;

    private:
      /** Reference to the IO service used by the node's components. */
      boost::asio::io_service& m_io_service;

      /** Connected socket used for communication. */
      Socket m_socket;

      /** Handle to the scheduled period-action used for synchronization.  */
      crisp::util::PeriodicAction* m_sync_action;

      crisp::util::SharedQueue<Message>
        m_outgoing_queue,         /**< Outgoing message queue. */
        m_incoming_queue;         /**< Incoming message queue. */

      /** Whether the node's `halt` method has been called. */
      std::atomic_flag m_halting;

      /** Referent for `stopped`.  This is used to provide a way for users of BasicNode to
          determine whether or not the node has been shut down. */
      std::atomic<bool> m_stopped;

      /** Handles for the worker threads used to handle the node's I/O handlers.
       *
       * *Three* is the minimum number of threads allowable here: the send and dispatch loops each
       * block when waiting for new outgoing and incoming messages, respectively, so we need at
       * least one other thread available to execute asynchronous event handlers.
       */
      std::thread m_run_threads[3];

    public:
      /** Public "node-is-halting-or-stopped" flag. */
      const std::atomic<bool>& stopped;

      /** A scheduler for managing periodic communication (and any user-set actions) on the node.
          Within BasicNode, this is used for producing synchronization messages.  */
      crisp::util::PeriodicScheduler scheduler;

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
        : m_io_service ( _socket.get_io_service() ),
          m_socket ( std::move(_socket) ),
          m_sync_action ( nullptr ),
          m_outgoing_queue ( ),
          m_incoming_queue ( ),
          m_halting ( ),
          m_stopped ( false ),
          m_run_threads ( ),
          stopped ( m_stopped ),
          scheduler ( m_io_service ),
          role ( _role ),
          configuration ( ),
          dispatcher ( *this )
      {
        m_halting.clear();

        boost::asio::spawn(m_io_service, std::bind(&BasicNode::send_loop,
                                                 this, std::placeholders::_1));
        boost::asio::spawn(m_io_service, std::bind(&BasicNode::receive_loop, this,
                                                 std::placeholders::_1));
        using namespace crisp::util::literals;

        m_sync_action = &scheduler.schedule(1_Hz, [&](crisp::util::PeriodicAction&)
                                            { send(MessageType::SYNC); });

        send(Handshake { PROTOCOL_VERSION, role });
      }

      ~BasicNode()
      {
        if ( ! stopped )
          halt();
      }


      /** Launch worker threads to handle the node's IO. */
      void launch()
      {
        m_run_threads[0] = std::thread(std::bind(&BasicNode::dispatch_received_loop, this));
        for ( std::thread& thread : m_run_threads )
          if ( ! thread.joinable() ) /* skip initialized handles (e.g. dispatch thread...) */
            thread = std::thread ( std::bind(BasicNode::worker_main,
                                             std::ref(m_io_service),
                                             std::cref(m_stopped)) );
      }

      static void
      worker_main(boost::asio::io_service& io_service,
                  const std::atomic<bool>& stopping)
      {
        while ( ! stopping )
          io_service.run_one();
      }


      /** Halt the node by closing the socket and waiting for its worker threads to finish.  */
      void halt()
      {
        if ( ! m_halting.test_and_set() )
          {
            m_stopped = true;
            fprintf(stderr, "[0x%x] Halting... ", THREAD_ID);

            if ( m_sync_action )
              m_sync_action->cancel();
            m_sync_action = nullptr;

            m_socket.close();
            m_outgoing_queue.wake_all();
            m_incoming_queue.wake_all();

            m_io_service.poll();

            for ( std::thread& thread : m_run_threads )
              if ( thread.joinable() && thread.get_id() != std::this_thread::get_id() )
                thread.join();
            fprintf(stderr, "done.\n");
          }
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

        m_io_service.post(std::bind(&BasicNode::halt, this));
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

        m_io_service.post(std::bind(&BasicNode::halt, this));
      }
    };
  }
}

#endif  /* crisp_comms_BasicNode_hh */
