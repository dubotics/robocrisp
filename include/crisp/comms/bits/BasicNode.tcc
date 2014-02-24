/** @file
 *
 * Contains implementation of the BasicNode template class.
 */
#ifndef crisp_comms_bits_BasicNode_tcc
#define crisp_comms_bits_BasicNode_tcc 1

#include <crisp/comms/Handshake.hh>
#include <crisp/util/Buffer.hh>

#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>


namespace crisp
{
  namespace comms
  {
    template < typename _Protocol >
    BasicNode<_Protocol>::BasicNode(typename _Protocol::socket&& _socket, NodeRole _role)
      : WorkerObject ( _socket.get_io_service(), 5 ),
        m_socket ( std::move(_socket) ),
        m_sync_action ( ),
        m_halt_action ( ),
        m_outgoing_queue ( ),
        m_halting ( ),
        m_stopped ( false ),
        m_halt_mutex ( ),
        m_halt_cv ( ),
        m_disconnect_signal ( ),
        m_disconnect_emitted ( ),
        stopped ( m_stopped ),
        scheduler ( m_io_service ),
        role ( _role ),
        configuration ( ),
        dispatcher ( *this )
    {
      m_halting.clear();
      m_disconnect_emitted.clear();
    }

    template < typename _Protocol >
    BasicNode<_Protocol>::~BasicNode()
    { halt(true); }


    template < typename _Protocol >
    typename BasicNode<_Protocol>::DisconnectSignal::Connection
    BasicNode<_Protocol>::on_disconnect(typename BasicNode<_Protocol>::DisconnectSignal::Function func)
    {
      return m_disconnect_signal.connect(func);
    }


    template < typename _Protocol >
    void
    BasicNode<_Protocol>::run()
    {
      launch();

      {
        std::unique_lock<std::mutex> lock ( m_halt_mutex );
        m_halt_cv.wait(lock);
      }

      if ( ! m_stopped )
        halt();
    }

    template < typename _Protocol >
    bool
    BasicNode<_Protocol>::launch()
    {
      if ( running() )
        return false;

      /* NOTA BENE: for some reason, the order in which the next four statements
         are executed is CRITICAL.  I don't completely understand why right now
         -- it has something to do with threading and having something for the
         worker threads to do (maybe also having worker threads already
         available for the coroutine spawns?).  */
      send(Handshake { PROTOCOL_VERSION, role });

      WorkerObject::launch();

      boost::asio::spawn(m_io_service, std::bind(&BasicNode::receive_loop, this,
                                                 std::placeholders::_1));
      boost::asio::spawn(m_io_service, std::bind(&BasicNode::send_loop, this,
                                                 std::placeholders::_1));

      /* The default `handshake_response.received` handler will cancel this
         action on successful handshake sequence. */
      m_halt_action =
        scheduler.set_timer(std::chrono::seconds(5),
                            [this](crisp::util::ScheduledAction&)
                            { fprintf(stderr, "[0x%x][Node] Handshake not completed before timer expired.  Halting.\n",
                                      THREAD_ID);
                              halt(); });

      return true;
    }

    template < typename _Protocol >
    bool
    BasicNode<_Protocol>::halt(bool force_try)
    {
      if ( ! m_halting.test_and_set() )
        {
          std::unique_lock<std::mutex> lock ( m_halt_mutex );
          m_stopped = true;

          if ( ! m_disconnect_emitted.test_and_set() )
            m_disconnect_signal.emit(*this);

          if ( ! can_halt() && ! force_try )
            {               /* Can't halt from this thread. */
              m_stopped = false;
              m_halting.clear();

              m_halt_cv.notify_all();
              m_io_service.poll();

              return false;
            }

          fprintf(stderr, "[0x%x][Node] Halting... ", THREAD_ID);
          fflush_unlocked(stderr); /* make sure we print diagnostics in the
                                      right order. */

          m_socket.close();

          if ( ! m_sync_action.expired() )
            m_sync_action.lock()->cancel();

          if ( ! m_halt_action.expired() )
            m_halt_action.lock()->cancel();

          m_sync_action.reset();
          m_halt_action.reset();

          m_outgoing_queue.wake_all();
          m_io_service.poll();

          WorkerObject::halt();

          fflush_unlocked(stderr);
          fprintf(stderr, "done.\n");

          m_halt_cv.notify_all();
          return true;
        }
      else
        return false;
    }

    template < typename _Protocol >
    void
    BasicNode<_Protocol>::send(const Message& m)
    {
      m_outgoing_queue.push(m);
    }

    template < typename _Protocol >
    void
    BasicNode<_Protocol>::send(Message&& m)
    {
      m_outgoing_queue.push(std::move(m));
    }

    template < typename _Protocol >
    void
    BasicNode<_Protocol>::send_loop(boost::asio::yield_context yield)
    {
      bool aborted ( false );

      fprintf(stderr, "[0x%x][Node] Entered send loop.\n", THREAD_ID);
      while ( ! m_stopped )
        {
          /* Fetch the next message to send, waiting if necessary. */
          Message message ( m_outgoing_queue.next(&aborted) );

          if ( aborted || m_stopped )
            break;

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
                dispatcher.dispatch(std::move(message), MessageDirection::OUTGOING);
              else if ( ec.value() == boost::asio::error::try_again )
                /* Reinsert the message for another go later. */
                m_outgoing_queue.emplace(std::move(message));
            }
        }
      fprintf(stderr, "[0x%x][Node] Exiting send loop.\n", THREAD_ID);

      if ( ! m_stopped )
        m_io_service.post(std::bind(&BasicNode::halt, this, false));
    }

    template < typename _Protocol >
    bool
    BasicNode<_Protocol>::resync(boost::asio::yield_context& yield)
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

    template < typename _Protocol >
    void
    BasicNode<_Protocol>::receive_loop(boost::asio::yield_context yield)
    {
      fprintf(stderr, "[0x%x][Node] Entered receive loop.\n", THREAD_ID);

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
            dispatcher.dispatch(std::move(m), MessageDirection::INCOMING);
        }
      fprintf(stderr, "[0x%x][Node] Exiting receive loop.\n", THREAD_ID);

      if ( ! m_stopped )
        m_io_service.post(std::bind(&BasicNode::halt, this, false));
    }

  }
}

#endif  /* crisp_comms_bits_BasicNode_tcc */
