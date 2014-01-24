#ifndef crisp_util_PeriodicScheduleSlot_hh
#define crisp_util_PeriodicScheduleSlot_hh 1

#include <crisp/util/PeriodicAction.hh>

#ifndef __AVR__
# define BOOST_ASIO_HAS_STD_CHRONO 1
# include <boost/asio/steady_timer.hpp>
# include <forward_list>
# include <thread>
# include <memory>
#else
# include <stdint.h>
# include <stddef.h>
# include <crisp/util/StaticArray.hh>
#endif

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
#ifndef __AVR__
      /** Timer type used. */
      typedef boost::asio::steady_timer Timer;

      /** Type used to reference a timer instance. */
      typedef Timer* TimerReference;

      /** Duration type used. */
      typedef typename Timer::duration Duration;
#else
      /** Index-type for the timer interrupt used. */
      typedef uint8_t Timer;

      /** Type used to reference a timer instance. */
      typedef Timer TimerReference;

      /** Type used to store the number of interrupts per slot invocation. */
      typedef size_t Duration;
#endif

    protected:
      /** A set of actions to be performed   */
#ifndef __AVR__
      typedef std::forward_list<std::shared_ptr<PeriodicAction> > ActionList;
#else
      typedef crisp::util::StaticArray<PeriodicAction*,5> ActionList;
#endif

      /** Scheduler that owns this slot. */
      Scheduler& m_scheduler;

      /** The slot's timer. */
      TimerReference m_timer;

      /** Interval at which the slot's handler runs. */
      Duration m_interval;

      /** Actions assigned to this slot. */
      ActionList m_actions;

      /** Callback used to handle timer events.  */
#ifndef __AVR__
      void
      timer_expiry_handler(const boost::system::error_code& error);
#else
      void
      timer_expiry_handler();
#endif

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
      PeriodicAction::Pointer
      push(const PeriodicAction& action);

      /** Add an action to the slot (via move-construct).
       */
      PeriodicAction::Pointer
      push(PeriodicAction&& action);


      /** Emplace a new assigned action. */
      template < typename... Args >
      PeriodicAction::Pointer
      emplace(Args... args);


      /** Remove an action from the slot.
       *
       * @param action The action to be removed.
       */
      void
      remove(const std::weak_ptr<PeriodicAction>& action);
    };
  }
}

#include <crisp/util/bits/PeriodicScheduleSlot.tcc>

#endif	/* crisp_util_PeriodicScheduleSlot_hh */
