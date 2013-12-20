#include <fcntl.h>
#include <linux/input.h>
#include <crisp/input/EvDevController.hh>

namespace crisp
{
  namespace input
  {
    const char* EvDevButton::get_name() const
    {
      return libevdev_event_code_get_name(EV_KEY, id);
    }

    const char* EvDevAxis::get_name() const
    {
      return libevdev_event_code_get_name(type == Axis::Type::RELATIVE
                                          ? EV_REL
                                          : EV_ABS, id);
    }


    EvDevController::EvDevController(const char* evdev) throw ( std::system_error )
      : Controller ( ),
	m_fd ( -1 ),
        m_evdev ( nullptr ),
	m_axis_map ( )
    {
      if ( (m_fd = open(evdev, O_RDONLY)) < 0 )
	throw std::system_error(std::error_code(errno, std::system_category()));
      else
	{
          int rc;
          if ( (rc = libevdev_new_from_fd(m_fd, &m_evdev)) < 0 )
            throw std::system_error(std::error_code(-rc, std::system_category()));

	  /* Determine the available axes. */
	  size_t num_axes = 0;
	  struct {
            uint16_t type;
            uint16_t code;
	    struct input_absinfo absinfo;
	  } _axes[ABS_CNT + REL_CNT];
	  memset(_axes, 0, sizeof(_axes));
          const struct input_absinfo* absinfo;

          /* Absolute axes */
          if ( libevdev_has_event_type(m_evdev, EV_ABS) )
            for ( size_t i = 0; i < ABS_CNT; ++i )
              if ( (absinfo = libevdev_get_abs_info(m_evdev, i)) != nullptr &&
                   absinfo->minimum != absinfo->maximum )
                {
                  _axes[num_axes].type = EV_ABS;
                  _axes[num_axes].code = i;
                  memcpy(&(_axes[num_axes++].absinfo), absinfo, sizeof(struct input_absinfo));
                }

          /* Relative axes */
          if ( libevdev_has_event_type(m_evdev, EV_REL) )
            for ( size_t i ( 0 ); i < EV_CNT; ++i )
              if ( libevdev_has_event_code(m_evdev, EV_REL, i) )
                {
                  _axes[num_axes].type = EV_REL;
                  _axes[num_axes++].code = i;
                }

	  /* Initialize our stored list of axes. */
	  m_axes.clear();
	  m_axes.reserve(num_axes);
	  for ( size_t i = 0; i < num_axes; ++i )
	    {
              if ( _axes[i].type == EV_ABS )
                {
                  struct input_absinfo& info ( _axes[i].absinfo );
                  Axis::RawConfig raw { info.value, info.minimum, info.maximum,  info.flat, info.flat };
                  m_axes.emplace_back(std::make_shared<EvDevAxis>(raw, _axes[i].code));
                }
              else
                m_axes.emplace_back(std::make_shared<EvDevAxis>(Axis::Type::RELATIVE, _axes[i].code));

              m_axis_map.emplace(std::make_pair(_axes[i].type, _axes[i].code), i);
	    }

          /* Buttons */
          if ( libevdev_has_event_type(m_evdev, EV_KEY) )
            {
              uint16_t _buttons[KEY_CNT];
              size_t num_buttons ( 0 );
              for ( size_t i = 0; i < KEY_CNT; ++i )
                if ( libevdev_has_event_code(m_evdev, EV_KEY, i) )
                  _buttons[num_buttons++] = i;

              m_buttons.clear();
              m_buttons.reserve(num_buttons);
              for ( size_t i = 0; i < num_buttons; ++i )
                {
                  m_buttons.emplace_back(std::make_shared<EvDevButton>(_buttons[i]));
                  m_button_map.emplace(std::make_pair(_buttons[i], i));
                }
            }
	}
    }

    EvDevController::~EvDevController()
    {
      close(m_fd);
      libevdev_free(m_evdev);
    }


    ssize_t
    EvDevController::wait_for_event(struct input_event* ev)
    {
      return read(m_fd, ev, sizeof(struct input_event));
    }


    void
    EvDevController::run(const std::atomic<bool>& run_flag)
    {
      struct input_event ev;
      while ( run_flag )
	{
	  if ( wait_for_event(&ev) > 0 )
	    {
	      switch ( ev.type )
		{
                case EV_REL:
		case EV_ABS:
		  {             /* <-- need these brackets so that the iterator
                                   (next line) is initialized properly. */
		    auto iter ( m_axis_map.find(std::make_pair(ev.type, ev.code)) );
		    if ( iter != m_axis_map.end() )
		      axes[iter->second].post(ev.value);
		  }
		  break;

                case EV_KEY:
                  {
                    auto iter ( m_button_map.find(ev.code) );
                    if ( iter != m_button_map.end() )
                      buttons[iter->second].post(ev.value);
                    break;
                  }

		default:
		  if ( ev.type != EV_SYN )
		    fprintf(stderr, "got %s event: code \"%s\", value %d (0x%x)\n",
                            libevdev_event_type_get_name(ev.type), libevdev_event_code_get_name(ev.type, ev.code),
                            ev.value, ev.value);
		  break;
		}
	    }
	}
    }
  }
}
