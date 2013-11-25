/* -*- C++ -*- */
#include "MessageDispatcher.hh"
#include "Handshake.hh"
#include "Configuration.hh"
#include "ModuleControl.hh"

using namespace Robot;

template < typename _BodyType, typename _PortType >
static typename std::enable_if<std::is_void<_BodyType>::value, void>::type
call_handler(ProtocolNode<_PortType>& node,
	     MessageDirection direction,
	     MessageHandler<_BodyType, _PortType>& handler)
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


template < typename _BodyType, typename _PortType >
static typename std::enable_if<std::is_same<_BodyType, Message>::value, void>::type
call_handler(ProtocolNode<_PortType>& node,
	     const Message& m, MessageDirection direction,
	     MessageHandler<_BodyType, _PortType>& handler)
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


template < typename _BodyType, typename _PortType >
static typename std::enable_if<!(std::is_void<_BodyType>::value||std::is_same<_BodyType, Message>::value), void>::type
call_handler(ProtocolNode<_PortType>& node,
	     const Message& m, MessageDirection direction,
	     MessageHandler<_BodyType, _PortType>& handler)
{
  _BodyType&& value ( m.as<_BodyType>() );

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


namespace Robot
{
  template < typename _PortType >
  MessageDispatcher<_PortType>::MessageDispatcher(ProtocolNode<_PortType>& node)
    : m_node ( &node ),
      handshake ( ),
      handshake_response ( ),
      sync ( ),
      configuration_query ( ),
      configuration_response ( ),
      module_control ( )
  {}


  template < typename _PortType >
  MessageDispatcher<_PortType>::~MessageDispatcher()
  {}

  template < typename _PortType >
  void
  MessageDispatcher<_PortType>::dispatch(const Message& message, MessageDirection direction) throw ( std::runtime_error )
  {
    switch ( message.type )
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
	call_handler(*m_node, message, direction, module_control);
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
