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
