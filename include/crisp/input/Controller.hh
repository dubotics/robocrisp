#ifndef input_Controller_hh
#define input_Controller_hh 1

#include <vector>
#include <atomic>

#include <crisp/util/ArrayAccessor.hh>
#include <crisp/input/Axis.hh>

namespace input
{
  /** Linux `evdev`-based game controller class.
   *
   * This is mostly just a collection of axes right now; needs some 
   */
  class Controller
  {
  protected:
    std::vector<Axis> m_axes;

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

    ArrayAccessor<Axis> axes;
  };
}

#endif	/* input_Controller_hh */
