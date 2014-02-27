#include <crisp/util/WorkerObject.hh>
#include <sys/time.h>
#include <time.h>

#define ONE_BILLION 1000000000

static void
sleep_ns(long int ns)
{
  long int seconds = 0;
  if ( ns > ONE_BILLION )
    {
      seconds = ns / ONE_BILLION;
      ns = ns % ONE_BILLION;
    }

  struct timespec sleep_time = { seconds, ns };
  struct timespec remain_time = { 0 };

  while ( nanosleep(&sleep_time, &remain_time) < 0 )
    sleep_time = remain_time;
}

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

    bool WorkerObject::WorkerThread::halt(bool wait)
    {
      if ( thread.get_id() == std::this_thread::get_id() )
        return false;
      else if ( thread.joinable() )
        {
          should_halt = true;
          if ( wait )
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

    boost::asio::io_service&
    WorkerObject::get_io_service()
    { return m_io_service; }

    bool
    WorkerObject::launch()
    {
      bool can_launch ( m_worker_threads.size < m_worker_threads.capacity );

      if ( can_launch )
        for ( size_t i ( m_worker_threads.size ); i < m_worker_threads.capacity; ++i )
          m_worker_threads.emplace().launch(m_io_service);

      return can_launch;
    }
      
    std::size_t
    WorkerObject::num_live_threads() const
    {
      size_t out ( 0 );
      for ( const WorkerThread& worker : m_worker_threads )
        if ( worker.thread.joinable() )
          ++out;
      return out;
    }


    bool
    WorkerObject::running(bool all) const
    {
      if ( all )
        {
          for ( const WorkerThread& worker : m_worker_threads )
            if ( ! worker.thread.joinable() )
              return false;
          return true;
        }
      else
        {
          for ( const WorkerThread& worker : m_worker_threads )
            if ( worker.thread.joinable() )
              return true;
          return false;
        }
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
        {
          /* WARNING: disgusting hack follows :) */

          /* start a thread that:

               (1) calls `halt(false)` on each worker thread to tell them to
                   exit, and

               (2) calls `halt(true)` on each worker thread to wait for them to
                   exit
          */
          std::thread halt_thread([&]() {
              for ( WorkerThread& worker : m_worker_threads )
                worker.halt(false);
              for ( WorkerThread& worker : m_worker_threads )
                worker.halt(true);

              m_worker_threads.clear();
            });

          /* Make work for those threads so they can do something and then
             notice they've been asked to halt.

             I don't like the way this is implemented, since it might flood the
             io_service's work queue with excess calls to `sleep_ns`.  It does,
             however, seem to work.
          */
          std::thread make_work_thread([&]() {
              while ( running() )
                {
                  m_io_service.post(std::bind(&sleep_ns, 1000));
                }
            });

          if ( halt_thread.joinable() )
            halt_thread.join();
          if ( make_work_thread.joinable() )
            make_work_thread.join();
        }

      return canhalt;
    }
  }
}
