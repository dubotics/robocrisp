#ifndef Message_hh
#define Message_hh 1


#include <cassert>
#include "Buffer.hh"
#include "common.hh"
#include "checksum.hh"

/** Message representation.  This class provides little to no encapsulation (not important for
 *  us!), but does do some memory-management.
 */
namespace Robot
{

  ENUM_CLASS(MessageType, uint8_t,
	     HANDSHAKE,
	     HANDSHAKE_RESPONSE,
	     SYNC,
	     ERROR,
	     CONFIGURATION_QUERY,
	     CONFIGURATION_RESPONSE,
	     SENSOR_DATA,
	     MODULE_CONTROL
	     );

#ifndef SWIG
#  define MESSAGE_TYPE_MAX MessageType::MODULE_CONTROL
#  define MESSAGE_TYPE_COUNT (static_cast<uint8_t>(MESSAGE_TYPE_MAX)+1u)

  namespace detail
  {
    /** Bitfield values used to specify message flow direction. */
    ENUM_CLASS(FlowDirection, uint8_t,
	       TO_MASTER   = 1 << 0,
	       TO_SLAVE	   = 1 << 1,
	       ANY = TO_MASTER | TO_SLAVE
	       );

    struct
    MessageTypeInfo
    {
      MessageType type;
      const char* name;

      bool
	has_body : 2,
	has_checksum : 2;

      /** Bitwise-OR of allowed flow direction bits (from FlowDirection). */
      FlowDirection allowed_flow : 4;

      const char* body;
    };

    const MessageTypeInfo&
    get_type_info(MessageType type);
  }
#endif

  struct __attribute__ (( packed, align(1) ))
  Message
  {
    /** Default constructor. */
    Message();

    /** Move constructor. */
    Message(Message&& m);

    Message(MessageType _type);

    Message(const Message&);

    template < typename _T,
	       typename _Enable = typename std::enable_if<!std::is_same<_T,Robot::Message>::value>::type >
    Message(const _T& _body)
      : Message(std::move(_body))
    {}

    template < typename _T, typename _U = typename std::remove_reference<_T>::type,
	       typename _Enable = typename std::enable_if<!std::is_same<_U,Robot::Message>::value>::type>
    Message(_T&& _body)
      : Message()
    {
      const detail::MessageTypeInfo& info ( detail::get_type_info(_U::Type) );
      type = _U::Type;
      MemoryEncodeBuffer eb ( _body.get_encoded_size() );
      _body.encode(eb);
      body.reset(Buffer::copy_new(eb.data, eb.length));
      length = _body.get_encoded_size() + (info.has_checksum ? MESSAGE_CHECKSUM_SIZE : 0);
      checksum = compute_checksum();
    }

    Message&
    operator =(Message&& m);

    size_t get_encoded_size() const;

    /** Decode the Message-layer data in a decode buffer.  */
    static Message
    decode(DecodeBuffer& db);


    /** Encode a Message instance into a buffer. */
    EncodeResult
    encode(MemoryEncodeBuffer& buf) const;

    EncodeResult
    encode(StreamEncodeBuffer& buf) const;

    /* template < typename _T >
     * static inline EncodeResult
     * encode(MemoryEncodeBuffer& buf, const _T&& _body)
     * { return Message(_body).encode(buf); } */

    /** Encode a message with pre-specified (or no) body into a buffer.  */
    static EncodeResult
    encode(MemoryEncodeBuffer& buf, MessageType type);

    template < typename _T, typename... _Args >
    inline _T
    as(_Args&&... args) const
    {
      if ( ! body )
	throw std::runtime_error("Cannot convert `null` body to object form");
      else
	{
	  DecodeBuffer db ( body.get() );
	  return _T::decode_copy(db, args...);
	}
    }

    bool
    operator ==(const Message&) const;

    uint32_t compute_checksum() const;

    inline bool checksum_ok() const {
      if ( detail::get_type_info(type).has_checksum )
	return checksum == compute_checksum();
      else
	return true;
    }

    /** Number of bytes present at the start of all encoded Message objects. */
    static const size_t HeaderSize;

    uint16_t length;		/**< Number of bytes *after the header* in this message.  If
				   applicable, this values _does_ include the length of the
				   checksum field.
				 */
    MessageType type;		/**< Message type (a value of type MessageType) */
#if defined(MESSAGE_USE_SEQUENCE_ID) && MESSAGE_USE_SEQUENCE_ID
    uint32_t sequence_id;	/**< Sequence number of this message.  */
#endif

    spt::RefTraits<Buffer>::stored_ref body;

    uint32_t checksum;		/**< Stored checksum value.  The checksum is computed over the
				 * entire message (with the exception, of course, of the
				 * checksum field itself).
				 */
  };
}

/**@}*/

#endif	/* Message_hh */
