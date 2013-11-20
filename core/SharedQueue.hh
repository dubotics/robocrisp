#ifndef SharedQueue_hh
#define SharedQueue_hh 1

#include <condition_variable>
#include <mutex>
#include <queue>

/** Encapsulates `std::queue` to provide a thread-safe interface.  Additionally provides a means
 * of waiting for the next available item in the queue.
 */
template < typename _Tp >
class SharedQueue : protected std::queue<_Tp>
{
protected:
  typedef std::queue<_Tp> BaseType;

  /**< Kind of mutex we'll be using within this class. */
  typedef std::mutex Mutex;

  /** Mutex on which a lock should be acquired during any modifying operation on the queue.  */
  Mutex m_mutex;

  /** Condition variable to be used for "next available item" notification. */
  std::condition_variable m_cv;

public:
  /** Passthrough constructor for the underlying `std::queue`. */
  template < typename... Args >
  inline
  SharedQueue(Args... args)
    : BaseType(args...),
      m_mutex ( ),
      m_cv ( )
  {}

  using BaseType::empty;
  using BaseType::size;
  using BaseType::front;
  using BaseType::back;

  /** Construct an item in-place at the end of the queue.
   *
   * @param args Arguments to be passed to the item's constructor.
   */
  template < typename... Args >
  inline void
  emplace(Args... args)
  { std::unique_lock<Mutex> lock ( m_mutex );
    BaseType::emplace(args...);
    m_cv.notify_one(); }

  /** Push an item onto the queue.
   *
   * @param value Value to be added at the end of the queue.
   */
  inline void
  push(const _Tp& value)
  { std::unique_lock<Mutex> lock ( m_mutex );
    BaseType::push(value);
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
    BaseType::push(value);
    if ( size() == 1 )
      m_cv.notify_one();
  }

  /** Remove the first item in the queue. */
  inline void
  pop()
  { std::unique_lock<Mutex> lock ( m_mutex );
    BaseType::pop(); }
    
  /** Fetch the next available entry.  If the queue is currently empty, this method will block
   *  the current thread until another thread enqueues an object.
   *
   * @return The next item on the queue.
   */
  inline _Tp
  next()
  {
    _Tp out;

    {
      std::unique_lock<Mutex> lock ( m_mutex );

      while ( empty() )
	m_cv.wait(lock);

      out = std::move(front());
      BaseType::pop();
    }

    return out;
  }
};


#endif	/* SharedQueue_hh */
