#include <crisp/util/Scheduler.hh>
#include <cassert>
namespace crisp
{
  namespace util
  {

    Scheduler::Scheduler(boost::asio::io_service& io_service)
      : m_io_service ( io_service ),
        m_slots ( ),
        m_data_mutex ( )
    {}

    Scheduler::~Scheduler()
    {
    }

    std::weak_ptr<ScheduledAction>
    Scheduler::set_timer(ScheduledAction::Duration duration, ScheduledAction::Function function)
    {
      std::shared_ptr<ScheduledAction> action ( std::make_shared<ScheduledAction>(*this, function) );
      action->reset(duration);

      std::pair<ActionSet::iterator,bool>
        action_pair;

      {
        std::unique_lock<std::mutex> lock ( m_data_mutex );
        action_pair  = m_actions.insert(std::move(action));
      }

      assert(action_pair.second);

      return *(action_pair.first);
    }

    std::weak_ptr<PeriodicAction>
    Scheduler::schedule(PeriodicScheduleSlot::Duration interval,
                        PeriodicAction::Function function)
    {
      std::unique_lock<std::mutex> lock ( m_data_mutex );

      SlotMap::iterator iter ( m_slots.find(interval) );
      Slot* slot ( NULL );

      /* Look for a matching slot. */
      if ( iter != m_slots.end() )
        slot = &(iter->second);
      else
        {
          /* Add a new slot running at the specified interval  */
          std::pair<SlotMap::iterator, bool>
            slotpair = m_slots.emplace(std::make_pair(interval, Slot { *this, interval }));
          assert(slotpair.second);

          slot = &(slotpair.first->second);
        }
      
      assert(slot != NULL);

      /* Add the action to the slot. */
      std::weak_ptr<PeriodicAction> out ( slot->emplace(function) );
      assert(! slot->empty() );

      return out;
    }

    void
    Scheduler::remove(const std::weak_ptr<ScheduledAction> action)
    {
      if ( ! action.expired() )
        {
          std::unique_lock<std::mutex> lock ( m_data_mutex );
          m_actions.erase(action.lock());
        }
    }

    void
    Scheduler::remove(std::weak_ptr<PeriodicAction> action)
    {
      if ( ! action.expired() )
        {
          std::unique_lock<std::mutex> lock ( m_data_mutex );
          action.lock()->slot->remove(action);
        }
    }

    boost::asio::io_service&
    Scheduler::get_io_service()
    {
      return m_io_service;
    }
  }
}
