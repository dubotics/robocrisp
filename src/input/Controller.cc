#include <crisp/input/Controller.hh>

namespace crisp
{
  namespace input
  {
    Controller::Controller()
      : m_axes ( ),
	axes ( m_axes )
    {}

    Controller::~Controller()
    {}
  }
}
