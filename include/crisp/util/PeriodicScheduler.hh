#ifndef crisp_util_PeriodicScheduler_hh
#define crisp_util_PeriodicScheduler_hh 1

#include <chrono>
#include <set>
#include <unordered_map>
#include <boost/asio/steady_timer.hpp>
#include <crisp/util/PeriodicAction.hh>
#include <crisp/util/PeriodicScheduleSlot.hh>


/* Need to define std::hash<std::chrono::microseconds> for PeriodicScheduler,
   which uses a map keyed on std::chrono::microseconds. I'd expect the standard
   library to implement this, but it apparently doesn't. */
namespace std {
  template <> struct hash<typename crisp::util::PeriodicScheduleSlot::Duration> {
    inline constexpr size_t operator()(const typename crisp::util::PeriodicScheduleSlot::Duration& d) const noexcept
    { return d.count(); }
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
       *  the constant...
       *
       *
       */
      inline constexpr PeriodicScheduleSlot::Duration
      operator "" _Hz(unsigned long long int n) {
        return PeriodicScheduleSlot::Duration(1000000000 / n);
      }
    }

    /** Runs user-defined functions at user-defined regular intervals.
     * PeriodicScheduler manages scheduled actions in a threaded fashion, such
     * that a function's execution time does not affect the scheduler's timing.
     */
    class PeriodicScheduler
    {
    public:
      typedef PeriodicScheduleSlot Slot;
      typedef PeriodicAction Action;
      typedef typename Slot::Duration Duration;

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
      schedule(Duration interval, Action::Function function);


      /** Reschedule a previously-created Action by moving it into a slot that
       * matches the given interval.
       *
       * @param action The action to be rescheduled.
       *
       * @param interval New interval at which the action should be invoked.
       */
      void
      reschedule(Duration interval, Action& action);


      /** Fetch a reference to the Boost.Asio `io_service` object used by this
       *  scheduler.
       *
       * @return The scheduler's `io_service` reference.
       */
      boost::asio::io_service&
      get_io_service();


    protected:
      /** A mapping from slot-interval to slot.  */
      typedef std::unordered_map<Duration,Slot> SlotMap;

      /** Reference to the io_service used by this scheduler. */
      boost::asio::io_service& m_io_service;

      /** Slots active on this scheduler. */
      SlotMap m_slots;
    };
  }
}

#endif	/* crisp_util_PeriodicScheduler_hh */
