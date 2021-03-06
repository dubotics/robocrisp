/** @file
 *
 * Defines a light-weight signal class for asynchronous event notification and
 * callback invocation.
 */
#ifndef crisp_util_Signal_hh
#define crisp_util_Signal_hh 1

#include <functional>
#include <mutex>
#include <unordered_set>
#include <boost/asio/io_service.hpp>
#include <crisp/util/SignalAction.hh>
#include <crisp/util/SArray.hh>

namespace crisp
{
  namespace util
  {
#ifndef CRISP_GENERATING_DOCUMENTATION
    /* This declaration lets us declare Signal using a function-like template
     * parameter.
     */
    /**@internal */
    template < typename >
    class Signal;
#endif

    /** Light-weight event-source and callback handler.  A signal object may be
     * used to invoke a user-configurable set of event handlers, each of which
     * will receive the arguments passed to the signal's `emit` method.
     *
     * Unlike Boost.Signals (1 or 2), Signal provides no "combiner" interface,
     * i.e. handler return values are ignored.
     *
     * @tparam Return Return-type for handler functions.
     *
     * @tparam Args Argument-types for arguments to handler functions.
     * 
     */
    template < typename Return, typename... Args >
    class Signal<Return(Args...)>
    {
    public:
      /** SignalAction type used by the signal. */
      typedef SignalAction<Return(Args...)> Action;

      /** Reference-to-action type used by the signal. */
      typedef std::weak_ptr<Action> Connection;

      /** Alias of Action::Function for brevity's sake. */
      typedef typename Action::Function Function;

    private:
      typedef std::unordered_set<std::shared_ptr<Action> > ActionSet;

      /** Actions assigned to this signal. */
      ActionSet m_actions;

      /** If non-null, a pointer to the Boost.Asio `io_service` passed to the
          object's constructor and to be used for callback invocation.  */
      boost::asio::io_service* m_io_service;

      /** Mutex used to synchronize modifications of the action-set across
          threads.  */
      std::mutex m_mutex;

    public:
      /** Default constructor.  Sets up the signal to use blocking callback
       *  invocation.
       */
      Signal();

      /** Move constructor. */
      Signal(Signal&& sig);

      /** Copy constructor. */
      Signal(const Signal& sig);

      Signal&
      operator =(const Signal& sig);

      /** Set up a signal to invoke callbacks via a Boost.Asio `io_service`.
       *
       * @param service The io_service to use.  Callbacks will be invoked via
       *     the service's `post` method.
       */
      Signal(boost::asio::io_service& service);

      /** Virtual destructor provided to enable polymorphic use of derived
       *  types.
       */
      virtual ~Signal();
      
      /** Set a signal to invoke callbacks via the given Boost.Asio
       *  `io_service`.
       *
       * @param service Reference to the io_service to which signal handlers
       *     should be posted.
       */
      void set_io_service(boost::asio::io_service& service);


      /** Set the signal to invoke callbacks in a blocking manner.  If an
       * `io_service` was previously set on the signal (via constructor or
       *  `set_io_service`), the internal pointer will be cleared.
       */
      void clear_io_service();

      /** Add a function to be called on signal emission.
       *
       * @param function Function to call.
       *
       * @return A weak pointer reference to the internal object used to wrap
       *     the supplied function.  This may be used to disconnect the handler
       *     from the signal.
       */
      Connection
      connect(Function function);

      /** Remove a previously-added callback.
       *
       * @param action Weak pointer to the action to remove.
       */
      void remove(const Connection& action);

      /** Disconnect _all_ previously-connected callbacks.
       */
      void clear();

      /** Emit the signal and invoke callbacks.
       *
       * @param args Arguments to be passed to the connected callbacks.
       */
      void emit(Args... args);
    };
  }
}

#include <crisp/util/bits/Signal.tcc>

#endif  /* crisp_util_Signal_hh */
