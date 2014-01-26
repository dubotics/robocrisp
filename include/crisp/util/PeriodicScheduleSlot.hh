#ifndef crisp_util_PeriodicScheduleSlot_hh
#define crisp_util_PeriodicScheduleSlot_hh 1

#define BOOST_ASIO_HAS_STD_CHRONO 1
#include <crisp/util/PeriodicAction.hh>
#include <boost/asio/steady_timer.hpp>
#include <forward_list> 
#include <thread>
#include <memory>

namespace crisp
{
  namespace util
  {
    /* forward declaration */
    class Scheduler;

    /** Producer of a regularly-occurring signal.  A slot sets up a timer, and
     * when that timer expires launches a thread to invoke all actions
     * assigned to it; the timer is then reset and the process starts again.
     *
     * `Slot` manages its assigned actions through member functions `push` (or
     * `emplace`) and `delete`.  If a previously-non-empty slot becomes empty
     * at any time, it will cancel its timer and reset it only when it becomes
     * non-empty again.
     */
    class PeriodicScheduleSlot
    {
    public:
      /** Timer type used. */
      typedef boost::asio::steady_timer Timer;

      /** Duration type used. */
      typedef typename Timer::duration Duration;


    protected:
      /** A set of actions to be performed   */
      typedef std::forward_list<std::shared_ptr<PeriodicAction> > ActionList;

      /** Scheduler that owns this slot. */
      Scheduler& m_scheduler;

      /** The slot's timer. */
      Timer* m_timer;

      /** Interval at which the slot's handler runs. */
      Duration m_interval;

      /** Actions assigned to this slot. */
      ActionList m_actions;

      /** Callback used to handle timer events.  */
      void
      timer_expiry_handler(const boost::system::error_code& ec);

      void
      reset_timer();

    public:
      /** Constructor.
       *
       * @param scheduler Scheduler object that created this slot.
       *
       * @param interval Interval at which the slot should activate.
       */
      PeriodicScheduleSlot(Scheduler& scheduler, Duration interval);

      /** Move constructor.
       *
       * @param slot The slot from which to move-construct.
       */
      PeriodicScheduleSlot(PeriodicScheduleSlot&& slot);


      /** Destructor. */
      virtual ~PeriodicScheduleSlot();


      /** Get a reference to the scheduler that owns this slot.
       */
      Scheduler&
      get_scheduler() const;


      /** Check if the slot is empty (has no assigned actions). */
      bool
      empty();

      /** Add an action to the slot (via copy-construct). */
      std::weak_ptr<PeriodicAction>
      push(const PeriodicAction& action);

      /** Add an action to the slot (via move-construct).
       */
      std::weak_ptr<PeriodicAction>
      push(PeriodicAction&& action);


      /** Emplace a new assigned action. */
      template < typename... Args >
      std::weak_ptr<PeriodicAction>
      emplace(Args... args);

    protected:
      friend class Scheduler;

      /** Remove an action from the slot.
       *
       * @param action The action to be removed.
       */
      void
      remove(std::weak_ptr<PeriodicAction> action);
    };
  }
}

#include <crisp/util/bits/PeriodicScheduleSlot.tcc>

#endif	/* crisp_util_PeriodicScheduleSlot_hh */
