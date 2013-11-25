#ifndef MessageHandler_hh
#define MessageHandler_hh 1

#include <type_traits>
#include <functional>
#include <crisp/comms/config.h>

namespace crisp
{
  namespace comms
  {
  template < typename _SocketType > class ProtocolNode;
  struct Message;

  /** Interface template for message handlers. */
  template < typename _BodyType = void, typename _SocketType = PROTOCOL_NODE_DEFAULT_SOCKET_TYPE, typename _Enable = _BodyType>
  struct MessageHandler;

  template < typename _BodyType, typename _SocketType >
  struct
  MessageHandler<_BodyType, _SocketType, typename std::enable_if<std::is_void<_BodyType>::value, void>::type>
  {
    typedef std::function<void(ProtocolNode<_SocketType>&)> HandlerFunction;


    HandlerFunction
      sent,
      received;
  };


  template < typename _BodyType, typename _SocketType >
  struct
  MessageHandler<_BodyType, _SocketType, typename std::enable_if<std::is_same<_BodyType, crisp::comms::Message>::value, _BodyType>::type>
  {
    typedef std::function<void(ProtocolNode<_SocketType>&,const Message&)> HandlerFunction;
    HandlerFunction
      sent,
      received;
  };


  template < typename _BodyType, typename _SocketType >
  struct
  MessageHandler<_BodyType, _SocketType, typename std::enable_if<!(std::is_void<_BodyType>::value||std::is_same<_BodyType, Message>::value), _BodyType>::type>
  {
    typedef std::function<void(ProtocolNode<_SocketType>&,const _BodyType&)> HandlerFunction;
    HandlerFunction
      sent,
      received;
  };

  }
}

#endif	/* MessageHandler_hh */
