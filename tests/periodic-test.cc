#include <crisp/util/PeriodicScheduler.hh>
#include <cstdio>

int
main(int argc, char* argv[])
{
  using namespace crisp::util::literals;

  boost::asio::io_service service;
  crisp::util::PeriodicScheduler scheduler ( service );

  scheduler.schedule(1_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "1\n"); });
  scheduler.schedule(5_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "5\n"); });
  scheduler.schedule(10_Hz, [&](crisp::util::PeriodicAction&)
                     { fprintf(stdout, "10\n"); });

  service.run();

  return 0;
}
