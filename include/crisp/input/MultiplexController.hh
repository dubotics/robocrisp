#ifndef crisp_input_MultiplexController_hh
#define crisp_input_MultiplexController_hh 1

#include <crisp/input/Controller.hh>
#include <vector>

namespace crisp
{
  namespace input
  {
    /** Virtual controller for making two or more input devices appear as a
        single device. */
    class MultiplexController : public Controller
    {
    protected:
      std::vector<std::shared_ptr<Controller> > m_controllers;

    public:
      MultiplexController();
      virtual ~MultiplexController();

      void add(std::shared_ptr<Controller> controller);

      virtual void run();
      virtual void stop();
    };
  }
}


#endif  /* crisp_input_MultiplexController_hh */
