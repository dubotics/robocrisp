#include <crisp/input/Controller.hh>

namespace crisp
{
  namespace input
  {
    Controller::Controller()
      : m_axes ( ),
	m_buttons ( ),
	axes ( m_axes ),
	buttons ( m_buttons )
    {}

    Controller::~Controller()
    {}
  }
}
