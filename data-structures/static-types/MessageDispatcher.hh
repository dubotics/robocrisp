#ifndef Robot_MessageDispatcher_hh
#define Robot_MessageDispatcher_hh 1

#include "Message.hh"
#include "MessageHandler.hh"

namespace Robot
{
  template < typename _SocketType > class ProtocolNode;

  struct Handshake;
  struct HandshakeResponse;
  struct Configuration;
  struct ModuleControl;

  ENUM_CLASS(MessageDirection, uint8_t,
	     INCOMING,
	     OUTGOING);

  template < typename _SocketType >
  class MessageDispatcher
  {
  public:

    MessageDispatcher(ProtocolNode<_SocketType>& node);
    virtual ~MessageDispatcher();

    /* MessageDispatcher&
     * operator = (MessageDispatcher&& md); */

  private:
    ProtocolNode<_SocketType>* m_node;


  public:
    /** Invoke the appropriate handler for the given message and direction.
     */
    void dispatch(const Message& message, MessageDirection direction) throw ( std::runtime_error );

    MessageHandler<Handshake, _SocketType> handshake;
    MessageHandler<HandshakeResponse, _SocketType> handshake_response;
    MessageHandler<void, _SocketType> sync;
    MessageHandler<void, _SocketType> configuration_query;
    MessageHandler<Configuration, _SocketType> configuration_response;
    MessageHandler<Message, _SocketType> module_control;

  };
}

#include "MessageDispatcher.tcc"


#endif	/* Robot_MessageDispatcher_hh */
