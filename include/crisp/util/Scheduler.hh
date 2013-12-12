#ifndef crisp_util_Scheduler_hh
#define crisp_util_Scheduler_hh 1

#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <boost/asio/steady_timer.hpp>
#include <crisp/util/ScheduledAction.hh>
#include <crisp/util/PeriodicAction.hh>
#include <crisp/util/PeriodicScheduleSlot.hh>


/* Need to define several std::hash<...> implementations for Scheduler,
   which uses std::unordered_map and std::unordered_set keyed on these types. */
namespace std
{
  template <> struct hash<typename crisp::util::PeriodicScheduleSlot::Duration>
  {
    inline constexpr size_t operator()(const typename crisp::util::PeriodicScheduleSlot::Duration& d) const noexcept
    { return d.count(); }
  };


  template <> struct hash<typename crisp::util::ScheduledAction::Function>
  {
    inline std::size_t operator()(const typename crisp::util::ScheduledAction::Function& f) const noexcept
    { return reinterpret_cast<std::size_t>(f.target<void>()); }
  };


  template <> struct hash<crisp::util::ScheduledAction>
  {
    inline std::size_t operator()(const crisp::util::ScheduledAction& a) const noexcept
    { return
        std::hash<typename crisp::util::ScheduledAction::Function>()(a.m_function)
        ^ (std::hash<typename crisp::util::ScheduledAction::Duration>()(a.m_timer->expires_from_now()) << 1);
    };
  };
}

namespace crisp
{
  namespace util
  {
    namespace literals
    {
      /** User-defined literal for conversion from rate to interval.  Note that
       *  this is semantically *wrong* because we're returning the reciprocal of
       *  the constant.  It is, however, very convenient.
       */
      inline constexpr PeriodicScheduleSlot::Duration
      operator "" _Hz(unsigned long long int n) {
        return PeriodicScheduleSlot::Duration(1000000000 / n);
      }
    }

    /** Runs user-defined functions at regular intervals or after specified timeouts.
     * Scheduler manages scheduled actions such that a function's execution time does
     * not affect the scheduler's timing.
     */
    class Scheduler
    {
    public:
      typedef PeriodicScheduleSlot Slot;

      /** Construct a Scheduler that uses the given `io_service`.
       *
       * @param io_service Boost.Asio `io_service` to use.
       */
      Scheduler(boost::asio::io_service& io_service);

      virtual ~Scheduler();      /**< Destructor. */


      /** Schedule a function to be called after a specified duration.
       *
       * @param duration Time after which the function should be called.
       *
       * @param function Function to call.
       *
       * @return Reference to the scheduled action object.
       */
      ScheduledAction&
      set_timer(ScheduledAction::Duration duration, ScheduledAction::Function function);


      /** Schedule a function to be called at regular intervals.
       *
       * @param interval Interval at which the function is to be called.
       *
       * @param function Function to be called.
       *
       * @return Reference to the Action object that represents the scheduled
       *     function call.
       */
      PeriodicAction&
      schedule(Slot::Duration interval, PeriodicAction::Function function);


      /** Remove a previously-scheduled action.
       *
       * @param action Action to be canceled and removed.
       */
      void remove(const ScheduledAction& action);

      /** Fetch a reference to the Boost.Asio `io_service` object used by this
       *  scheduler.
       *
       * @return The scheduler's `io_service` reference.
       */
      boost::asio::io_service&
      get_io_service();


    protected:
      /** A mapping from slot-interval to slot.  */
      typedef std::unordered_map<Slot::Duration,Slot> SlotMap;

      typedef std::unordered_set<ScheduledAction*> ActionSet;

      /** Reference to the io_service used by this scheduler. */
      boost::asio::io_service& m_io_service;

      /** Slots active on this scheduler. */
      SlotMap m_slots;

      /** One-time actions active on this scheduler. */
      ActionSet m_actions;
    };
  }
}

#endif	/* crisp_util_Scheduler_hh */
