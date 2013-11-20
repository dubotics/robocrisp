#ifndef Module_hh
#define Module_hh 1

#include "config.h"
#include "common.hh"
#include "Sensor.hh"
#include "ModuleInput.hh"
#include "SArray.hh"


#include "decl-defn-macros.hh"

namespace Robot
{
  /* ****************************************************************
   * Module
   */

  /** Controllable component on the robot.
   * Standard layout: yes.
   */
  struct __attribute__ (( packed, align(1) ))
  Module
  {
    typedef Module TranscodeAsType;
    static const uint8_t MAX_INPUTS; /**< Maximum number of inputs that can be accomodated by
					this implementation.  */

    /** Null constructor. */
    Module();

    /** Initialize a new Module instance from scratch.
     * @param _name The module's name.
     * @param _inputs_capacity Number of inputs for which we should allocate space in the inputs array.
     * @param _inputs_capacity Number of sensors for which we should allocate space in the sensors array.
     */
    Module(const char* _name, uint8_t _inputs_capacity = 0, uint8_t _sensors_capacity = 0);

    /** Move constructor. */
    Module(Module&& m);
    /* Module(const Module& m); */

    ~Module();

    /** Equality comparison operator. */
    bool
    operator == (const Module& o) const;

    inline bool
    operator != (const Module& o) const
    { return !(*this == o); }

    /** Get the encoded size of a module instance. */
    size_t
    get_encoded_size() const;

    EncodeResult
    encode(MemoryEncodeBuffer& buf) const;
    

    /** Parse a Module instance from a byte buffer.
     *
     * @param buf Buffer from which to decode the module instance.
     */
    DecodeResult
    decode(DecodeBuffer& buf);
    
    static inline Module
    decode_copy(DecodeBuffer& buf)
    { Module m; m.decode(buf); return m; }

    void reset();

    /** Add a typed sensor to the Configuration.
     *  The sensor will be converted to a generic Sensor<>.
     *
     * @return `*this` for additional calls to add_sensor or add_input.
     */
    template < typename _T >
    inline Module&
    add_sensor(Sensor<_T>&& sensor)
    { 
      if ( sensors.owns_data )
	{
	  ++num_sensors;
	  sensors.push(static_cast<Sensor<> >(sensor));
	  sensors[num_sensors - 1].id = next_sensor_id++;
	}

      return *this;
    }


    /** Add an input to the module.
     *
     * @return `*this` for additional calls to add_sensor or add_input.
     */
    template < typename _T >
    inline Module&
    add_input(ModuleInput<_T>&& input)      
    {
      if ( inputs.owns_data || ! inputs.data )
	{
	  ++num_inputs;
	  inputs.push(input);
	  inputs.back().input_id = next_input_id++;
	}
      
      return *this;
    }

    /** Find an input by name. */
    inline const ModuleInput<>*
    find_input(const char* _name) const
    { for ( size_t i ( 0 ); i < num_inputs; ++i )
	if ( inputs[i].name_length == strlen(_name) && ! strncmp(_name, inputs[i].name, inputs[i].name_length) )
	  return &(inputs[i]);
      return nullptr;
    }

    /** Header is the five bytes always present at the start of an encoded Module. */
    static const size_t HeaderSize;


    uint8_t id;			/**< Component ID for this module.
				 * Range: 0..255 */
    uint8_t
      num_inputs,		/**< number of inputs this module has; range: 0..15. */
      num_sensors;

#ifdef SWIG
%immutable;
#endif
    uint8_t name_length;	/**< length of the name string directly following this structure. */

#ifdef SWIG
%immutable;
#endif
    const char* name;

#ifdef SWIG
%immutable;
#endif
    spt::SArray<ModuleInput<> > inputs;
    spt::SArray<Sensor<> > sensors;

    uint8_t
      next_input_id,
      next_sensor_id;

    bool owns_name;	    /**< If `true`, name pointer will be freed at object destruction. */
  };
}

#ifdef SWIG
#  undef ModuleInput
#endif

#endif	/* Module_hh */
