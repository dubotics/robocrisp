#ifndef crisp_util_WorkerObject_hh
#define crisp_util_WorkerObject_hh 1

#include <atomic>
#include <thread>
#include <cstdint>
#include <crisp/util/SArray.hh>
#include <boost/asio/io_service.hpp>

namespace crisp
{
  namespace util
  {
    /** Provides a convenient way to manage worker threads for Boost.Asio that are attached to a
     *  specific object.
     *
     * Although threads calling the `run` or `run_one` method of a Boost.Asio `io_service`
     * object may be used for _any_ work that needs to be done (and not necessarily work
     * produced by the current object), the number of threads required for a distinct set of
     * objects depends on the design of those objects -- so thread management should be left to
     * the objects that produce the work.  WorkerObject provides a simple way to manage the
     * worker threads initiated by a given derived class instance, including clean shutdown.
     */
    class WorkerObject
    {
    protected:
      /** Structure used to keep track of and control individual worker threads. */
      struct WorkerThread
      {
        WorkerThread();
        ~WorkerThread();

        /** Launch the worker thread, servicing the specified `io_service`.
         *
         * @param service The Boost.Asio `io_service` on which to service completion handlers.
         *
         * @return `true` if the thread was launched, and `false` if it was already running.
         */
        bool launch(boost::asio::io_service& service);

        /** Halt the worker thread.  This method will block until the thread has exited.
         *
         * @return `true` if the thread could be halted (or was already halted), and `false` if
         *   the method was called from within the worker thread itself.
         */
        bool halt();

        std::thread thread;         /**< Thread handle. */
        std::atomic<bool> should_halt; /**< Used to indicate to the thread that it should exit. */
      };

      boost::asio::io_service& m_io_service;

      /** Handles to the worker threads. */
      crisp::util::SArray<WorkerThread> m_worker_threads;

    public:
      /** Constructor. */
      WorkerObject(boost::asio::io_service& _io_service, size_t pool_size);
      virtual ~WorkerObject();


      /** Fetch a reference to the `io_service` for which the object does work.
       *
       * @return The object's internal `io_service` reference.
       */
      boost::asio::io_service&
      get_io_service();


      /** Launch the object's worker threads.
       *
       * @return `true` if the threads were launched, and `false` if they were already running.
       */
      bool
      launch();


      /** Check if worker threads are running.
       *
       * @param all When `true`, requests that the function return `true` only
       *   when _all_ allocated worker threads are running; otherwise returns
       *   `true` when _any_ allocated worker thread is running.
       *
       * @return A value indicating if any (when `all` is `false`) or all (when
       *   `all` is `true`) allocated worker threads are running.
       */
      bool
      running(bool all = false) const;

      /** Check if the worker pool can be halted from the calling thread.
       *
       * @return `true` if the calling thread is not one of the worker threads.
       */
      bool
      can_halt() const;


      /** Halt the object's worker threads.
       *
       * @return `true` if the threads were halted or were already halted, and `false` if the
       *    function was called from the wrong thread.
       */
      bool
      halt();
    };
  }
}


#endif  /* crisp_util_WorkerObject_hh */
