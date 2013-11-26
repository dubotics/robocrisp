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


	/** Reschedule the action by changing the slot used.  */
	void reschedule(std::chrono::interval interval);


	/** Temporarily remove the action from its associated slot, preventing
	    it from running. */
	void pause();

	/** Unpause an action previously paused with `pause`. */
	void unpause();

	/** Cancel the action, removing it from its slot. */
	void cancel();
      };


      /** A regularly-occurring-signal producer.  A slot sets up a timer, and
       * when that timer expires launches a thread to invoke all actions
       * assigned to it; the timer is then reset and the process starts again.
       *
       * `Slot` manages its assigned actions through member functions `push` and
       * `delete`.  If a previously-non-empty slot becomes empty at any time, it
       * will cancel its timer and reset it only when it becomes non-empty
       * again.
       */
      struct Slot
      {
	/** Timer type used. */
	typedef boost::asio::steady_timer Timer;

	/** The slot's timer. */
	Timer timer;

	/** Interval at which the slot's handler runs. */
	std::chrono::duration interval;

	/** Actions assigned to this slot. */
	std::set<Action> actions;

	/** Constructor.
	 *
	 * @param scheduler Scheduler object that created this slot.
	 *
	 * @param _interval Interval at which the slot should activate.
	 */
	Slot(PeriodicScheduler& scheduler, std::chrono::duration _interval);

	/** Callback used to handle timer events.  */
	void
	timer_expiry_handler(const boost::system::error_code& ec);
      };


      /** Construct a PeriodicScheduler that uses the given `io_service`.
       *
       * @param io_service Boost.Asio `io_service` to use.
       */
      PeriodicScheduler(boost::asio::io_service& io_service);

      virtual ~PeriodicScheduler();      /**< Destructor. */


      /** Schedule a function to be called at regular intervals.
       *
       * @param function Function to be called.
       *
       * @param interval Interval at which the function is to be called.
       *
       * @return Reference to the Action object that represents the scheduled
       *     function call.
       */
      Action&
      schedule(Action::Function function, std::chrono::duration interval);


      /** Reschedule a previously-created Action by moving it into a slot that
       * matches the given interval.
       *
       * @param action The action to be rescheduled.
       *
       * @param interval New interval at which the action should be invoked.
       */
      void
      reschedule(Action& action, std::chrono::duration interval);


      /** Fetch a reference to the Boost.Asio `io_service` object used by this
       *  scheduler.
       *
       * @return The scheduler's `io_service` reference.
       */
      boost::asio::io_service&
      get_io_service();


      /** Cancel an entire slot. */
      void
      cancel_slot(Slot& slot);

    protected:
      /** A mapping from slot-interval to slot.  */
      typedef std::map<std::chrono::duration,Slot> SlotMap;

      /** Reference to the io_service used by this scheduler. */
      boost::asio::io_service& m_io_service;

      /** Slots active on this scheduler. */
      SlotMap m_slots;
    };
  }
}

#endif	/* crisp_util_PeriodicScheduler_hh */
