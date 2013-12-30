/* -*- C++ -*- */
#ifndef crisp_util_bits_Signal_tcc
#define crisp_util_bits_Signal_tcc 1

#include <type_traits>
#include <functional>

namespace crisp
{
  namespace util
  {
    template < typename _Tp >
    typename std::enable_if<std::is_reference<_Tp>::value && ! std::is_const<typename std::remove_reference<_Tp>::type>::value,
                            std::reference_wrapper<typename std::remove_reference<_Tp>::type> >::type
    forward_for_bind(_Tp arg)
    {
      return std::ref(arg);
    }

    template < typename _Tp >
    typename std::enable_if<std::is_reference<_Tp>::value && std::is_const<typename std::remove_reference<_Tp>::type>::value,
                            std::reference_wrapper<typename std::remove_reference<_Tp>::type> >::type
    forward_for_bind(_Tp arg)
    {
      return std::cref(arg);
    }

    template < typename _Tp >
    typename std::enable_if<!std::is_reference<_Tp>::value,_Tp>::type
    forward_for_bind(_Tp arg)
    {
      return arg;
    }

    template < typename Return, typename... Args >
    Signal<Return(Args...)>::Signal(boost::asio::io_service& service)
      : m_actions ( ),
        m_io_service ( &service ),
        m_mutex ( )
    {}

    template < typename Return, typename... Args >
    Signal<Return(Args...)>::Signal()
      : m_actions ( ),
        m_io_service ( nullptr ),
        m_mutex ( )
    {}

    template < typename Return, typename... Args >
    Signal<Return(Args...)>::Signal(Signal&& sig)
    : m_actions ( std::move(sig.m_actions) ),
      m_io_service ( sig.m_io_service ),
      m_mutex ( )
    {}

    template < typename Return, typename... Args >
    Signal<Return(Args...)>::Signal(const Signal& sig)
    : m_actions ( sig.m_actions ),
      m_io_service ( sig.m_io_service ),
      m_mutex ( )
    {}


    template < typename Return, typename... Args >
    Signal<Return(Args...)>::~Signal()
    {}


    template < typename Return, typename... Args >
    Signal<Return(Args...)>&
    Signal<Return(Args...)>::operator =(const Signal<Return(Args...)>& sig)
    {
      std::unique_lock<std::mutex> lock ( const_cast<std::mutex&>(sig.m_mutex) );
      m_actions = sig.m_actions;
      m_io_service = sig.m_io_service;
      return *this;
    }


    template < typename Return, typename... Args >
    void
    Signal<Return(Args...)>::set_io_service(boost::asio::io_service& service)
    { m_io_service = &service; }

    template < typename Return, typename... Args >
    void
    Signal<Return(Args...)>::clear_io_service()
    { m_io_service = nullptr; }


    template < typename Return, typename... Args >
    void
    Signal<Return(Args...)>::emit(Args... args)
    {
      if ( m_actions.empty() )
        return;

      /* For thread safety and to enable concurrency, we acquire a lock on the
         mutex only for long enough to copy the action list (it's a bunch of
         pointers, so copy efficiency should be a non-issue). */

      size_t i ( 0 );

      std::unique_lock<std::mutex> lock ( m_mutex );
      /* The use case for this code does not anticipate a significant number of
         actions per signal -- so we'll allocate space for our copied pointers
         on the stack (stack allocation is pretty damn fast).

         We use `std::weak_ptr`s for the copied pointers to allow changes
         (e.g. disconnections) by other threads to take effect in the time
         between this copy operation and each handler's invocation.
      */
      std::weak_ptr<Action>* actions
        ( static_cast<std::weak_ptr<Action>*>(alloca(m_actions.size() *
                                                     sizeof(std::shared_ptr<Action>))) );
      for ( const std::shared_ptr<Action>& action : m_actions )
        new ( &actions[i++] ) std::weak_ptr<Action>(action);
      lock.unlock();


      /* Invoke the user callbacks as appropriate. */
      if ( m_io_service )
        {
          for ( size_t j ( 0 ); j < i; ++j )
            {
              std::shared_ptr<Action> action ( actions[j].lock() );
              if ( action )
                m_io_service->post(std::bind(action->m_function, forward_for_bind(args)...));
            }
        }
      else
        {
          for ( size_t j ( 0 ); j < i; ++j )
            {
              std::shared_ptr<Action> action ( actions[j].lock() );
              if ( action )
                action->m_function(args...);
            }
        }


      /* Destructors of objects constructed via placement new must be called
         manually. */
      for ( size_t j ( 0 ); j < i; ++j )
        actions[j].~weak_ptr();
    }

    template < typename Return, typename... Args >
    void
    Signal<Return(Args...)>::clear()
    {
      std::unique_lock<std::mutex> lock ( m_mutex );
      m_actions.clear();
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
    typename Signal<Return(Args...)>::Connection
    Signal<Return(Args...)>::connect(typename Signal<Return(Args...)>::Function function)
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
