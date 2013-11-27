#ifndef crisp_util_PeriodicAction_hh
#define crisp_util_PeriodicAction_hh 1

#include <functional>

namespace crisp
{
  namespace util
  {

    class PeriodicScheduleSlot;

    /** An action scheduled on a PeriodicScheduleSlot.  `PeriodicAction` is used to
        manage a scheduled action by rescheduling, cancelling, or temporarily
        disabling it.  */
    struct PeriodicAction
    {
      /** Function type for user callbacks.
       *
       * @param action The Action object associated with the callback.
       */
      typedef std::function<void(PeriodicAction& action)> Function;
	
      PeriodicScheduleSlot* slot; /**< Pointer to the schedule slot that
                                     contains this action.  This is used by
                                     `pause`, `unpause`, and `cancel`. */

      Function function;	/**< User-defined function to be called by the
				   slot. */

      PeriodicAction(PeriodicScheduleSlot* _slot,
                     Function _function);

      bool
      operator < (const PeriodicAction& action) const;

      bool
      operator == (const PeriodicAction& action) const;

      /** Temporarily remove the action from its associated slot, preventing
          it from running. */
      void pause();

      /** Unpause an action previously paused with `pause`. */
      void unpause();

      /** Cancel the action, removing it from its slot. */
      void cancel();
    };

  }
}

#endif	/* crisp_util_PeriodicAction_hh */
