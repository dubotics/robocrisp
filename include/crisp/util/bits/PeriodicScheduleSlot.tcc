/* -*- C++ -*- */
#ifndef crisp_util_bits_PeriodicScheduleSlot_tcc
#define crisp_util_bits_PeriodicScheduleSlot_tcc 1


namespace crisp
{
  namespace util
  {
    template < typename... Args >
    PeriodicAction::Pointer
    PeriodicScheduleSlot::emplace(Args... args)
    {
      bool was_empty ( m_actions.empty() );
      m_actions
#ifndef __AVR__
        .emplace_front(std::make_shared<PeriodicAction>(this, args...));
#else
        .emplace(new PeriodicAction(this, args...));
#endif

      if ( was_empty )
        reset_timer();

      return
#ifndef __AVR__
        m_actions.front();
#else
        m_actions.back();
#endif
    }
  }
}
          

#endif	/* crisp_util_bits_PeriodicScheduleSlot_tcc */
