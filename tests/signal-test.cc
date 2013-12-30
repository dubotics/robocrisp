#include <crisp/util/Signal.hh>
#include <cstdio>

void
int_callback_1(int n)
{
  fprintf(stderr, "%s: n = 0x%X\n", __PRETTY_FUNCTION__, n);
}
void
int_callback_2(int n)
{
  fprintf(stderr, "%s: n = 0x%X\n", __PRETTY_FUNCTION__, n);
}
void
int_callback_3(int n)
{
  fprintf(stderr, "%s: n = 0x%X\n", __PRETTY_FUNCTION__, n);
}


struct AnObject {
  int id;
};

void obj_callback(const AnObject& obj)
{
  fprintf(stderr, "%s: obj.id = 0x%X\n", __PRETTY_FUNCTION__, obj.id);
}

template < typename _Signal, typename _Arg >
static int
test_sig(_Signal sig,
         const std::initializer_list<typename _Signal::Function>& handlers,
         const std::initializer_list<_Arg>& args)
{
  typedef std::weak_ptr<typename _Signal::Action> Action;
  Action *actions ( new Action[handlers.size()] );
  size_t i ( 0 );
  for ( const typename _Signal::Function& function : handlers )
    actions[i++] = sig.connect(function);

  for ( size_t j ( 0 ); j < i; j++ )
    fprintf(stderr, "actions[%zu]: %p\n", j, actions[j].lock().get());

  for ( const _Arg& arg : args )
    sig.emit(arg);

  sig.remove(actions[0]);

  for ( const _Arg& arg : args )
    sig.emit(arg);

  sig.clear();

  for ( const _Arg& arg : args )
    sig.emit(arg);

  delete[] actions;

  return 0;
}

template < typename _Signal, typename _Arg >
static int
run_tests(_Signal sig, boost::asio::io_service& service,
          const std::initializer_list<typename _Signal::Function>& handlers,
          const std::initializer_list<_Arg>& args)
{
  fprintf(stderr, "_without_ io_service:\n");
  sig.clear();
  sig.clear_io_service();
  test_sig(sig, handlers, args);


  fprintf(stderr, "\n_with_ io_service:\n");
  sig.clear();
  sig.set_io_service(service);
  test_sig(sig, handlers, args);
  service.run();

  return 0;
}


int
main(int argc, char* argv[])
{
  using namespace crisp::util;
  boost::asio::io_service service;
  /* Test pass-by-value */
  fprintf(stderr, "\033[1;33mPass by value:\033[0m\n");
  Signal<void(int)> int_signal;
  run_tests(int_signal, service,
            { int_callback_1, int_callback_2, int_callback_3 },
            { 1, 2, 3 });
  service.reset();

  /* Test pass-by-reference */
  fprintf(stderr, "\n\033[1;33mPass by reference:\033[0m\n");
  Signal<void(const AnObject&)> obj_signal;
  run_tests(obj_signal, service,
            { obj_callback }, { AnObject{ 24 } });


  return 0;
}
