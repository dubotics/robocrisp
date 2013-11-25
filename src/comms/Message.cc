#include <crisp/comms/Message.hh>
#include <thread>
#define THREAD_ID							\
  (__extension__							\
   ({ std::thread::id _id ( std::this_thread::get_id() );		\
     *reinterpret_cast<unsigned int*>(&_id);				\
   }))

#define TRACE()					\
  fprintf(stderr, "[0x%0x] \033[96mTRACE: \033[0m \033[97m%s\033[0m (0x%p)\n", THREAD_ID, __PRETTY_FUNCTION__, this)

#define MTI_FOR(n) MessageType::n, #n
namespace crisp
{
  namespace comms
  {
    namespace detail
    {
      static const MessageTypeInfo
      message_type_info[] =
	{ { MTI_FOR(HANDSHAKE),		true, false,		FlowDirection::ANY,	  nullptr },
	  { MTI_FOR(HANDSHAKE_RESPONSE),	true, false,		FlowDirection::ANY,	  nullptr },
	  { MTI_FOR(SYNC), 		true, false, 		FlowDirection::ANY, 	  "SYNC" },
	  { MTI_FOR(ERROR), 		true, true, 		FlowDirection::ANY, 	  nullptr },
	  { MTI_FOR(CONFIGURATION_QUERY), false, false,		FlowDirection::TO_SLAVE,  "QUERY-CONFIGURATION" },
	  { MTI_FOR(CONFIGURATION_RESPONSE), true, true,		FlowDirection::TO_MASTER, nullptr },
	  { MTI_FOR(SENSOR_DATA), 	true, true, 	       	FlowDirection::TO_MASTER, nullptr },
	  { MTI_FOR(MODULE_CONTROL), 	true, true,	       	FlowDirection::TO_SLAVE,   nullptr } };
  
      static_assert(sizeof(message_type_info) / sizeof(MessageTypeInfo) == MESSAGE_TYPE_COUNT, "`message_type_info` array needs update!");

      const MessageTypeInfo&
      get_type_info(MessageType type)
      {
	assert(static_cast<size_t>(type) < MESSAGE_TYPE_COUNT);
	assert(message_type_info[static_cast<size_t>(type)].type == type);
	return message_type_info[static_cast<size_t>(type)];
      }
    }



    const size_t
    Message::HeaderSize =
      offsetof(Message, body);

    Message::Message()
      : length ( 0 ),
	type ( MessageType::HANDSHAKE ),
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
	sequence_id ( 0 ),
#endif
	body ( nullptr )
    {
      /* TRACE(); */
    }

    Message::Message(Message&& m)
      : length ( m.length ),
	type ( m.type ),
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
	sequence_id ( m.sequence_id ),
#endif
	body ( std::move(m.body) )
    {
      /* TRACE(); */
    }

    Message::Message(const Message& m)
      : length ( m.length ),
	type ( m.type ),
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
	sequence_id ( m.sequence_id ),
#endif
	body ( m.body )
    {
      /* TRACE(); */
    }

    Message::Message(MessageType _type)
      : length ( 0 ),
	type ( _type ),
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
	sequence_id ( ),
#endif
	body ( nullptr )
    {
      /* TRACE(); */
      length = get_encoded_size() - HeaderSize;
    }

    Message&
    Message::operator =(Message&& m)
    {
      /* TRACE(); */
      length = m.length;
      type = m.type;
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
      sequence_id = m.sequence_id;
#endif
      body.reset(m.body.get());
      return *this;
    }


    size_t
    Message::get_encoded_size() const
    {
      const detail::MessageTypeInfo& info ( detail::get_type_info(type) );
      if ( info.has_checksum )
	assert(info.has_body);
      if ( info.has_body )
	assert((body && ! info.body) || info.body);

      return HeaderSize
	+ ( info.has_body ? ( body ? body->length : strlen(info.body)) : 0 )
	+ ( info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0 );
    }

    Message
    Message::decode(DecodeBuffer& db)
    {
      Message out;

      db.read(&out, HeaderSize);
      const detail::MessageTypeInfo& info ( detail::get_type_info(out.type) );

      if ( info.has_body )
	{ if ( ! info.body )
	    out.body.reset(Buffer::copy_new(db.data + db.offset, out.length - ( info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0 )));
	  db.offset += out.length - (info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0);
	}

      if ( info.has_checksum )
	db.read(&(out.checksum), sizeof(out.checksum));

      return out;
    }

    EncodeResult
    Message::encode(MemoryEncodeBuffer& buf) const
    {
      size_t initial_offset ( buf.offset );

      buf.write(this, HeaderSize);
      const detail::MessageTypeInfo& info ( detail::get_type_info(type) );

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
	{
	  assert(info.has_body);
	  uint32_t _checksum ( crisp::util::crc32(buf.data + initial_offset, buf.offset - initial_offset) );
	  buf.write(&_checksum, MESSAGE_CHECKSUM_SIZE);
	}

      return EncodeResult::SUCCESS;
    }

    EncodeResult
    Message::encode(StreamEncodeBuffer& buf) const
    {
      buf.write(this, HeaderSize);
      const detail::MessageTypeInfo& info ( detail::get_type_info(type) );
      if ( info.has_checksum && ! checksum )
	const_cast<uint32_t&>(checksum) = compute_checksum();

      assert((body && ! (info.body)) || (info.body && ! body) || ((! body) && (! (info.body)) && (! info.has_body)));

      if ( info.has_body )
	buf.write(body ? body->data : info.body, body ? body->length : strlen(info.body));

      if ( info.has_checksum )
	buf.write(&checksum, MESSAGE_CHECKSUM_SIZE);

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

      m.length = (info.body ? strlen(info.body) : 0) + (info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0 );
      m.type = _type;
      buf.write(&m, HeaderSize);

      if ( info.has_body )
	buf.write(info.body, m.length - (info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0));

      if ( info.has_checksum )
	{
	  m.checksum = crisp::util::crc32(buf.data + initial_offset,
					  buf.offset - initial_offset);
	  buf.write(&(m.checksum), MESSAGE_CHECKSUM_SIZE);
	}
      return EncodeResult::SUCCESS;
    }

    uint32_t
    Message::compute_checksum() const
    {
      const detail::MessageTypeInfo& info ( detail::get_type_info(type) );
      if ( ! info.has_checksum )
	return 0;

      /* Allocate memory for the buffers on the stack, since we don't need to pass it to any other
	 functions.  */
      size_t bufsize ( Message::HeaderSize + ( body ? body->length : 0 ) );
      char buf[bufsize];

      memcpy(buf, this, HeaderSize);
      if ( body )
	memcpy(buf + HeaderSize, body->data, body->length);

      return crisp::util::crc32(buf, bufsize);
    }

    bool
    Message::operator ==(const Message& m) const
    {
      return
	type == m.type &&
	length == m.length &&
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
	sequence_id == m.sequence_id &&
#endif
	compute_checksum() == m.compute_checksum();
    }


  }
}
