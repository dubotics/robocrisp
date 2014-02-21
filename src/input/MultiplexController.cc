#include <crisp/input/MultiplexController.hh>
#include <thread>

namespace crisp
{
  namespace input
  {
    MultiplexController::MultiplexController()
      : Controller ( ),
        m_controllers ( )
    {}

    MultiplexController::~MultiplexController()
    {}
    
    void
    MultiplexController::add(std::shared_ptr<Controller> controller)
    {
      m_controllers.emplace_back(controller);

      for ( Axis& axis : controller->axes )
        m_axes.push_back(axis.shared_from_this());

      for ( Button& button : controller->buttons )
        m_buttons.emplace_back(button.shared_from_this());
    }

    void
    MultiplexController::run()
    {
      for ( auto iter = m_controllers.begin(); *iter != m_controllers.back(); ++iter )
        {
          std::shared_ptr<Controller>& controller ( *iter );
          std::thread([this,controller](){ controller->run(); stop(); }).detach();
        }

      m_controllers.back()->run();
    }
    void
    MultiplexController::stop()
    {
      for ( std::shared_ptr<Controller>& controller : m_controllers )
        controller->stop();
    }


  }
}
