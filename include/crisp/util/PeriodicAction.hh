#ifndef crisp_util_PeriodicAction_hh
#define crisp_util_PeriodicAction_hh 1

#ifndef __AVR__
# include <functional>
# include <boost/system/error_code.hpp>
# include <memory>
#endif

namespace crisp
{
  namespace util
  {

    class PeriodicScheduleSlot;

    /** An action scheduled on a PeriodicScheduleSlot.  `PeriodicAction` is used to
        manage a scheduled action by rescheduling, cancelling, or temporarily
        disabling it.  */
    struct PeriodicAction
#ifndef __AVR__
      : public std::enable_shared_from_this<PeriodicAction>
#endif
    {
      /** Function type for user callbacks.
       *
       * @param action The Action object associated with the callback.
       */
#ifndef __AVR__
      typedef std::function<void(PeriodicAction& action)> Function;
#else
      typedef void(*Function)(PeriodicAction& action);
#endif

#ifndef __AVR__
      typedef std::weak_ptr<PeriodicAction> Pointer;
      typedef const std::weak_ptr<PeriodicAction> ConstPointer;
#else
      typedef PeriodicAction* Pointer;
      typedef const PeriodicAction* ConstPointer;
#endif
	
      PeriodicScheduleSlot* slot; /**< Pointer to the schedule slot that
                                     contains this action.  This is used by
                                     `pause`, `unpause`, and `cancel`. */

      Function function;	/**< User-defined function to be called by the
				   slot. */

      bool active;              /**< When `true`, the slot will continue to
                                   enqueue timer waits on behalf of the action. */

      PeriodicAction(PeriodicScheduleSlot* _slot,
                     Function _function);

#ifndef __AVR__
      inline std::weak_ptr<PeriodicAction>
      get_pointer() { return shared_from_this(); }
#endif

      /** Handler invoked by the slot's timer when it expires or is cancelled.
       * This function calls the user function whenever called with an empty
       * error object.
       *
       * @param error An error code that specifies why the function was called.
       */
#ifndef __AVR__
      void
      timer_expiry_handler(const boost::system::error_code& error);
#else
      void
      timer_expiry_handler();
#endif


      bool
      operator < (const PeriodicAction& action) const;

      bool
      operator == (const PeriodicAction& action) const;


      /** Unset the `active` flag, preventing the action's associated slot from
          enqueuing timer waits on the action. */
      void pause();

      /** Unpause an action previously paused with `pause`. */
      void unpause();

      /** Cancel the action, removing it from its slot.
       *
       * @post The PeriodicAction object is destroyed by the slot that owns it, and the
       *     reference used to access it is no longer valid.
       */
      void cancel();
    };

  }
}

#endif	/* crisp_util_PeriodicAction_hh */
