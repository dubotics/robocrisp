#ifndef crisp_input_Button_hh
#define crisp_input_Button_hh 1

#include <crisp/input/MappedEventSource.hh>

namespace crisp
{
  namespace input
  {
    /** State of a button on an input controller. */
    enum class ButtonState
      {
	UNKNOWN,
	PRESSED,
	RELEASED
      };

    /* Declare an external instantiation of this MappedEventSource type. */
    class Button;
    extern template class MappedEventSource<Button, int32_t, ButtonState>;

    class Button : public MappedEventSource<Button, int32_t, ButtonState>
    {
      typedef MappedEventSource<Button, int32_t, ButtonState> BaseType;
    public:
      Button(ID _id);
      virtual ~Button() = default;
      ButtonState map(int32_t raw_value) const;

      /** Fetch the name of the button. */
      virtual const char* get_name() const = 0;
    };
  }
}

#endif	/* crisp_input_Button_hh */
