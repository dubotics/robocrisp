/* -*- C++ -*- */
#define BOOST_ASIO_DISABLE_EPOLL

#include <iostream>
#include <chrono>
#include <boost/asio/read.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <crisp/comms/Handshake.hh>

#define TRACE()								\
  fprintf(stderr, "[0x%0x] \033[96mTRACE: \033[0m \033[97m%s\033[0m\n", THREAD_ID, __PRETTY_FUNCTION__)

#define THREAD_ID							\
  (__extension__							\
   ({ std::thread::id _id ( std::this_thread::get_id() );		\
     *reinterpret_cast<unsigned int*>(&_id);				\
   }))


using namespace std::placeholders;

static const char* ReceiveStep_names[] =
  { "READ_HEADER", "READ_TAIL", "INSERT_INTO_QUEUE", "RESYNC", "RESYNC_COMPLETE" };

namespace crisp
{
  namespace comms
  {
    template < typename _SocketType>
    ProtocolNode<_SocketType>::ProtocolNode(_SocketType&& socket, NodeRole role)
      : role ( role ),
	m_io_service ( socket.get_io_service() ),
	m_socket ( std::move(socket) ),
	m_send ( *this, std::mem_fn(&ProtocolNode<_SocketType>::send_next) ),
	m_receive ( *this, std::mem_fn(&ProtocolNode<_SocketType>::receive_handler) ),
	m_send_buffer ( ),
	m_receive_buffer ( 8192 ),
	m_receive_step ( ReceiveStep::READ_HEADER ),
	m_sync_thread ( ),
	m_sync_strand ( m_io_service ),
	m_sync_handler ( m_sync_strand.wrap(std::bind(std::mem_fn(&ProtocolNode<_SocketType>::send_sync), this, _1)) ),
	m_sync_timer ( m_io_service ),
	m_synced ( true ),
	m_stopping ( false ),
	m_dispatch_threads ( ),
	m_dispatch_sent_strand ( m_io_service ),
	m_wait_mutex ( ),
	m_wait_cv ( ),
	dispatcher ( *this )
    {
    }

    template < typename _SocketType>
    ProtocolNode<_SocketType>::~ProtocolNode()
    {
      halt();
    }


    template < typename _SocketType>
    ProtocolNode<_SocketType>::IOState::IOState(IOState&& state)
      : node ( state.node ),
      thread ( std::move(state.thread) ),
      strand ( state.strand.get_io_service() ),
      queue ( state.queue ),
      handler ( state.handler ),
      remaining_bytes ( state.remaining_bytes )
    {}

    template < typename _SocketType>
    ProtocolNode<_SocketType>::IOState::~IOState()
    {
      if ( thread.joinable() )
	thread.join();
    }


    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::IOState::run(std::function<void()> _handler)
    {
      thread = std::thread(_handler);
    }
  

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::send_do(Message&& m)
    {
      /* TRACE(); */

      m_send_message = std::move(m);

      m_send_buffer.prepare(m_send_message.get_encoded_size());
      StreamEncodeBuffer buf { &m_send_buffer };
      m_send_message.encode(buf);
      m_send.remaining_bytes = m_send_message.get_encoded_size();

      /* const detail::MessageTypeInfo& info ( detail::get_type_info(m.type) );
       * fprintf(stderr, "[0x%0x] \033[1;33mSending message:\033[0m %s (%zu bytes)\n", THREAD_ID, info.name, m_send.remaining_bytes); */

      /* Add the work item for the write operation. */
      boost::asio::async_write(m_socket, m_send_buffer, m_send.handler);
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::send_next(const boost::system::error_code& error,
					 std::size_t bytes_transferred)
    {
      /* TRACE(); */

      if ( ! m_stopping )
	{
	  if ( ! error )
	    {
	      /* Consume the transferred bytes. */
	      if ( bytes_transferred > 0 )
		{
		  assert(bytes_transferred <= m_send.remaining_bytes);

		  m_send_buffer.consume(bytes_transferred);
		  m_send.remaining_bytes -= bytes_transferred;
		}

	      /* Do we have any more data to send for the current message? */
	      if ( m_send.remaining_bytes > 0 )
		{
		  /* fprintf(stderr, "remaining_bytes: %zu\n", m_send.remaining_bytes); */
		  boost::asio::async_write(m_socket, m_send_buffer, m_send.handler);
		}
	      else
		{
		  if ( bytes_transferred > 0 )
		    {
		      /* const detail::MessageTypeInfo& info ( detail::get_type_info(m_send_message.type) );
		       * fprintf(stderr, "[0x%0x] \033[1;33mSent message:\033[0m %s (%zu bytes)\n", THREAD_ID, info.name, bytes_transferred); */
		      m_dispatch_sent_strand.post([&]{ dispatcher.dispatch(m_send_message, MessageDirection::OUTGOING); });
		    }

		  send_do(m_send.queue.next());
		}
	    }
	  else
	    fprintf(stderr, "[0x%0x] \033[1;31mwrite error: \033[0m%s\n", THREAD_ID, strerror(error.value()));
	}
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::receive_resync()
    {
      /* TRACE(); */
      fprintf(stderr, "[0x%0x] \033[1;31mSync lost\033[0m\n", THREAD_ID);
      MemoryEncodeBuffer buf ( 32 );
      Message::encode(buf, MessageType::SYNC);

      m_receive_step = ReceiveStep::RESYNC;
      m_read_until_string = std::string(reinterpret_cast<const char*>(buf.data), buf.offset);
      m_receive.remaining_bytes = buf.offset;

      boost::asio::async_read_until(m_socket, m_read_until_buffer, m_read_until_string, m_receive.handler);
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::receive_init()
    { fprintf(stderr, "[0x%0x] \033[1;32mStarting read loop\033[0m\n", THREAD_ID);

      m_receive_buffer.reset();
      m_receive_step = ReceiveStep::READ_HEADER;
      m_receive.remaining_bytes = Message::HeaderSize;

      m_receive.strand.post([&]{ receive_do(); });
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::receive_do()
    {
      assert(m_receive.remaining_bytes > 0);
      /* TRACE(); */
      boost::asio::async_read(m_socket,
			      boost::asio::buffer(m_receive_buffer.data + m_receive_buffer.offset, m_receive_buffer.length - m_receive_buffer.offset),
			      boost::asio::transfer_exactly(m_receive.remaining_bytes),
			      m_receive.handler);
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::receive_handler(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
      /* TRACE(); */
      if ( ! m_stopping )
	{
	  if ( ! error )
	    {
	      assert(bytes_transferred <= m_receive.remaining_bytes);
	      m_receive.remaining_bytes -= bytes_transferred;
	      m_receive_buffer.offset += bytes_transferred;

	      /* Continue the previous read operation if necessary  */
	      if ( m_receive.remaining_bytes > 0 )
		boost::asio::async_read(m_socket, boost::asio::buffer(m_receive_buffer.data + m_receive_buffer.offset,
								      m_receive.remaining_bytes),
					m_receive.handler);
	      else
		{
		  switch ( m_receive_step )
		    {
		    case ReceiveStep::READ_HEADER:
		      {
			/* Here we create a temporary Message object and copy the current contents
			   of the receive buffer into it so we can grab the `length` field without
			   too much hassle.  */
			Message m;
			memcpy(&m, m_receive_buffer.data, Message::HeaderSize);
			m_receive.remaining_bytes = m.length;

			/* const detail::MessageTypeInfo& info ( detail::get_type_info(m.type) );
			 * fprintf(stderr, "[0x%0x] \033[1;32mReceived header:\033[0m length %u, type %s\n", THREAD_ID, m.length, info.name); */
		      }
		      m_receive_step = ReceiveStep::READ_TAIL_AND_ENQUEUE;
		      break;

		    case ReceiveStep::READ_TAIL_AND_ENQUEUE:
		      {
			/* Dump the message contents. */
			/* for ( size_t i = 0; i < m_receive_buffer.offset; ++i )
			 * 	fprintf(stderr, " %02hhX", m_receive_buffer[i]);
			 * fputs("\n", stderr); */

			DecodeBuffer db ( m_receive_buffer.buffer );
			Message m ( Message::decode(db) );

			/* const detail::MessageTypeInfo& info ( detail::get_type_info(m.type) );
			 * fprintf(stderr, "[0x%0x] \033[1;32mReceived message: \033[0m %s\n", THREAD_ID, info.name); */
		      
			if ( ! m.checksum_ok() )
			  m_receive_step = ReceiveStep::RESYNC;
			else
			  {
			    /* Push the message. */
			    m_receive.queue.emplace(std::move(m));

			    /* Set us up to read the next message header. */
			    m_receive_step = ReceiveStep::READ_HEADER;
			    m_receive.remaining_bytes = Message::HeaderSize;
			  }
			m_receive_buffer.reset();
		      }
		      break;

		    case ReceiveStep::RESYNC:
		      fprintf(stderr, "[0x%0x] \033[1;32mSynchronized.\033[0m\n", THREAD_ID);
		      m_receive_step = ReceiveStep::READ_HEADER;
		      m_receive.remaining_bytes = Message::HeaderSize;
		      m_receive_buffer.reset();
		      break;
		    }
		  if ( m_receive_step != ReceiveStep::RESYNC )
		    receive_do();
		  else
		    receive_resync();
		}
	    }
	  else
	    {
	      fprintf(stderr, "[0x%0x] \033[1;31mread error: \033[0m%s\n", THREAD_ID, strerror(error.value()));
	    }
	}
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::send_sync(const boost::system::error_code& error)
    {
      /* TRACE(); */
    
      if ( error != boost::asio::error::operation_aborted )
	{
	  if ( error )
	    fprintf(stderr, "[0x%0x] \033[1;31msync loop error: \033[0m%s\n", THREAD_ID, strerror(error.value()));

	  if ( ! m_stopping )
	    {
	      send({ MessageType::SYNC });
	      /* fprintf(stderr, "[0x%0x] \033[1;34mSYNC\033[0m\n", THREAD_ID); */

	      m_sync_timer.expires_from_now(boost::posix_time::seconds(SYNC_INTERVAL));
	      m_sync_timer.async_wait(m_sync_handler);
	    }
	}
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::stop_sync_send_loop()
    {
      m_sync_timer.cancel();
      if ( m_sync_thread.joinable() )
	m_sync_thread.join();
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::start_sync_send_loop()
    {
      if ( ! m_sync_thread.joinable() )
	{
	  m_sync_handler(boost::system::error_code {});
	  m_sync_thread = std::thread([&]{ m_io_service.run(); });
	}
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::dispatch_received_loop()
    {
      while ( ! m_stopping )
	dispatcher.dispatch(m_receive.queue.next(), MessageDirection::INCOMING);
	    
    }

    template < typename _SocketType >
    bool
    ProtocolNode<_SocketType>::launch()
    {
      bool out ( false );
      if ( ! m_send.thread.joinable() )
	{
	  m_send.strand.post([&]{ send_do(Handshake { PROTOCOL_VERSION, role }); });
	  m_send.run([&] { m_io_service.run(); });
	  out = true;
	}

      if ( ! m_receive.thread.joinable() )
	{
	  m_receive.strand.post([&]{ receive_init(); });
	  m_receive.run([this] { m_io_service.run(); });
	  out = true;
	}

      if ( ! m_dispatch_threads[0].joinable() )
	{
	  m_dispatch_threads[0] = std::thread([&]{ dispatch_received_loop(); });
	  out = true;
	}
      if ( ! m_dispatch_threads[1].joinable() )
	m_dispatch_threads[1] = std::thread([&]{ m_io_service.run(); });

      start_sync_send_loop();

      return out;
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::wait()
    {
      std::unique_lock<std::mutex> lock ( m_wait_mutex );
      while ( ! m_stopping )
	m_wait_cv.wait(lock);
    }


    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::halt()
    {
      m_stopping = true;
      m_io_service.stop();

      stop_sync_send_loop();
      if ( m_send.thread.joinable() )
	{
	  m_send.cv.notify_all();
	  m_send.thread.join();
	}

      if ( m_receive.thread.joinable() )
	m_receive.thread.join();

      if ( m_dispatch_threads[0].joinable() )
	{
	  m_receive.cv.notify_all();
	  m_dispatch_threads[0].join();
	}
      if ( m_dispatch_threads[1].joinable() )
	m_dispatch_threads[1].join();

      m_socket.close();
    }

    template < typename _SocketType>
    void
    ProtocolNode<_SocketType>::send(Message&& msg)
    {
      m_send.queue.push(msg);
    }

    template < typename _SocketType >
    bool
    ProtocolNode<_SocketType>::operator <(const ProtocolNode& other) const
    {
      return const_cast<_SocketType&>(m_socket).native_handle() <
	const_cast<_SocketType&>(other.m_socket).native_handle();
    }

  }
}
