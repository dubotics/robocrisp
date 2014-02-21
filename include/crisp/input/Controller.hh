#ifndef input_Controller_hh
#define input_Controller_hh 1

#include <vector>
#include <atomic>
#include <memory>

#include <crisp/util/ArrayAccessor.hh>
#include <crisp/input/Axis.hh>
#include <crisp/input/Button.hh>

namespace crisp
{
  namespace input
  {
    /** Generic controller class.  This has pure-virtual method(s) and cannot be
        used directly; see e.g. EvDevController for an implementation.
     */
    class Controller
    {
    protected:
      std::vector<std::shared_ptr<Axis> > m_axes;
      std::vector<std::shared_ptr<Button> > m_buttons;

    public:
      Controller();
      virtual ~Controller();
 
      /** Read events from the underlying hardware.  This function blocks
       *  execution of the thread in which it is called.
       */
      virtual void run() = 0;

      /** Stop reading events from the underlying hardware.  The implementation
          of this function must be thread-safe. */
      virtual void stop() = 0;

      DereferencedArrayAccessor<Axis,std::shared_ptr<Axis> > axes;
      DereferencedArrayAccessor<Button,std::shared_ptr<Button> > buttons;
    };
  }
}

#endif	/* input_Controller_hh */
