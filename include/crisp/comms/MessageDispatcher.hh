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

    /** Container and interface for user-set sent/received callbacks.
     */
    template < typename _Node >
    class MessageDispatcher
    {
    public:

      /** Initialize the dispatcher to handle callbacks for the given node.
       *
       * @param node Node for which the dispatcher will be handling event callbacks.
       */
      MessageDispatcher(_Node& node);

      /** Virtual destructor provided to enable polymorphic use of derived classes.
       */
      virtual ~MessageDispatcher();


      /** Set certain message handlers to generic callbacks.  This method is called by the
       * MessageDispatcher constructor, and may be used later to return the node to default
       * behaviour.
       *
       * The set callbacks include:
       *
       * - Send and receive handlers in `sync`: informational output only.  Resync is handled as
       *   necessary by the node implementation itself, so these would only be changed to
       *   silence or change the report format of the handler.
       *
       * - Send and receive handlers in `handshake` and `handshake_response`: these callbacks
       *   ensure that the local and connected nodes agree on each others' roles, as well as
       *   produce informational (debug) output on `stderr`.  If a handshake fails for any
       *   reason, the `handshake_response.received` handler function will halt the local node.
       *
       * - Send and receive handlers in `module_control`: informational output only.  You will
       *   almost certainly want to override the `received` handler function for nodes operating
       *   in slave mode.
       */
      void
      set_default_callbacks();

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
