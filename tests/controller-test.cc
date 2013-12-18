#include <crisp/input/EvDevController.hh>
#include <csignal>
#include <thread>

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
  if ( argc < 2 )
    {
      fprintf(stderr, "Usage: %s EVDEV\n", argv[0]);
      return 1;
    }
  using namespace crisp::input;

  EvDevController controller ( argv[1] );
  for ( Axis& axis : controller.axes )
    {
      if ( axis.type == Axis::Type::ABSOLUTE )
        axis.set_coefficients({ 1, 0, 0, 0 }); /* y = x³ */
      axis.hook([&](const Axis& _axis, Axis::State state)
		{ fprintf(stderr, "[%2d](%s) raw %8d | mapped %.5f\n", axis.id, axis.get_name(), state.raw_value, state.value); });
    }
  for ( Button& button : controller.buttons )
    button.hook([&](const Button& _button, Button::State state)
                { fprintf(stderr, "[%2d](%s) %s\n", button.id, button.get_name(),
                          state.value == ButtonState::PRESSED
                          ? "pressed" : "released"); });

  std::atomic<bool> run_flag ( true );
  std::thread controller_thread ( [&]() { controller.run(run_flag); });

  wait_for_signal();

  run_flag = false;
  controller_thread.join();
  
  return 0;
}
