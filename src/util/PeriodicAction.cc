#include <crisp/util/PeriodicAction.hh>

namespace crisp
{
  namespace util
  {

    PeriodicAction::PeriodicAction(PeriodicScheduleSlot* _slot,
                                   PeriodicAction::Function _function)
      : slot ( _slot ),
        function ( _function )
    {}

    void
    PeriodicAction::timer_expiry_handler(const boost::system::error_code& error)
    {
      if ( ! error )
        function(*this);
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
