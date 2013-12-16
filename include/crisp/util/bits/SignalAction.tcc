/* -*- C++ -*- */
#ifndef crisp_util_bits_SignalAction_tcc
#define crisp_util_bits_SignalAction_tcc 1


namespace crisp
{
  namespace util
  {
    template < typename Return, typename... Args >
    SignalAction<Return(Args...)>::SignalAction(crisp::util::Signal<Return(Args...)>& signal,
                                                Function&& function)
      : m_signal ( signal ),
        m_function ( function )
    {}

    template < typename Return, typename... Args >
    SignalAction<Return(Args...)>::~SignalAction()
    {}


    template < typename Return, typename... Args >
    std::shared_ptr<SignalAction<Return(Args...)> >
    SignalAction<Return(Args...)>::get_pointer()
    {
      return std::enable_shared_from_this<SignalAction>::shared_from_this();
    }
    
    template < typename Return, typename... Args >
    void
    SignalAction<Return(Args...)>::cancel()
    {
      m_signal.remove(get_pointer());
    }
        
  }
}

#endif  /* crisp_util_bits_SignalAction_tcc */
