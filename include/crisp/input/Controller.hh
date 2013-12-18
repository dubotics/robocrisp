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
 
      /** Read events from the underlying hardware while a condition flag is set.  This function
       *  blocks execution.
       *
       * @param run_flag A flag that will be set to `true` while the controller's
       *   event-read loop should keep running.
       */
      virtual void run(const std::atomic<bool>& run_flag) = 0;

      DereferencedArrayAccessor<Axis,std::shared_ptr<Axis> > axes;
      DereferencedArrayAccessor<Button,std::shared_ptr<Button> > buttons;
    };
  }
}

#endif	/* input_Controller_hh */
