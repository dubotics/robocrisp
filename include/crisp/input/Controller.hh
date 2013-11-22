#ifndef input_Controller_hh
#define input_Controller_hh 1

#include <system_error>
#include <vector>
#include <unordered_map>

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
  private:
    int m_fd;
    std::vector<Axis> m_axes;
    std::unordered_map<uint16_t,size_t> m_axis_map; /**< Mapping from axis code to index. */
    /* std::vector<Button> m_buttons; */
    char
    *m_name,
      *m_location,
      *m_identifier;

    ssize_t
    wait_for_event(struct input_event* e);

  public:		 
    /** Constructor.
     *
     * @param jsdev Joystick device-file path.
     *
     * @param evdev `evdev` device file path.
     */
    Controller(const char* evdev) throw ( std::system_error );
    virtual ~Controller();
 
    /** Read events from the underlying hardware while a condition flag is set.  This function
     *  blocks execution.
     */
    void run();

    /** Public, read-only accessors for private state variables.
     *@{
     */
    const char*& name;
    const char*& location;
    const char*& identifier;
    /**@}*/

    ArrayAccessor<Axis> axes;
  };
}

#endif	/* input_Controller_hh */
