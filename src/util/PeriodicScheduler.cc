#include <crisp/util/PeriodicScheduler.hh>
#include <cassert>
namespace crisp
{
  namespace util
  {

    PeriodicScheduler::PeriodicScheduler(boost::asio::io_service& io_service)
      : m_io_service ( io_service ),
        m_slots ( )
    {}

    PeriodicScheduler::~PeriodicScheduler()
    {}

    PeriodicScheduler::Action&
    PeriodicScheduler::schedule(PeriodicScheduler::Duration interval,
                                PeriodicScheduler::Action::Function function)
    {
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
      Action& out ( slot->emplace(function) );
      assert(! slot->empty() );
    }


    boost::asio::io_service&
    PeriodicScheduler::get_io_service()
    {
      return m_io_service;
    }
  }
}
