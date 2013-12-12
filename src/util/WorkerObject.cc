#include <crisp/util/WorkerObject.hh>

namespace crisp
{
  namespace util
  {
    WorkerObject::WorkerThread::WorkerThread()
      : thread ( ),
        should_halt ( false )
    {}

    WorkerObject::WorkerThread::~WorkerThread()
    {
      should_halt = true;

      if ( thread.joinable() )
        thread.join();
    }

    bool
    WorkerObject::WorkerThread::launch(boost::asio::io_service& service)
    {
      bool running ( thread.joinable() );
      if ( ! running )
        thread = std::thread([&]()
                             {
                               while ( ! should_halt )
                                 service.run_one();
                             });
      return ! running;
    }

    bool WorkerObject::WorkerThread::halt()
    {
      if ( thread.get_id() == std::this_thread::get_id() )
        return false;
      else if ( thread.joinable() )
        {
          should_halt = true;
          thread.join();
        }

      return true;
    }

    
    WorkerObject::WorkerObject(boost::asio::io_service& _io_service, size_t pool_size)
      : m_io_service ( _io_service ),
        m_worker_threads ( pool_size )
    {}

    WorkerObject::~WorkerObject()
    {
      halt();
    }

    bool
    WorkerObject::launch()
    {
      bool can_launch ( m_worker_threads.size < m_worker_threads.capacity );

      if ( can_launch )
        for ( size_t i ( m_worker_threads.size ); i < m_worker_threads.capacity; ++i )
          m_worker_threads.emplace().launch(m_io_service);

      return can_launch;
    }
      
    bool WorkerObject::can_halt() const
    {
      for ( const WorkerThread& worker : m_worker_threads )
        if ( worker.thread.joinable() && worker.thread.get_id() == std::this_thread::get_id() )
          return false;
      return true;
    }

    bool
    WorkerObject::halt()
    {
      bool canhalt ( can_halt() );

      if ( canhalt )
        for ( WorkerThread& worker : m_worker_threads )
          worker.halt();
      m_worker_threads.clear();

      return canhalt;
    }
  }
}
