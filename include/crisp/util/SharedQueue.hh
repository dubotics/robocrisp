/** @file
 *
 * Defines a `SharedQueue` container class inherited from std::queue for
 * multi-threaded access.
 */
#ifndef SharedQueue_hh
#define SharedQueue_hh 1

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <pthread.h>
#include <csignal>

namespace crisp
{
  namespace util
  {
    /** Encapsulates `std::queue` to provide a thread-safe interface.
     * Additionally provides a means of waiting for the next available item in
     * the queue.
     *
     * Thread safety is provided by acquiring a lock on an internal mutex
     * whenever the underlying queue is accessed.
     */
    template < typename _Tp >
    class SharedQueue : protected std::queue<_Tp>
    {
    protected:
      /** Base class of the SharedQueue instantiation. */
      typedef std::queue<_Tp> Base;

      /** Mutex type used for access control. */
      typedef std::mutex Mutex;

      /** Mutex on which a lock should be acquired during any modifying
          operation on the queue.  */
      Mutex m_mutex;

      /** Condition variable to be used for "next available item"
          notification. */
      std::condition_variable m_cv;

      /** Flag set to indicate that a wait-for-next was aborted. */
      std::atomic<bool> m_aborted;

    public:
      /** Passthrough constructor for the underlying `std::queue`. */
      template < typename... Args >
      inline
      SharedQueue(Args... args)
        : Base(args...),
          m_mutex ( ),
          m_cv ( ),
          m_aborted ( false )
      {}

      /** Move constructor.  Moves the contents of the given shared queue into
          the current object. */
      SharedQueue(SharedQueue&& sq)
        : Base(std::move(sq)),
          m_mutex ( ),
          m_cv ( )
      {}

      /** Wake all threads waiting for an item from this queue.  This is a stopgap solution for
       * properly halting network nodes.
       */
      inline void
      wake_all()
      {
        std::unique_lock<Mutex> lock ( m_mutex );

        m_aborted = true;
        m_cv.notify_all();
      }

      using Base::empty;
      using Base::size;
      using Base::front;
      using Base::back;

      /** Construct an item in-place at the end of the queue.
       *
       * @param args Arguments to be passed to the item's constructor.
       */
      template < typename... Args >
      inline void
      emplace(Args... args)
      { std::unique_lock<Mutex> lock ( m_mutex );
	Base::emplace(args...);
	m_cv.notify_one(); }

      /** Push an item onto the queue.
       *
       * @param value Value to be added at the end of the queue.
       */
      inline void
      push(const _Tp& value)
      { std::unique_lock<Mutex> lock ( m_mutex );
	Base::push(value);
	if ( size() == 1 )
	  m_cv.notify_one();
      }

      /** Push an item onto the queue.
       *
       * @param value Value to be added at the end of the queue.
       */
      inline void
      push(_Tp&& value)
      { std::unique_lock<Mutex> lock ( m_mutex );
	Base::push(value);
	if ( size() == 1 )
	  m_cv.notify_one();
      }

      /** Remove the first item in the queue. */
      inline void
      pop()
      { std::unique_lock<Mutex> lock ( m_mutex );
	Base::pop(); }
    

      /** Fetch the next available entry.  If the queue is currently empty, this method will block
       *  the current thread until another thread enqueues an object.
       *
       * @return The next item on the queue.
       */
      inline _Tp
      next(bool* aborted = nullptr)
      {
	_Tp out;

        if ( m_aborted && aborted )
          *aborted = true;
        else
          {
            std::unique_lock<Mutex> lock ( m_mutex );

            if ( empty() )
              {
                /* Signal handlers seem to get confused when initiated while `next` is called from
                   a Boost.Asio coroutine function. */
                sigset_t sigset, oldset;
                sigfillset(&sigset);

                pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
                while ( empty() && ! m_aborted )
                  m_cv.wait(lock);
                pthread_sigmask(SIG_SETMASK, &oldset, NULL);
              }

            if ( ! m_aborted )
              {
                out = std::move(front());
                Base::pop();
              }
            else if ( aborted )
              *aborted = true;
          }

	return out;
      }
    };
  }
}

#endif	/* SharedQueue_hh */
