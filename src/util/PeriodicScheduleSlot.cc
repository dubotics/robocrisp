#include <crisp/util/PeriodicScheduleSlot.hh>
#include <crisp/util/Scheduler.hh>

namespace crisp
{
  namespace util
  {

    PeriodicScheduleSlot::PeriodicScheduleSlot(Scheduler& scheduler,
                                               PeriodicScheduleSlot::Duration interval)
      : m_scheduler ( scheduler ),
        m_timer ( new Timer(m_scheduler.get_io_service()) ),
        m_interval ( interval ),
        m_actions ( )
    {}

    PeriodicScheduleSlot::PeriodicScheduleSlot(PeriodicScheduleSlot&& slot)
      : m_scheduler ( slot.m_scheduler ),
        m_timer ( std::move(slot.m_timer) ),
        m_interval ( std::move(slot.m_interval) ),
        m_actions ( std::move(slot.m_actions) )
    {
      slot.m_timer = nullptr;
    }

    PeriodicScheduleSlot::~PeriodicScheduleSlot()
    {
      if ( m_timer )
        delete m_timer;
      m_timer = nullptr;
    }

    Scheduler&
    PeriodicScheduleSlot::get_scheduler() const
    {
      return m_scheduler;
    }


    bool
    PeriodicScheduleSlot::empty()
    { return m_actions.empty(); }


    std::weak_ptr<PeriodicAction>
    PeriodicScheduleSlot::push(const PeriodicAction& action)
    {
      bool was_empty ( m_actions.empty() );
      m_actions.push_front(std::make_shared<PeriodicAction>(action));

      if ( was_empty )
        reset_timer();

      return m_actions.front();
    }


    std::weak_ptr<PeriodicAction>
    PeriodicScheduleSlot::push(PeriodicAction&& action)
    {
      bool was_empty ( m_actions.empty() );
      m_actions.push_front(std::make_shared<PeriodicAction>(std::move(action)));

      if ( was_empty )
        reset_timer();

      return m_actions.front();
    }

    void
    PeriodicScheduleSlot::remove(const std::weak_ptr<PeriodicAction> action)
    {
      if ( ! action.expired() )
        m_actions.remove(action.lock());

      if ( m_actions.empty() )
        m_timer->cancel();

    }

    void
    PeriodicScheduleSlot::timer_expiry_handler(const boost::system::error_code& ec)
    {
      if ( ! ec )
        reset_timer();
      else if ( ec != boost::asio::error::operation_aborted )
        fprintf(stderr, "timer error: %s\n", ec.message().c_str());
    }

    void
    PeriodicScheduleSlot::reset_timer()
    {
      m_timer->expires_from_now(m_interval);

      /* Set up the action callbacks for this run.  Note that we're setting up
         only the actions' timer-expiry handlers -- PeriodicScheduleSlot doesn't
         call the user-defined callbacks itself.
       */
      for ( const std::shared_ptr<PeriodicAction>& action : m_actions )
        if ( action->active )
          m_timer->async_wait(std::bind(&PeriodicAction::timer_expiry_handler,
                                        action.get(), std::placeholders::_1));

      /* Set up the slot's expiry handler so we can reset the timer again.  */
      m_timer->async_wait(std::bind(&PeriodicScheduleSlot::timer_expiry_handler,
                                    this, std::placeholders::_1));
    }
  }
}
