/* -*- C++ -*- */
#include <crisp/comms/Handshake.hh>
#include <crisp/comms/Configuration.hh>
#include <crisp/comms/ModuleControl.hh>

using namespace crisp::comms;

template < typename _Node, typename _BodyType >
static typename std::enable_if<std::is_void<_BodyType>::value, void>::type
call_handler(_Node& node,
	     MessageDirection direction,
	     MessageHandler<_Node, _BodyType>& handler)
{
  switch ( direction )
    {
    case MessageDirection::INCOMING:
      if ( handler.received )
	handler.received(node);
      break;
    case MessageDirection::OUTGOING:
      if ( handler.sent )
	handler.sent(node);
      break;
    }
}


template < typename _Node, typename _BodyType >
static typename std::enable_if<std::is_same<_BodyType, Message>::value, void>::type
call_handler(_Node& node,
	     const Message& m, MessageDirection direction,
	     MessageHandler<_Node, _BodyType>& handler)
{
  switch ( direction )
    {
    case MessageDirection::INCOMING:
      if ( handler.received )
	handler.received(node, m);
      break;
    case MessageDirection::OUTGOING:
      if ( handler.sent )
	handler.sent(node, m);
      break;
    }
}


template < typename _Node, typename _BodyType, typename... Args >
static typename std::enable_if<!(std::is_void<_BodyType>::value||std::is_same<_BodyType, Message>::value), void>::type
call_handler(_Node& node,
	     const Message& m, MessageDirection direction,
	     MessageHandler<_Node, _BodyType>& handler,
             const Args&... args)
{
  _BodyType&& value ( m.as<_BodyType>(args...) );

  switch ( direction )
    {
    case MessageDirection::INCOMING:
      if ( handler.received )
	handler.received(node, value);
      break;
    case MessageDirection::OUTGOING:
      if ( handler.sent )
	handler.sent(node, value);
      break;
    }
}


namespace crisp
{
  namespace comms
  {
    template < typename _Node >
    MessageDispatcher<_Node>::MessageDispatcher(_Node& node)
      : m_node ( &node ),
	handshake ( ),
	handshake_response ( ),
	sync ( ),
	configuration_query ( ),
	configuration_response ( ),
	module_control ( )
    {}


    template < typename _Node >
    MessageDispatcher<_Node>::~MessageDispatcher()
    {}

    template < typename _Node >
    void
    MessageDispatcher<_Node>::dispatch(const Message& message, MessageDirection direction) throw ( std::runtime_error )
    {
      switch ( message.header.type )
	{
	case MessageType::HANDSHAKE:
	  call_handler(*m_node, message, direction, handshake);
	  break;

	case MessageType::HANDSHAKE_RESPONSE:
	  call_handler(*m_node, message, direction, handshake_response);
	  break;

	case MessageType::SYNC:
	  call_handler(*m_node, direction, sync);
	  break;

	case MessageType::ERROR:
	  throw std::runtime_error("Unexpected unimplemented message type ERROR");
	  break;

	case MessageType::CONFIGURATION_QUERY:
	  call_handler(*m_node, direction, configuration_query);
	  break;

	case MessageType::CONFIGURATION_RESPONSE:
	  call_handler(*m_node, message, direction, configuration_response);
	  break;

	case MessageType::SENSOR_DATA:
	  throw std::runtime_error("not yet implemented");
	  break;

	case MessageType::MODULE_CONTROL:
	  call_handler(*m_node, message, direction, module_control, m_node->configuration);
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
