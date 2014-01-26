#include <crisp/util/PeriodicAction.hh>
#include <crisp/util/PeriodicScheduleSlot.hh>
#include <crisp/util/Scheduler.hh>

namespace crisp
{
  namespace util
  {

    PeriodicAction::PeriodicAction(PeriodicScheduleSlot* _slot,
                                   PeriodicAction::Function _function)
      : slot ( _slot ),
        function ( _function ),
        active ( true )
    {}

    void
    PeriodicAction::timer_expiry_handler(const boost::system::error_code& error)
    {
      if ( ( !error ) && active )
        function(*this);
    }

    void
    PeriodicAction::pause()
    { active = false; }

    void
    PeriodicAction::unpause()
    { active = true; }

    void
    PeriodicAction::cancel()
    {
      active = false;
      Scheduler& scheduler ( slot->get_scheduler() );
      scheduler.get_io_service()
        .post(std::bind(static_cast<void(Scheduler::*)(std::weak_ptr<PeriodicAction>)>(&Scheduler::remove),
                        &scheduler, get_pointer()));
    }

    bool
    PeriodicAction::operator < (const PeriodicAction& action) const
    {
      return slot < action.slot &&
        function.target<void>() < action.function.target<void>();
    }

    bool
    PeriodicAction::operator == (const PeriodicAction& action) const
    {
      return slot == action.slot &&
        function.target<void>() == action.function.target<void>();
    }
  }
}
