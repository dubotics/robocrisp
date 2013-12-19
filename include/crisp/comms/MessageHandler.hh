#ifndef MessageHandler_hh
#define MessageHandler_hh 1

#include <type_traits>
#include <functional>

namespace crisp
{
  namespace comms
  {
    struct Message;

    /** Interface template for message handlers. */
    template < typename _Node, typename _Body = void , typename _Enable = _Body>
    struct MessageHandler;

    template < typename _Node, typename _Body >
    struct
    MessageHandler< _Node, _Body, typename std::enable_if<std::is_void<_Body>::value, void>::type>
    {
      typedef std::function<void(_Node&)> HandlerFunction;


      HandlerFunction
        sent,
        received;
    };


    template < typename _Node, typename _Body>
    struct
    MessageHandler< _Node, _Body, typename std::enable_if<std::is_same<_Body, crisp::comms::Message>::value, _Body>::type>
    {
      typedef std::function<void(_Node&,const Message&)> HandlerFunction;
      HandlerFunction
        sent,
        received;
    };


    template < typename _Node, typename _Body >
    struct
    MessageHandler<  _Node, _Body, typename std::enable_if<!(std::is_void<_Body>::value||std::is_same<_Body, Message>::value), _Body>::type>
    {
      typedef std::function<void(_Node&,const _Body&)> HandlerFunction;
      HandlerFunction
        sent,
        received;
    };

  }
}

#endif	/* MessageHandler_hh */
