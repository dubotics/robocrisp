#ifndef input_Axis_hh
#define input_Axis_hh 1

#include <cstdlib>
#include <initializer_list>
#include <vector>

#include <crisp/input/MappedEventSource.hh>

namespace input
{
  class Axis;

  /* Declare an external instantiation of this MappedEventSource type. */
  extern template class MappedEventSource<Axis, int32_t, double>;

  /** An absolute axis on an input device.  Axis supports both the linear
   *  value-mapping of MappedEventSource, and an arbitrary polynomial-expansion
   *  mapping with user-supplied polynomial coefficients.
   */
  class Axis : public input::MappedEventSource<Axis, int32_t, double>
  {
    typedef MappedEventSource<Axis, int32_t, double> BaseType;
  public:
    /** Method used to map a raw axis value to its output value. */
    enum class MapMethod
    {
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


    /** Initialize an axis with linear mapping only.
     *
     * @param _raw Raw-value mapping configuration.  This structure is used to
     *     _initialize_ the axis' "raw" field; i.e. the passed configuration may
     *     be modified post-construction.
     */
    Axis(RawConfig _raw);

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
    Axis(RawConfig _raw, const std::initializer_list<Value>& _coefficients);

    /** Move constructor for efficient construction of an axis from an
     *	lvalue.
     */
    Axis(Axis&& _axis);
    ~Axis();


    /** Set the coefficients used for the polynomial-expansion mapping.
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


#endif	/* input_Axis_hh */
