#include <crisp/util/PeriodicScheduler.hh>

#include <cstdio>
#include <csignal>

/** Block execution until one of SIGINT, SIGQUIT, or SIGTERM is sent to the
 *  process.
 */
void
wait_for_signal()
{
  /* Create a signal set and add SIGQUIT, SIGINT, and SIGTERM to
   * it.
   */
  sigset_t sigset;
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGQUIT);
  sigaddset(&sigset, SIGTERM);

  /* Block these signals so that program execution continues after the
   * call to sigwait.
   */
  pthread_sigmask(SIG_BLOCK, &sigset, 0);

  /* Wait for the arrival of any signal in the set. */
  int sig = 0;
  sigwait(&sigset, &sig);
}


int
main(int argc, char* argv[])
{
  using namespace crisp::util::literals;

  boost::asio::io_service service;
  crisp::util::PeriodicScheduler scheduler ( service );

  scheduler.schedule(1_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "1\n"); fsync(fileno(stdout)); });
  scheduler.schedule(5_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "5\n"); fsync(fileno(stdout)); });
  scheduler.schedule(10_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "10\n"); fsync(fileno(stdout)); });

  service.run();

  return 0;
}
