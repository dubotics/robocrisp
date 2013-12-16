/* -*- C++ -*- */
#ifndef crisp_util_bits_Signal_tcc
#define crisp_util_bits_Signal_tcc 1


namespace crisp
{
  namespace util
  {
    template < typename Return, typename... Args >
    Signal<Return(Args...)>::Signal(boost::asio::io_service& service)
      : m_actions ( ),
        m_io_service ( &service ),
        m_mutex ( )
    {}

    template < typename Return, typename... Args >
    Signal<Return(Args...)>::Signal()
      : m_actions ( ),
        m_io_service ( nullptr )
    {}

    template < typename Return, typename... Args >
    Signal<Return(Args...)>::~Signal()
    {}

    template < typename Return, typename... Args >
    void
    Signal<Return(Args...)>::emit(Args... args)
    {
      /* For thread safety and to enable concurrency, we acquire a lock on the
         mutex only for long enough to copy the action list (it's a bunch of
         pointers, so copy efficiency should be a non-issue). */

      size_t i ( 0 );

      std::unique_lock<std::mutex> lock ( m_mutex );
      /* The use case for this code does not anticipate a significant number of
         actions per signal -- so we'll allocate space for our copied pointers
         on the stack (stack allocation is pretty damn fast).  */
      std::shared_ptr<Action>* actions
        ( static_cast<std::shared_ptr<Action>*>(alloca(m_actions.size() *
                                                       sizeof(std::shared_ptr<Action>))) );
      for ( const std::shared_ptr<Action>& action : m_actions )
        new ( &actions[i++] ) std::shared_ptr<Action>(action);
      lock.unlock();


      /* Invoke the user callbacks as appropriate. */
      if ( m_io_service )
        for ( size_t j ( 0 ); j < i; ++j )
          m_io_service->post(std::bind(actions[j]->m_function, args...));
      else
        for ( size_t j ( 0 ); j < i; ++j )
          actions[j]->m_function(args...);


      /* Destructors of objects constructed via placement new must be called
         manually. */
      for ( size_t j ( 0 ); j < i; ++j )
        actions[j].~shared_ptr<Action>();
    }

    template < typename Return, typename... Args >
    void
    Signal<Return(Args...)>::remove
      (const std::weak_ptr<typename Signal<Return(Args...)>::Action>& action)
    {
      if ( ! action.expired() )
        {
          std::unique_lock<std::mutex> lock ( m_mutex );
          m_actions.erase(action.lock());
        }
    }

    template < typename Return, typename... Args >
    std::weak_ptr<typename Signal<Return(Args...)>::Action>
    Signal<Return(Args...)>::add(typename Signal<Return(Args...)>::Function&& function)
    {
      std::unique_lock<std::mutex> lock ( m_mutex );

      std::pair<typename ActionSet::iterator, bool>
        result = m_actions.insert(std::make_shared<Action>(*this, std::move(function)));
      assert(result.second);

      return *result.first;
    }
  }
}

#endif  /* crisp_util_bits_Signal_tcc */
