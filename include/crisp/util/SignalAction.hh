/** @file
 *
 * Defines a generic user-callback management utility class for use with
 * crisp::util::Signal a light-weight signal emitter for asynchronous event
 * notification and callback invocation.
 */
#ifndef crisp_util_SignalAction_hh
#define crisp_util_SignalAction_hh 1

#include <functional>
#include <memory>

namespace crisp
{
  namespace util
  {
    /* Forward declaration. */
    template < typename >
    class Signal;

    /* This declaration lets us declare SignalAction using a function-like
     * template parameter.
     */
    template < typename >
    class SignalAction;

    /** User-callback manager for crisp::util::Signal.  A pointer to a
     *  SignalAction can be used to permanently disable (via `cancel`) the
     *  action's invocation by its parent signal.
     */
    template < typename Return, typename... Args >
    class SignalAction<Return(Args...)>
      : public std::enable_shared_from_this<SignalAction<Return(Args...)> >
    {
    public:
      typedef std::function<Return(Args...)> Function;
      typedef crisp::util::Signal<Return(Args...)> Signal;

    private:
      friend class crisp::util::Signal<Return(Args...)>;
      friend class std::hash<SignalAction>;

      /** A reference to the signal that owns this action. */
      Signal& m_signal;

      /** Function called by this action. */
      Function m_function;

    public:
      /** Initialize a SignalAction.
       *
       * @param signal Parent signal object.
       *
       * @param function User callback.
       */
      SignalAction(Signal& signal, Function&& function);

      /** Move constructor. */
      SignalAction(SignalAction&& action) = default;

      /** Virtual destructor provided to enable polymorphic use of derived
       *  types.
       */
      virtual ~SignalAction();

      /** Fetch a shared pointer to this object. */
      std::shared_ptr<SignalAction>
      get_pointer();

      /** Remove this action from the signal's action list. */
      void disconnect();
    };
  }
}

namespace std
{
  template <typename Return, typename... Args>
  struct hash<crisp::util::SignalAction<Return(Args...)> >
  {
    typedef crisp::util::SignalAction<Return(Args...)> SignalAction;
    inline std::size_t operator()(const SignalAction& a) const noexcept;
  };
}

#include <crisp/util/bits/SignalAction.tcc>


#endif  /* crisp_util_SignalAction_hh */
