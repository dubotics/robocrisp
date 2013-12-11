#include <crisp/util/Scheduler.hh>
#include <cstdio>

int
main(int argc, char* argv[])
{
 /* This gives us the user-defined-literal suffix `_Hz`, which takes a value in
    Hz and transforms it into the interval we need to pass to
    `Scheduler::schedule`. */
  using namespace crisp::util::literals;

  /* Instantiate an IO-coordinator object and a scheduler. */
  boost::asio::io_service service;
  crisp::util::Scheduler scheduler ( service );


  /* Schedule some functions to run every so often. */
  scheduler.schedule(1_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "1\n"); });
  scheduler.schedule(5_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "5\n"); });

  scheduler.schedule(10_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "10\n"); });

  scheduler.set_timer(std::chrono::milliseconds(500),
                      [&](crisp::util::ScheduledAction& action)
                      {
                        fprintf(stdout, "500 ms has elapsed.\n");
                      });

  /* The timers used by the scheduler won't execute unless there's a thread
     calling the io_service's `run` method, which parcels out work that results
     from asynchronous IO calls and other things as long as there's work left
     to be done.

     Normally we'd have a ProtocolNode running with at least one thread already
     inside that method; however here the scheduler is the only user of the
     `io_service` and we need to invoke `run` ourselves. */
  service.run();

  return 0;
}
