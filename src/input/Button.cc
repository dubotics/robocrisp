#include <crisp/input/Button.hh>

namespace crisp
{
  namespace input
  {

    template class MappedEventSource<Button, int32_t, ButtonState>;

    Button::Button(Button::ID _id)
      : BaseType ( _id )
    {}

    ButtonState
    Button::map(int32_t raw_value) const
    {
      return raw_value ? ButtonState::PRESSED : ButtonState::RELEASED;
    }


  }
}
