#ifndef input_EvDevController_hh
#define input_EvDevController_hh 1

#include <system_error>
#include <unordered_map>

#include <crisp/input/Controller.hh>

namespace input
{
  /** Linux `evdev`-based game controller class.
   */
  class EvDevController : public Controller
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
     * @param evdev `evdev` device file path.
     *
     * @throws std::system_error if an error occurs opening the device file.
     */
    EvDevController(const char* evdev) throw ( std::system_error );
    virtual ~EvDevController();

    /** Read events from the underlying hardware while a condition flag is set.  This function
     *  blocks execution.
     */
    virtual void run(std::atomic<bool> run_flag);

    /** Public, read-only accessors for private state variables.
     *@{
     */
    const char*& name;
    const char*& location;
    const char*& identifier;
    /**@}*/
  };
}

#endif	/* input_EvDevController_hh */
