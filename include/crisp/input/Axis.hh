#ifndef crisp_input_Axis_hh
#define crisp_input_Axis_hh 1

#include <cstdlib>
#include <initializer_list>
#include <vector>
#include <algorithm>

#include <crisp/input/MappedEventSource.hh>

namespace crisp
{
  namespace input
  {

    /* Declare an external instantiation of this MappedEventSource type. */
    class Axis;
    extern template class MappedEventSource<Axis, int32_t, double>;

    /** An axis on an input device.
     *
     *  For absolute axes, Axis supports both the linear value-mapping of
     *  MappedEventSource, and an arbitrary polynomial-expansion mapping with
     *  user-supplied polynomial coefficients.
     */
    class Axis : public input::MappedEventSource<Axis, int32_t, double>
    {
      typedef MappedEventSource<Axis, int32_t, double> BaseType;

    public:
      using BaseType::ID;

      /** Axis-value reporting style. */
      enum class Type
      {
        ABSOLUTE,               /**< Values are absolute input states. */
        RELATIVE                /**< Values are relative to the last input state. */
      };

      /** Method used to map a raw axis value to its output value. */
      enum class MapMethod
      {
        NONE,                   /**< No mapping is done. */
	LINEAR,			/**< Only the base linear mapping is used. */
	POLYNOMIAL		/**< After linear mapping, a polynomial
				   expansion with user-supplied coefficients is
				   applied. */
      };

      /** Information used to map a raw value to its linear axis value. */
      struct RawConfig
      {
	int32_t
          neutral,	/**< Neutral value for raw event values on this axis. */
	  minimum,	/**< Minimum value for raw event values on this axis. */
	  maximum,	/**< Maximum value for raw event values on this axis. */
	  deadzone_lower,	/**< Lower deadzone value. */
	  deadzone_upper;	/**< Upper deadzone value. */
      };
    public:
      Type
        type,                   /**< Hardware reporting style for the axis. */
        mode;                   /**< Software reporting style for the axis.
                                   When `mode` is different from `type`, the
                                   axis will emulate the selected axis type. */

    protected:
       /**< Last "raw" value for the axis.
        *
        * When emulating an absolute axis, this will contain the most recent
        * _emulated_ absolute raw value; when emulating a relative axis, it will
        * contain the most recent _hardware-reported_ absolute raw value.
        */
      RawValue m_last_raw_value;

    public:
      MapMethod map_method;	/**< Kind of value-mapping selected for this axis. */
      RawConfig raw;		/**< Raw-value mapping configuration. */

      /** Coefficients used for the polynomial-expansion mapping.
       *
       * As stored, the first coefficient corresponds to the zeroth power of the
       * input, the second to the first power of the input, etc.
       * <strong>Note</strong>, however, that this is _not_ how the polynomial
       * coefficients are _passed_ to the functions that set them -- there we
       * expect highest-order first.
       */
      std::vector<Value> coefficients;


      /** Fetch the name of the axis. */
      virtual const char* get_name() const = 0;

      /** Initialize a relative or absolute axis with no pre-set value mapping.
       *
       * @param _type Type of axis; either `Axis::Type::ABSOLUTE` or
       *     `Axis::Type::RELATIVE`.
       *
       * @param _id Implementation- and hardware-specific ID code for the axis.
       */
      Axis(Type _type, ID _id);

      /** Initialize an absolute axis with linear mapping only.
       *
       * @param _raw Raw-value mapping configuration.  This structure is used to
       *     _initialize_ the axis' "raw" field; i.e. the passed configuration may
       *     be modified post-construction.
       */
      Axis(RawConfig _raw, ID _id);

      /** Initialize an axis with polynomial-expansion mapping.
       *
       * @param _raw Raw-value mapping configuration.  This structure is used to
       *     _initialize_ the axis' "raw" field; i.e. the passed configuration may
       *     be modified post-construction.
       *
       * @param _coefficients Coefficients to be used with the
       *     polynomial-expansion mapping.
       *
       *     As with set_coefficients, the first coefficient given corresponds to
       * 	   the highest power of the input variable, and the last to the zeroth
       * 	   power of the input variable (i.e., to a constant offset).
       */
      Axis(RawConfig _raw, ID _id, const std::initializer_list<Value>& _coefficients);

      /** Move constructor for efficient construction of an axis from an
       *	Rvalue reference.
       */
      Axis(Axis&& _axis);
      ~Axis();


      /** Set the coefficients used for the polynomial-expansion mapping, and
       * set the axis to use that mapping method.
       *
       * @param list A list of N + 1 coefficients for an Nth-order polynomial.
       * 	   The first coefficient given corresponds to the highest power of the
       * 	   input variable, and the last to the zeroth power of the input
       * 	   variable (i.e., to a constant offset).
       */
      void set_coefficients(const std::initializer_list<Value>& list);


      /** Linear value-map implementation.  Linearly maps a raw input value to an
       * axis value.
       *
       * @param raw_value The raw value to map.
       *
       * @return A value between -1.0 and 1.0.  The raw value's neutral point is
       *     always mapped to 0.0.
       */
      Value map(RawValue raw_value) const;


      /** Secondary mapping function.  This method takes the output of
       * map(RawValue) and maps it according to map_method.
       *
       * @param linear_value Linearly-mapped axis value from map(RawValue).
       *
       * @return A value mapped onto the output space from linear space. 
       */
      Value map(Value linear_value) const;


      /* override the MappedEventSource method to allow for emulating a
         different axis type and for value-map type "none" */
      void
      post(RawValue raw);


      /**@name Deleted constructors.
       *
       * These constructors have been deleted and are not available for use.
       *@{
       */
      /** Default constructor. */
      Axis() = delete;
      /** Copy constructor. */
      Axis(const Axis&) = delete;
      /**@}*/
    };
  }
}

#endif	/* crisp_input_Axis_hh */
