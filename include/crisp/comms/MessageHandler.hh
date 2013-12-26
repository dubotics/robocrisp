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
    dereference_and_call(_Function handler, _Node& node, const std::shared_ptr<_Tp>& body,
                         MessageHandlerSignal<_Node, _Tp>& source)
    {
      /* Here we create a copy of the shared_ptr (to which we were passed a
         _reference_) so that the managed memory isn't freed while the handler
         function is still running. */
      std::shared_ptr<_Tp> ptr ( body );
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
     * objects using `std::shared_ptr` and `std::unordered_set` for argument
     * persistance.
     */
    /* NOTE that an attempt _has_ been made to avoid the need for keeping a
       local set of `shared_ptr`s by inheriting from a signal that passes the
       body `shared_ptr` by value rather than by reference.  For reasons that
       aren't entirely clear to the author, that resulted in `NULL`
       shared-pointer objects being passed to `dereference_and_call`. */
    template < typename _Node, typename _Body >
    class MessageHandlerSignal : public crisp::util::Signal<void(_Node&, const std::shared_ptr<_Body>&)>
    {
    public:
      typedef crisp::util::Signal<void(_Node&, const std::shared_ptr<_Body>&)> Base;
      typedef std::function<void(_Node&,const _Body&)> Function;

      using Connection = typename Base::Connection;
      using Base::Base;         /* inherit constructors */

    private:
      typedef std::unordered_set<std::shared_ptr<_Body> > BodySet;
      BodySet m_live_bodies;
      std::mutex m_live_bodies_mutex;

    public:
      MessageHandlerSignal()
        : Base(),
          m_live_bodies ( ),
          m_live_bodies_mutex ( )
      {}

      MessageHandlerSignal(const MessageHandlerSignal& sig)
        : Base(sig),
          m_live_bodies ( sig.m_live_bodies ),
          m_live_bodies_mutex ( )
      {}

      MessageHandlerSignal&
      operator =(const MessageHandlerSignal& sig)
      {
        Base::operator=(sig);
        m_live_bodies = sig.m_live_bodies;
        return *this;
      }


      Connection
      connect(Function&& function)
      {
        using namespace std::placeholders;
        return Base::connect(std::bind(dereference_and_call<_Node, _Body, Function>,
                                       function, _1, _2, std::ref(*this)));
      }


      template < typename... Args >
      void emit(_Node& node, const Message& m, Args... args)
      {
        std::shared_ptr<_Body> ptr ( std::make_shared<_Body>(m.as<_Body>(args...)) );

        typename BodySet::iterator iter;

        {
          std::unique_lock<std::mutex> lock ( m_live_bodies_mutex );
          iter = m_live_bodies.insert(ptr).first;
        }

        this->Base::emit(node, *iter);

        using namespace std::placeholders;

        /* Set a timer, and check if we can free the message-body data
           (`release_on_timeout`) when it expires.  */
        node.scheduler.set_timer(std::chrono::seconds(MESSAGE_HANDLER_SIGNAL_FREE_DELAY),
                                 std::bind(&MessageHandlerSignal::release_on_timeout,
                                           this, _1, std::cref(*iter)));
      }

    protected:
      /** Timer-expiry handler for the `set_timer` call in `emit`. */
      void
      release_on_timeout(crisp::util::ScheduledAction& action, const std::shared_ptr<_Body>& ptr)
      {
        if ( ptr.unique() )
          {
            std::unique_lock<std::mutex> lock ( m_live_bodies_mutex );
            m_live_bodies.erase(ptr);
          }
        else
          action.reset(std::chrono::seconds(MESSAGE_HANDLER_SIGNAL_FREE_DELAY));
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
