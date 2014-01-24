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
    PeriodicAction::timer_expiry_handler(
#ifndef __AVR__
                                         const boost::system::error_code& error
#endif
                                         )
    {
#ifndef __AVR__
      if ( ( !error ) && active )
#else
      if ( active )
#endif
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
      slot->get_scheduler().get_io_service().
        post(std::bind(&PeriodicScheduleSlot::remove, slot,
                       get_pointer()));
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
