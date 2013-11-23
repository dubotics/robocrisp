#include <crisp/input/Controller.hh>

namespace input
{
  Controller::Controller()
    : m_axes ( ),
      axes ( m_axes )
  {}

  Controller::~Controller()
  {}
}
