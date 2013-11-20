#ifndef DataValue_hh
#define DataValue_hh 1

#include <type_traits>
#include "config.h"
#include "DataDeclaration.hh"

#if defined(ENABLE_ASSERT) && ENABLE_ASSERT
#  include <cassert>
#endif

namespace Robot
{
  /* ****************************************************************
   * DataValue
   */
  /** Encapsulation for a general-purpose serializable data value.  */
  template < typename _T = void, typename _Enable = _T >
  struct DataValue;

  /** Generic data-value structure. */
  template < typename _T >
  struct __attribute__ (( packed, align(1) ))
  DataValue < _T, typename std::enable_if<std::is_void<_T>::value,_T>::type >
  {
    DataValue(DataDeclaration<>&& type);

    /** Decode constructor. */
    DataValue(const DataDeclaration<>& _data_type, DecodeBuffer& buf);

#ifndef SWIG
    /** Move constructor. */
    DataValue(DataValue&& miv);
#endif

    ~DataValue();

    template < typename _U >
    DataValue(const DataDeclaration<_U>& _data_type)
      : DataValue(static_cast<DataDeclaration<>>(_data_type))
    {}

#ifndef SWIG
    template < typename _U, typename _V = const DataDeclaration<_U>& >
    inline
    DataValue(_V _data_type, _U _value)
      : data_type ( _data_type ),
        value ( new uint8_t[sizeof(_U)] ),
        owns_value ( true )
    { memcpy(value, &_value, sizeof(_U)); }
#endif

    template < typename _U >
    _U get() const
    {
      DataDeclaration<_U> expect_type;
      if ( data_type.type != expect_type.type ||
	   data_type.width != expect_type.width )
	fprintf(stderr, "WARNING: `get<%s>()` called on DataValue of type `%s`!\n",
		static_cast<DataDeclaration<>>(DataDeclaration<_U>()).type_name(), data_type.type_name());

      if ( value )
	return *reinterpret_cast<_U*>(value);
      else
	return _U ( );
    }

    /** Set the value to be encoded. */
    template < typename _U >
    void set(_U _value)
    {
      fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
      fprintf(stderr, "    %zd\n", _value);

      if ( ! value )
	{ data_type = DataDeclaration<_U>();
	  if ( value && owns_value )
	    delete[] value;
	  value = new uint8_t[sizeof(_U)];
	  owns_value = true;
	}

      *reinterpret_cast<_U*>(value) = _value;
    }

    /** Get the encoded size of the value. */
    inline size_t
    get_encoded_size() const
    { return data_type.width; }
    

    EncodeResult
    encode(MemoryEncodeBuffer& buf) const;

    static DataValue
    decode(DecodeBuffer& buf, const DataDeclaration<>& _data_type);

    DataValue<>&
    operator =(DataValue<>&& v);

    inline bool
    operator !=(const DataValue& v) const
    { return !(*this == v); }

    inline bool
    operator ==(const DataValue& v) const
    { return data_type == v.data_type && !memcmp(value, v.value, data_type.width); }


    DataDeclaration<> data_type;
    uint8_t* value;
    bool owns_value;
  };

  template < typename _T >
  struct __attribute__ (( packed, align(1) ))
  DataValue < _T, typename std::enable_if<!std::is_void<_T>::value,_T>::type >
  {
    DataDeclaration<_T> data_type;
    _T value;
  };
}

#endif	/* DataValue_hh */
