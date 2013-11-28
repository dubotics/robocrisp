#ifndef Robot_MessageDispatcher_hh
#define Robot_MessageDispatcher_hh 1

#include <crisp/comms/Message.hh>
#include <crisp/comms/MessageHandler.hh>

namespace crisp
{
  namespace comms
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

      MessageHandler<_SocketType,Handshake> handshake;
      MessageHandler<_SocketType,HandshakeResponse> handshake_response;
      MessageHandler<_SocketType,void> sync;
      MessageHandler<_SocketType,void> configuration_query;
      MessageHandler<_SocketType,Configuration> configuration_response;
      MessageHandler<_SocketType,ModuleControl> module_control;

    };
  }
}

#include <crisp/comms/bits/MessageDispatcher.tcc>


#endif	/* Robot_MessageDispatcher_hh */
