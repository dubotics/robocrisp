/* -*- C++ -*- */
#include <crisp/comms/Handshake.hh>
#include <crisp/comms/Configuration.hh>
#include <crisp/comms/ModuleControl.hh>
#include <crisp/util/Scheduler.hh>

namespace crisp
{
  namespace comms
  {
    namespace detail
    {
      /** Handler-caller helper for handlers that don't receive a message-body
          parameter. */
      template < typename _Node, typename _Body >
      static typename std::enable_if<std::is_void<_Body>::value, void>::type
      call_handler(_Node& node,
                   MessageDirection direction,
                   MessageHandler<_Node, _Body>& handler)
      {
        switch ( direction )
          {
          case MessageDirection::INCOMING:
            handler.received.emit(node);
            break;
          case MessageDirection::OUTGOING:
            handler.sent.emit(node);
            break;
          }
      }


      /** Handler-caller helper for handlers that DO receive a message-body
          parameter. */
      template < typename _Node, typename _Body, typename... Args >
      static typename std::enable_if<!std::is_void<_Body>::value && !std::is_same<_Body,Message>::value, void>::type
      call_handler(_Node& node, Message&& m, MessageDirection direction,
                   MessageHandler<_Node, _Body>& handler, Args... args)
      {
        switch ( direction )
          {
          case MessageDirection::INCOMING:
            handler.received.emit(node, m, args...);
            break;
          case MessageDirection::OUTGOING:
            handler.sent.emit(node, m, args...);
            break;
          }
      }
    }

    template < typename _Node >
    MessageDispatcher<_Node>::MessageDispatcher()
      : m_node ( nullptr ),
        handshake ( ),
        handshake_response ( ),
        sync ( ),
        configuration_query ( ),
        configuration_response ( ),
        module_control ( )
    {
      set_default_callbacks();
    }


    template < typename _Node >
    MessageDispatcher<_Node>::MessageDispatcher(_Node& node)
    : m_node ( &node ),
      handshake ( node.get_io_service() ),
      handshake_response ( node.get_io_service() ),
      sync ( node.get_io_service() ),
      configuration_query ( node.get_io_service() ),
      configuration_response ( node.get_io_service() ),
      module_control ( node.get_io_service() )
    {
      set_default_callbacks();
    }


    template < typename _Node >
    MessageDispatcher<_Node>::~MessageDispatcher()
    {}

    template < typename _Node >
    void
    MessageDispatcher<_Node>::set_target(_Node& node)
    {
      m_node = &node;
      boost::asio::io_service& service ( node.get_io_service() );

      handshake.received.set_io_service(service);
      handshake.sent.set_io_service(service);

      handshake_response.received.set_io_service(service);
      handshake_response.sent.set_io_service(service);

      sync.received.set_io_service(service);
      sync.sent.set_io_service(service);

      configuration_query.received.set_io_service(service);
      configuration_query.sent.set_io_service(service);

      configuration_response.received.set_io_service(service);
      configuration_response.sent.set_io_service(service);

      module_control.received.set_io_service(service);
      module_control.sent.set_io_service(service);
    }

    template < typename _Node >
    void
    MessageDispatcher<_Node>::set_default_callbacks()
    {
      sync.sent
        .connect([&](_Node&)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;34mSYNC\033[0m out\n", THREAD_ID);
                 });

      sync.received
        .connect([&](_Node&)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;34mSYNC\033[0m in\n", THREAD_ID);
                 });


      handshake.sent
        .connect([&](_Node&, const Handshake& hs)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;33mHandshake sent:\033[0m requested role \033[97m%s\033[0m\n", THREAD_ID,
                           hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");
                 });

      handshake.received
        .connect([&](_Node& _node, const Handshake& hs)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;33mHandshake received:\033[0m requests role \033[97m%s\033[0m\n", THREAD_ID,
                           hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");

                   HandshakeAcknowledge ack;

                   if ( hs.protocol != PROTOCOL_NAME )
                     ack = HandshakeAcknowledge::NACK_UNKNOWN_PROTOCOL;
                   else if ( hs.role == _node.role )
                     ack = HandshakeAcknowledge::NACK_ROLE_CONFLICT;
                   else if ( hs.version != PROTOCOL_VERSION )
                     ack = HandshakeAcknowledge::NACK_VERSION_CONFLICT;
                   else
                     ack = HandshakeAcknowledge::ACK;

                   _node.send(HandshakeResponse { ack, hs.role == NodeRole::MASTER ? NodeRole::SLAVE : NodeRole::MASTER });
                 });

      handshake_response.received
        .connect([&](_Node& _node, const HandshakeResponse& hs)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;33mHandshake response received:\033[0m remote node %s role \033[97m%s\033[0m\n", THREAD_ID,
                           hs.acknowledge == HandshakeAcknowledge::ACK ? "\033[32maccepts\033[0m" : "\033[31mrejects\033[0m",
                           hs.role == NodeRole::MASTER ? "MASTER" : (hs.role == NodeRole::SLAVE ? "SLAVE" : "ERROR"));

                   if ( hs.acknowledge == HandshakeAcknowledge::ACK )
                     {
                       if ( ! _node.m_halt_action.expired() )
                         _node.m_halt_action.lock()->cancel();
                       _node.m_halt_action.reset();

                       using namespace crisp::util::literals;
                       _node.m_sync_action = _node.scheduler.schedule(1_Hz, [&](crisp::util::PeriodicAction&)
                                                                      { _node.send(MessageType::SYNC); });
                     }
                   else
                     _node.halt();
                 });

      handshake_response.sent
        .connect([&](_Node&, const HandshakeResponse& hs)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;33mHandshake response sent:\033[0m %s role \033[97m%s\033[0m\n", THREAD_ID,
                           hs.acknowledge == HandshakeAcknowledge::ACK ? "\033[32maccepting\033[0m" : "\033[31mrejecting\033[0m",
                           hs.role == NodeRole::MASTER ? "MASTER" : "SLAVE");
                 });


      configuration_query.received
        .connect([&](_Node& _node)
                 { _node.send(_node.configuration); });


      module_control.sent
        .connect([&](_Node&, const ModuleControl&)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;33mModule-control sent.\033[0m\n", THREAD_ID);
                 });

      module_control.received
        .connect([&](_Node&, const ModuleControl& ctl)
                 {
                   fprintf(stderr, "[0x%0x] \033[1;33mModule-control received.\033[0m\n", THREAD_ID);
                   for ( const ModuleControl::ValuePair& pair : ctl.values )
                     {
                       fprintf(stderr, "    -> %s\n", pair.input->name);
                     }
                 });
    }  






    template < typename _Node >
    void
    MessageDispatcher<_Node>::dispatch(Message&& message, MessageDirection direction) throw ( std::runtime_error )
    {
      assert(m_node != NULL);
      switch ( message.header.type )
	{
	case MessageType::HANDSHAKE:
	  detail::call_handler(*m_node, std::move(message), direction, handshake);
	  break;

	case MessageType::HANDSHAKE_RESPONSE:
	  detail::call_handler(*m_node, std::move(message), direction, handshake_response);
	  break;

	case MessageType::SYNC:
	  detail::call_handler(*m_node, direction, sync);
	  break;

	case MessageType::ERROR:
	  throw std::runtime_error("Unexpected unimplemented message type ERROR");
	  break;

	case MessageType::CONFIGURATION_QUERY:
	  detail::call_handler(*m_node, direction, configuration_query);
	  break;

	case MessageType::CONFIGURATION_RESPONSE:
	  detail::call_handler(*m_node, std::move(message), direction, configuration_response);
	  break;

	case MessageType::SENSOR_DATA:
	  throw std::runtime_error("not yet implemented");
	  break;

	case MessageType::MODULE_CONTROL:
          detail::call_handler<_Node, ModuleControl, Configuration>(*m_node, std::move(message), direction, module_control, m_node->configuration);
	  break;
	}
    }

    /* MessageDispatcher&
     * MessageDispatcher::operator = (MessageDispatcher&& md)
     * {
     *   m_node = md.m_node;
     *   handshake_handler = md.handshake_handler;
     *   handshake_response_handler = md.handshake_response_handler;
     *   sync_handler = md.sync_handler;
     *   configuration_query_handler = md.configuration_query_handler;
     *   configuration_response_handler = md.configuration_response_handler;
     *   module_control_handler = md.module_control_handler;
     *   return *this;
     * } */


  }
}
