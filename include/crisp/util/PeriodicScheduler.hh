#ifndef crisp_util_PeriodicScheduler_hh
#define crisp_util_PeriodicScheduler_hh 1

#include <chrono>
#include <boost/asio/steady_timer.hpp>

namespace crisp
{
  namespace util
  {
    /** Runs user-defined functions at user-defined regular intervals.
     * PeriodicScheduler manages scheduled actions in a threaded fashion, such
     * that a function's execution time does not affect the scheduler's timing.
     */
    class PeriodicScheduler
    {
    public:
      /** A scheduled action.  `Action` is used to manage a scheduled action by
	  rescheduling, cancelling, or temporarily disabling it.  */
      struct Action
      {
	/** Function type for user callbacks.
	 *
	 * @param action The Action object associated with the callback.
	 */
	typedef std::function<void(Action& action)> Function;
	
	Slot* slot;		/**< Pointer to the scheduler slot used by the action. */
	Function function;	/**< User-defined function to be called by the
				   slot. */


	/** Reschedule the action  */
	void reschedule(std::chrono::interval interval);


	/** Temporarily remove the action from its associated slot, preventing
	    it from running. */
	void pause();

	/** Unpause an action previously paused with `pause`. */
	void unpause();

	/** Cancel the action, removing it from its slot. */
	void cancel();
      };

      /** A regularly-occurring signal producer.
       */
      struct Slot
      {
	/** Timer type used. */
	typedef boost::asio::steady_timer Timer;

	
	Timer timer;			/**< The slot's timer. */
	std::chrono::duration interval;	/**< Interval at which the slot's
					   handler runs. */
	std::set<Action> actions;	/**< Actions assigned to this slot. */

	/** Callback used to handle timer events.  */
	void
	timer_expiry_handler(const boost::system::error_code& ec);
      };


    protected:
      boost::asio::io_service m_io_service;
      std::set<Slot> m_slots;
    };
  }
}

#endif	/* crisp_util_PeriodicScheduler_hh */
