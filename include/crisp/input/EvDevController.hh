#ifndef input_EvDevController_hh
#define input_EvDevController_hh 1

#include <system_error>
#include <unordered_map>
#include <libevdev/libevdev.h>
#include <crisp/input/Controller.hh>

namespace std
{
  template <> struct hash<std::pair<uint16_t,uint16_t> >
  {
    inline constexpr size_t operator()(const std::pair<uint16_t,uint16_t>& p) const noexcept
    { return ((p.first << 16) | p.second); }
  };
}

namespace crisp
{
  namespace input
  {
    class EvDevButton : public Button
    {
    public:
      using Button::Button;
      virtual const char* get_name() const;
    };

    class EvDevAxis : public Axis
    {
    public:
      using Axis::Axis;
      virtual const char* get_name() const;
    };


    /** Linux `evdev`-based game controller class.
     */
    class EvDevController : public Controller
    {
    private:
      int m_fd;
      libevdev* m_evdev;

      /** Mapping from axis type and code to index in `axes`. */
      std::unordered_map<std::pair<uint16_t, uint16_t>,size_t>
        m_axis_map;

      /** Mapping from button code to index in `buttons`. */
      std::unordered_map<uint16_t,size_t> m_button_map;

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
      virtual void run(const std::atomic<bool>& run_flag);
    };
  }
}
#endif	/* input_EvDevController_hh */
