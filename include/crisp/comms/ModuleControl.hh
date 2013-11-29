#ifndef ModuleControl_hh
#define ModuleControl_hh 1

#include <cstddef>
#include <cstdint>
#include <limits>

#include <crisp/util/SArray.hh>
#include <crisp/comms/common.hh>
#include <crisp/comms/DataDeclaration.hh>
#include <crisp/comms/DataValue.hh>
#include <crisp/comms/ModuleInput.hh>
#include <crisp/comms/Message.hh>

namespace crisp
{
  namespace comms
  {
    struct Configuration;
    struct Module;

    /** Module-control data structure.  Used to send values to inputs on the robot. */
    struct __attribute__ (( packed, align(1) ))
    ModuleControl
    {
      static const MessageType Type;
      static const size_t HeaderSize;

      typedef ModuleControl TranscodeAsType;

      ModuleControl();

      /** Move constructor. */
      ModuleControl(ModuleControl&&);

      /** Copy constructor. */
      /* ModuleControl(const ModuleControl& mc); */

      ModuleControl&
      operator = (ModuleControl&& mc);

      /** Constructor used to build module control messages for transmission.
       *
       * @param _module Pointer to the module for which inputs are to be specified.
       *
       * @param _values_capacity Number of values for which to allocate space.  If this is the
       * default value, the num
       */
      ModuleControl(const Module* _module, size_t _values_capacity = std::numeric_limits<size_t>::max());
      ~ModuleControl();
      void reset(const Module* _module = nullptr);


      /** Fetch a pointer to the stored input-value instance for the given input.
       * @param 
       */
      DataValue<>*
	value_for(const ModuleInput<>& input);

      const DataValue<>*
	value_for(const ModuleInput<>& input) const;


      /** Fetch a pointer to the stored input-value instance for the given input.
       * @param name Name of the input for which the stored value is desired.
       * @return Pointer to the requested value, or @c NULL if no such value is set.
       */
      DataValue<>*
	value_for(const char* name);

      const DataValue<>*
	value_for(const char* name) const;


      /** Get the space required for this object in encoded form. */
      size_t
	get_encoded_size() const;

      bool
	operator ==(const ModuleControl& mc) const;

      /** Clear (set to neutral) an input by name. */
      ModuleControl&
	clear(const char* name);

      template < typename _T, typename... _Args >
	inline ModuleControl&
	clear(_T arg0, _Args... args)
      {
	clear(arg0);
	return clear(args...);
      }

      /** Clear (set to neutral) an input by reference. */
      ModuleControl&
	clear(const ModuleInput<>& input);

      /** Set the value of an input. */
      template < typename _T, typename _U >
	inline ModuleControl&
      set(const _U& input, _T value)
      {
	DataValue<>* dv ( value_for(input) );
	if ( dv )
	  dv->set(value);

	return *this;
      }

      /** Get the value of an input. */
      template < typename _T, typename _U >
	inline _T get(const _U input) const
      {
	const DataValue<>* dv ( value_for(input) );
	if ( dv )
	  return dv->get<_T>();
	else
	  return _T ( );
      }

      /** Decode a ModuleControl instance from a byte buffer.
       *
       * @param config Configuration from which to determine value types.
       *
       * @param buf Input buffer
       */
      DecodeResult
      decode(DecodeBuffer& buf,
             const Configuration& config);

      /** Decode a ModuleControl instance from a byte buffer, returning a copy of the
       *	instance.
       */
      static inline TranscodeAsType
	decode_copy(DecodeBuffer& buf, const Configuration& config)
      { TranscodeAsType out; out.decode(buf, config);
	return out; }

      /** Encode a ModuleInputValue instance into a byte buffer.
       *
       * @param buf Buffer into which the ModuleControl instance should be encoded.
       */
      EncodeResult
	encode(MemoryEncodeBuffer& buf) const;

      /** Get the number of values set in this object.
       */
      inline uint8_t
	get_num_values() const
      { uint8_t out ( 0 );
	if ( ! is_clear() )
	  for ( uint8_t i ( 0 ); i < (8 * sizeof(input_ids) - 1); ++i )
	    out += (input_ids & (1 << i)) ? 1 : 0;
	return out; }

      /** Check if this is a "clear" control message, i.e. requests that given inputs be reset to
       *	their neutral values.
       */
      inline bool is_clear() const
      { return input_ids & (1 << ((8 * sizeof(input_ids)) - 1)); }

      struct ValuePair
      {
	const ModuleInput<>* input;
	DataValue<> value;

	ValuePair(ValuePair&& vp);
	ValuePair(const ModuleInput<>& _input, DataValue<>&& dv);
	bool operator ==(const ValuePair& vp) const;
	bool operator !=(const ValuePair& vp) const;

	ValuePair& operator =(ValuePair&& vp);
      };

      uint8_t module_id;		/**< ID of the target module. */
      mutable uint16_t input_ids;	/**< bitfield specifying the inputs for values that are present */

      const Module* module;

      /* Tail fields: for each input present, a value (of size appropriate for that input's data
       * type) specifying the input value.
       */
      mutable crisp::util::SArray<ValuePair> values;
    };

  }
}
#endif	/* ModuleControl_hh */
