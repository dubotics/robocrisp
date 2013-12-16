#include <crisp/util/Signal.hh>

void
int_callback(int n)
{
  fprintf(stderr, "%s: n = 0x%X\n", __PRETTY_FUNCTION__, n);
}

int
main(int argc, char* argv[])
{
  using namespace crisp::util;
  boost::asio::io_service service;

  Signal<void(int)> int_signal;
  std::weak_ptr<typename Signal<void(int)>::Action>
    int_action ( int_signal.add(int_callback) );
  int_signal.emit(33);
  int_signal.remove(int_action);
  int_signal.emit(0xDEADBEEF);          /* ERROR */

  /* TODO: do some more testing */

  return 0;
}
