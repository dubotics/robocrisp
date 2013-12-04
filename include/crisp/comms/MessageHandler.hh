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
    template < typename _Node, typename _BodyType = void , typename _Enable = _BodyType>
    struct MessageHandler;

    template < typename _Node, typename _BodyType >
    struct
    MessageHandler< _Node, _BodyType, typename std::enable_if<std::is_void<_BodyType>::value, void>::type>
    {
      typedef std::function<void(_Node&)> HandlerFunction;


      HandlerFunction
        sent,
        received;
    };


    template < typename _Node, typename _BodyType>
    struct
    MessageHandler< _Node, _BodyType, typename std::enable_if<std::is_same<_BodyType, crisp::comms::Message>::value, _BodyType>::type>
    {
      typedef std::function<void(_Node&,const Message&)> HandlerFunction;
      HandlerFunction
        sent,
        received;
    };


    template < typename _Node, typename _BodyType >
    struct
    MessageHandler<  _Node, _BodyType, typename std::enable_if<!(std::is_void<_BodyType>::value||std::is_same<_BodyType, Message>::value), _BodyType>::type>
    {
      typedef std::function<void(_Node&,const _BodyType&)> HandlerFunction;
      HandlerFunction
        sent,
        received;
    };

  }
}

#endif	/* MessageHandler_hh */
