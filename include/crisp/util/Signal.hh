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
    /* This declaration lets us declare Signal using a function-like template
     * parameter.
     */
    template < typename >
    class Signal;

    /** Light-weight event-source and callback handler.
     *
     * Unlike Boost.Signals (1 or 2), Signal provides no "combiner" interface,
     * i.e. handler return values are ignored.
     * 
     */
    template < typename Return, typename... Args >
    class Signal<Return(Args...)>
    {
    public:
      /** SignalAction type used by the signal. */
      typedef SignalAction<Return(Args...)> Action;

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
      
      /** Add a function to be called on signal emission.
       *
       * @param function Function to call.
       *
       * @return A weak pointer reference to the internal object used to wrap
       *     the supplied function.  This may be used to disconnect the handler
       *     from the signal.
       */
      std::weak_ptr<Action>
      connect(Function&& function);

      /** Remove a previously-added callback.
       *
       * @param action Weak pointer to the action to remove.
       */
      void remove(const std::weak_ptr<Action>& action);

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
