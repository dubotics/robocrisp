/** @file
 *
 * Defines several classes used to manage user-defined handlers for message
 * send/receive events.
 */
#ifndef MessageHandler_hh
#define MessageHandler_hh 1

#include <type_traits>
#include <crisp/util/Signal.hh>
#include <unordered_set>
#include <crisp/util/Scheduler.hh>

namespace crisp
{
  namespace comms
  {
    struct Message;

#ifndef CRISP_GENERATING_DOCUMENTATION
    template < typename _Node, typename _Body >
    struct MessageHandlerSignal;
#endif



    /** Helper/adapter to provide registered message-callbacks with references,
     * rather than smart-pointers, to the received messages and/or decoded objects.
     */
    template < typename _Node, typename _Tp, typename _Function >
    static void
    dereference_and_call(_Function handler, _Node& node, const std::shared_ptr<_Tp> body,
                         MessageHandlerSignal<_Node, _Tp>& source)
    {
      handler(node, *body);
    }

    /** @begin Message-handler signal types.
     *
     * These signal specializations are used to hide the `shared_ptr` type
     * needed for proper Message-object lifetime management from the handler
     * functions.
     *
     * @{
     */

    /** Message-handler signal type for handlers that take no arguments beyond
        the associated node object. */
    template < typename _Node >
    struct MessageHandlerSignal<_Node, void> : public crisp::util::Signal<void(_Node&)>
    {
      typedef crisp::util::Signal<void(_Node&)> Base;
      using Base::Base;
    };


    /** Message-handler signal type for handlers that take a reference to the
     *  message or decoded message body as an argument.
     *
     * During the execution of a signal's user-defined callbacks, the referent
     * of any pointer-like argument to `emit` must not be deallocated or
     * otherwise destroyed, i.e. those arguments must remain valid pointers or
     * references.
     *
     * This poses no problem whatsoever when synchronous signal-emission is used
     * because `emit` runs all user callbacks within the current thread -- thus
     * its arguments *necessarily* remain in-scope the entire time.  When using
     * asynchronous signal-emission, however, it's quite possible that `emit`
     * will return before even the first callback has been called.
     *
     * This class implements a wrapper around `Signal` that manages message-body
     * objects by translating through an intermediary callback that takes a
     * `std::shared_ptr` argument.
     */
    template < typename _Node, typename _Body >
    class MessageHandlerSignal : public crisp::util::Signal<void(_Node&, const std::shared_ptr<_Body>)>
    {
    public:
      typedef crisp::util::Signal<void(_Node&, const std::shared_ptr<_Body>)> Base;
      typedef std::function<void(_Node&,const _Body&)> Function;

      using Base::Base;         /* inherit constructors */
      using Connection = typename Base::Connection;

      Connection
      connect(Function&& function)
      {
        using namespace std::placeholders;
        return Base::connect(std::bind(dereference_and_call<_Node, _Body, Function>,
                                       function, _1, _2, std::ref(*this)));
      }

    };
    /**@}*/


#ifndef CRISP_GENERATING_DOCUMENTATION
    /** Interface template for message handlers. */
    template < typename... _Args >
    struct MessageHandler;
#endif

    /** Handler type for message callbacks that are invoked without any kind of body
        argument. */
    template < typename _Node >
    struct MessageHandler<_Node>
    {
      typedef MessageHandlerSignal<_Node, void> HandlerSignal;

      MessageHandler() = default;
      MessageHandler(const MessageHandler& mh)
      : sent ( mh.sent ),
        received ( mh.received )
      {}

      inline
      MessageHandler(boost::asio::io_service& service)
        : sent ( service ),
          received ( service )
      {}

      HandlerSignal
        sent,
        received;
    };

    /** Handler type for handlers invoked with some kind of body argument. */
    template < typename _Node, typename _Body >
    struct MessageHandler<_Node, _Body>
    {
      typedef MessageHandlerSignal<_Node, _Body> HandlerSignal;

      MessageHandler() = default;
      MessageHandler(const MessageHandler& mh)
      : sent ( mh.sent ),
        received ( mh.received )
      {}

      inline
      MessageHandler(boost::asio::io_service& service)
        : sent ( service ),
          received ( service )
      {}

      HandlerSignal
        sent,
        received;
    };
  }
}

#endif	/* MessageHandler_hh */
