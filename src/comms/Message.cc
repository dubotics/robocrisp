#include <crisp/comms/Message.hh>
#include <thread>
#define THREAD_ID							\
  (__extension__							\
   ({ std::thread::id _id ( std::this_thread::get_id() );		\
     *reinterpret_cast<unsigned int*>(&_id);				\
   }))

/* #define TRACE()								\
    fprintf(stderr, "[0x%0x] \033[96mTRACE: \033[0m \033[97m%s\033[0m (0x%p)\n", THREAD_ID, __PRETTY_FUNCTION__, this) */
#define TRACE()


#define MTI_FOR(n) MessageType::n, #n
namespace crisp
{
  namespace comms
  {
    namespace detail
    {
      static const MessageTypeInfo
      message_type_info[] =
	{ { MTI_FOR(HANDSHAKE),			true, false,	FlowDirection::ANY,	  nullptr },
	  { MTI_FOR(HANDSHAKE_RESPONSE),	true, false,	FlowDirection::ANY,	  nullptr },
	  { MTI_FOR(SYNC), 			true, false, 	FlowDirection::ANY, 	  "SYNC" },
	  { MTI_FOR(ERROR), 			true, true, 	FlowDirection::ANY, 	  nullptr },
	  { MTI_FOR(CONFIGURATION_QUERY), 	false, false,	FlowDirection::TO_SLAVE,  nullptr },
	  { MTI_FOR(CONFIGURATION_RESPONSE), 	true, true,	FlowDirection::TO_MASTER, nullptr },
	  { MTI_FOR(SENSOR_DATA), 		true, true, 	FlowDirection::TO_MASTER, nullptr },
	  { MTI_FOR(MODULE_CONTROL), 		true, true,	FlowDirection::TO_SLAVE,  nullptr } };
  
      static_assert(sizeof(message_type_info) / sizeof(MessageTypeInfo) == MESSAGE_TYPE_COUNT, "`message_type_info` array needs update!");

      const MessageTypeInfo&
      get_type_info(MessageType type)
      {
	assert(static_cast<size_t>(type) < MESSAGE_TYPE_COUNT);
	assert(message_type_info[static_cast<size_t>(type)].type == type);
	return message_type_info[static_cast<size_t>(type)];
      }
    }



    Message::Message()
      : header {
            0,
	    MessageType::HANDSHAKE
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
	   , 0
#endif
       },
      body ( nullptr ),
      checksum ( 0 )
    {
      TRACE();
    }

    Message::Message(Message&& m)
      : header ( std::move(m.header) ),
	body ( std::move(m.body) ),
	checksum ( 0 )
    {
      TRACE();
    }

    Message::Message(const Message& m)
      : header ( m.header ),
	body ( m.body ),
	checksum ( m.checksum )
    {
      TRACE();
    }

    Message::Message(MessageType _type)
      : header { 0, _type },
        body ( nullptr ),
        checksum ( 0 )
    {
      TRACE();
      header.length = get_encoded_size() - sizeof(Header);
    }

    Message&
    Message::operator =(Message&& m)
    {
      TRACE();
      header = std::move(m.header);
      body = std::move(m.body);
      checksum = std::move(m.checksum);
      return *this;
    }


    size_t
    Message::get_encoded_size() const
    {
      TRACE();
      const detail::MessageTypeInfo& info ( detail::get_type_info(header.type) );
      if ( info.has_checksum )
	assert(info.has_body);

      if ( ! info.body && ! body )
        return sizeof(Header) + header.length;
      else
        {
          assert((body && ! info.body) || info.body);

          return sizeof(Header)
            + ( info.has_body ? ( body ? body->length : strlen(info.body)) : 0 )
            + ( info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0 );
        }
    }

    Message
    Message::decode(DecodeBuffer& db)
    {
      Message out;

      db.read(&out.header, sizeof(Header));
      const detail::MessageTypeInfo& info ( detail::get_type_info(out.header.type) );

      if ( info.has_body )
	{ size_t body_size ( out.header.length - ( info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0 ) );
          out.body.reset(new Buffer(body_size));
	  db.read(out.body->data, body_size);
	}

      if ( info.has_checksum )
	db.read(&(out.checksum), sizeof(out.checksum));

      return out;
    }

    EncodeResult
    Message::encode(MemoryEncodeBuffer& buf) const
    {
      size_t initial_offset ( buf.offset );

      buf.write(&header, sizeof(Header));
      const detail::MessageTypeInfo& info ( detail::get_type_info(header.type) );

      if ( info.has_body )
	{
	  if ( body )
	    {
	      assert(info.body == nullptr);
	      buf.write(body->data, body->length);
	    }
	  else
	    {
	      assert(info.body != nullptr);
	      buf.write(info.body, strlen(info.body));
	    }
	}

      if ( info.has_checksum )
	{ checksum = compute_checksum();
          buf.write(&checksum, MESSAGE_CHECKSUM_SIZE);
	}

      return EncodeResult::SUCCESS;
    }

    EncodeResult
    Message::encode(StreamEncodeBuffer& buf) const
    {
      buf.write(&header, sizeof(Header));
      const detail::MessageTypeInfo& info ( detail::get_type_info(header.type) );
      if ( info.has_checksum && ! checksum )
	const_cast<uint32_t&>(checksum) = compute_checksum();

      assert((body && ! (info.body)) || (info.body && ! body) || ((! body) && (! (info.body)) && (! info.has_body)));

      if ( info.has_body )
	buf.write(body ? body->data : info.body, body ? body->length : strlen(info.body));

      if ( info.has_checksum )
	{ checksum = compute_checksum();
	  buf.write(&checksum, MESSAGE_CHECKSUM_SIZE);
	}

      return EncodeResult::SUCCESS;
    }


    /* Static method for encoding bodyless or pre-specified-body messages. */
    EncodeResult
    Message::encode(MemoryEncodeBuffer& buf, MessageType _type)
    {
      size_t initial_offset ( buf.offset );
      Message m;
      const detail::MessageTypeInfo& info ( detail::get_type_info(_type) );

      if ( info.has_checksum )
	assert(info.has_body);
      if ( info.has_body )
	assert(info.body != nullptr);

      m.header.length = (info.body ? strlen(info.body) : 0) + (info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0 );
      m.header.type = _type;
      buf.write(&m.header, sizeof(Header));

      if ( info.has_body )
	buf.write(info.body, m.header.length - (info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0));

      if ( info.has_checksum )
	{
	  m.checksum = m.compute_checksum();
	  buf.write(&(m.checksum), MESSAGE_CHECKSUM_SIZE);
	}
      return EncodeResult::SUCCESS;
    }

    uint32_t
    Message::compute_checksum() const
    {
      const detail::MessageTypeInfo& info ( detail::get_type_info(header.type) );
      if ( ! info.has_checksum )
	return 0;

      /* Allocate memory for the buffers on the stack, since we don't need to pass it to any other
	 functions.  */
      size_t
        body_size ( body ? body->length : strlen(info.body) ),
        bufsize ( sizeof(Header) + body_size );
      char buf[bufsize];

      memcpy(buf, this, sizeof(Header));
      if ( body )
	memcpy(buf + sizeof(Header), body ? body->data : info.body, body_size);

      return crisp::util::crc32(buf, bufsize);
    }

    bool
    Message::operator ==(const Message& m) const
    {
      return
	header == m.header &&
	( body == m.body ||
	  ( body && m.body &&
	    body->length == m.body->length &&
	    ! memcmp(body->data, m.body->data, body->length) ) ) &&
	compute_checksum() == m.compute_checksum();
    }


  }
}
