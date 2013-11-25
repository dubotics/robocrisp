#ifndef Robot_Handshake_hh
#define Robot_Handshake_hh 1

/* #include "ProtocolNode.hh" */
#include <crisp/comms/common.hh>
#include <crisp/comms/Message.hh>

namespace crisp
{
  namespace comms
  {
    struct __attribute__ (( packed, align(1) ))
    Handshake
    {
      typedef Handshake TranscodeAsType;
      typedef crisp::comms::NodeRole Role;

      static constexpr MessageType Type = MessageType::HANDSHAKE;

      Handshake();
      Handshake(uint32_t _version, Role _role);

      inline size_t
	get_encoded_size() const { return sizeof(Handshake); }

      inline EncodeResult
	encode(MemoryEncodeBuffer& buffer) const
      { return buffer.write(this, sizeof(Handshake)); }

      inline DecodeResult
	decode(DecodeBuffer& buffer) const
      { return buffer.read(this, sizeof(Handshake)); }

      static Handshake
	decode_copy(DecodeBuffer& buffer);

      bool
	operator ==(const Handshake& hs) const;


      uint32_t protocol;
      uint32_t version;
      Role role;
    };

    ENUM_CLASS(HandshakeAcknowledge, uint8_t,
	       ACK,
	       NACK_UNKNOWN_PROTOCOL, /**< Negative acknowledge; unknown protocol name. */
	       NACK_ROLE_CONFLICT,    /**< Negative acknowledge; local and remote nodes request
					 identical roles. */
	       NACK_VERSION_CONFLICT /**< Negative acknowledge; local and remote nodes are
					using differing protocol versions. */
	       );


    struct __attribute__ (( packed, align(1) ))
    HandshakeResponse
    {
      typedef HandshakeResponse TranscodeAsType;
      static constexpr MessageType Type = MessageType::HANDSHAKE_RESPONSE;

      typedef NodeRole Role;

      inline size_t
	get_encoded_size() const { return sizeof(HandshakeResponse); }

      inline EncodeResult
	encode(MemoryEncodeBuffer& buffer) const
      { return buffer.write(this, sizeof(HandshakeResponse)); }

      inline DecodeResult
	decode(DecodeBuffer& buffer) const
      { return buffer.read(this, sizeof(HandshakeResponse)); }

      static inline HandshakeResponse
	decode_copy(DecodeBuffer& buffer)
      { HandshakeResponse out;
	buffer.read(&out, sizeof(out));
	return out; }
    
      inline bool
	operator ==(const HandshakeResponse& hs) const
      { return
	  acknowledge == hs.acknowledge &&
	  role == hs.role; }

      HandshakeAcknowledge acknowledge;
      Role role;			/**< Role accepted by the remote node.  This field has no
					   meaning unless `response` is ACK.  */
    };
  }
}


#endif	/* Robot_Handshake_hh */
