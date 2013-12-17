#include <fcntl.h>
#include <linux/input.h>
#include <crisp/input/EvDevController.hh>

namespace crisp
{
  namespace input
  {
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
                  m_axes.emplace_back(i, raw);
                }
              else
                m_axes.emplace_back(Axis::Type::RELATIVE, i);
              m_axis_map.emplace(std::make_pair(_axes[i].type, _axes[i].code), i);
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
