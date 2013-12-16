/* -*- C++ -*- */
#ifndef crisp_util_bits_PeriodicScheduleSlot_tcc
#define crisp_util_bits_PeriodicScheduleSlot_tcc 1


namespace crisp
{
  namespace util
  {
    template < typename... Args >
    std::weak_ptr<PeriodicAction>
    PeriodicScheduleSlot::emplace(Args... args)
    {
      bool was_empty ( m_actions.empty() );
      m_actions.emplace_front(std::make_shared<PeriodicAction>(this, args...));

      if ( was_empty )
        reset_timer();

      return m_actions.front();
    }
  }
}
          

#endif	/* crisp_util_bits_PeriodicScheduleSlot_tcc */
