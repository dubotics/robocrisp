#ifndef crisp_util_ScheduledAction_hh
#define crisp_util_ScheduledAction_hh 1

#define BOOST_ASIO_HAS_STD_CHRONO 1
#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <memory>

namespace crisp
{
  namespace util
  {
    /* forward declaration */
    class Scheduler;

    /** Represents an action (function call) scheduled for one-time activation.
     *  ScheduledAction provides a simplified interface for canceling and/or
     *  rescheduling the action.
     */
    class ScheduledAction : public std::enable_shared_from_this<ScheduledAction>
    {
    public:
      typedef boost::asio::steady_timer Timer; /**< Timer type used. */
      typedef Timer::duration Duration;        /**< Timer's duration type. */
      typedef Timer::time_point TimePoint;     /**< Timer's time-point type. */

      /** Function type expected for user callbacks. */
      typedef std::function<void(ScheduledAction&)> Function;

      inline std::weak_ptr<ScheduledAction>
      get_pointer() { return shared_from_this(); }

    private:
      friend class Scheduler;
      friend class std::hash<ScheduledAction>;

#if defined(__LIBCPP_VERSION)
      template < typename _A, typename _B, unsigned>
      friend class std::__1::__libcpp_compressed_pair_imp;
#elif defined(__GNUC__)
      template < typename _Tp >
      friend class __gnu_cxx::new_allocator;
#endif

      ScheduledAction(Scheduler& scheduler, Function function);

      Scheduler& m_scheduler;
      Timer* m_timer;
      Function m_function;

      void timer_expiry_handler(const boost::system::error_code& error);

    public:
      /** Move constructor. */
      ScheduledAction(ScheduledAction&& sa);
      ~ScheduledAction();

      /** Reschedule the timer to expire after the specified duration.  If
       * called while the timer is running, the previously-scheduled invocation
       * of the user callback will be canceled.
       *
       * @param duration Time after which the timer should expire.
       */
      void reset(Duration duration);

      /** Reschedule the timer to expire at the specified time.  If called while
       * the timer is running, the previously-scheduled invocation of the user
       * callback will be canceled.
       *
       * @param when Time at which the timer should expire.
       */
      void reset(TimePoint when);

      /** Cancel the action.  If called before the timer expires, this will
       *  prevent the user callback from being called.
       *
       * @post The ScheduledAction object is destroyed by the originating
       *   Scheduler object and the reference used to access it is no longer
       *   valid.
       */
      void cancel();

      /** Equality operator provided to allow use with std::unordered_set. */
      bool operator ==(const ScheduledAction& sa) const;
    };

  }
}

#endif  /* crisp_util_ScheduledAction_hh */
