#ifndef Robot_MessageDispatcher_hh
#define Robot_MessageDispatcher_hh 1

#include <crisp/comms/Message.hh>
#include <crisp/comms/MessageHandler.hh>

namespace crisp
{
  namespace comms
  {
    struct Handshake;
    struct HandshakeResponse;
    struct Configuration;
    struct ModuleControl;

    ENUM_CLASS(MessageDirection, uint8_t,
	       INCOMING,
	       OUTGOING);

    template < typename _Node >
    class MessageDispatcher
    {
    public:

      MessageDispatcher(_Node& node);
      virtual ~MessageDispatcher();

      /* MessageDispatcher&
       * operator = (MessageDispatcher&& md); */

    private:
      _Node* m_node;


    public:
      /** Invoke the appropriate handler for the given message and direction.
       */
      void dispatch(const Message& message, MessageDirection direction) throw ( std::runtime_error );

      MessageHandler<_Node,Handshake> handshake;
      MessageHandler<_Node,HandshakeResponse> handshake_response;
      MessageHandler<_Node,void> sync;
      MessageHandler<_Node,void> configuration_query;
      MessageHandler<_Node,Configuration> configuration_response;
      MessageHandler<_Node,ModuleControl> module_control;

    };
  }
}

#include <crisp/comms/bits/MessageDispatcher.tcc>


#endif	/* Robot_MessageDispatcher_hh */
