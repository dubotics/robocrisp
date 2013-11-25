#include "Handshake.hh"

namespace Robot
{

  Handshake::Handshake()
    : protocol { PROTOCOL_NAME },
      version { 0 },
      role ( Role::MASTER )
  {
    memset(this, 0, sizeof(Handshake));
  }

  Handshake::Handshake(uint32_t _version, Role _role)
    : protocol { PROTOCOL_NAME },
      version ( _version ),
      role ( _role )
  {}

  Handshake
  Handshake::decode_copy(DecodeBuffer& buffer)
  { Handshake out;
    buffer.read(&out, sizeof(out));
    return out; }

  bool
  Handshake::operator ==(const Handshake& hs) const
  {
    return
      protocol == hs.protocol &&
      version == hs.version &&
      role == hs.role;
  }
}
