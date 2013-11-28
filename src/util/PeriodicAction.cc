#include <crisp/util/PeriodicAction.hh>
#include <crisp/util/PeriodicScheduleSlot.hh>

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
      if ( ( !error ) && action )
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
      slot->remove(*this);
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
