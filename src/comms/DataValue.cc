#include <crisp/comms/DataValue.hh>

namespace crisp
{
  namespace comms
  {
    template <>
    DataValue<>::DataValue(DataDeclaration<>&& type)
      : data_type ( type ),
	value ( new uint8_t[type.width] ),
	owns_value ( true )
    {
      memset(value, 0, type.width);
    }

    template <>
    DataValue<>::DataValue(const DataDeclaration<>& _data_type, DecodeBuffer& buf)
      : data_type ( _data_type ),
	value ( nullptr ),
	owns_value ( true )
    {
      size_t count = 1;
      if ( data_type.is_array )
	buf.read(&count, data_type.width);

      value = new uint8_t[count * data_type.width];

      if ( buf.length - buf.offset >= data_type.width )
	buf.read(value, count * data_type.width);
      else
        memset(value, 0, count * data_type.width);
    }

    template <>
    DataValue<>::DataValue(DataValue&& dv)
    : data_type ( dv.data_type ),
      value ( dv.value ),
      owns_value ( dv.owns_value )
    { dv.value = nullptr;
      dv.owns_value = false; }



    template <>
    DataValue<>::~DataValue()
    {
      if ( value && owns_value )
	delete[] value;
      value = nullptr;
    }


    template <>
    DataValue<>
    DataValue<>::decode(DecodeBuffer& buf, const DataDeclaration<>& _data_type)
    { return DataValue<>(_data_type, buf); }


    template <>
    EncodeResult
    DataValue<>::encode(MemoryEncodeBuffer& buf) const
    {
      return buf.write(value, data_type.width);
    }


    template <>
    DataValue<>&
    DataValue<>::operator =(DataValue<>&& dv)
    {
      data_type = std::move(dv.data_type);
      if ( value && owns_value )
	delete[] value;
      value = dv.value;
      owns_value = dv.owns_value;
      dv.owns_value = false;
      return *this;
    }
  }
}
