#include "Sensor.hh"

namespace Robot
{
  template <>
  const size_t Sensor<>::HeaderSize =
    offsetof(Sensor<>, data_type) + Sensor<>::DataType::TranscodeAsType::HeaderSize;

  /* template <>
   * Sensor<>::Sensor(const Sensor& s)
   * : id ( s.id ),
   *   type ( s.type ),
   *   reporting_mode ( s.reporting_mode ),
   *   name_length ( s.name_length ),
   *   data_type ( s.data_type ),
   *   name ( s.name ),
   *   owns_name ( false )
   * {} */


  template <>
  Sensor<>::Sensor()
    : id ( 0 ),
      type ( ),
      reporting_mode ( ),
      name_length ( 0 ),
      data_type ( ),
      name ( nullptr ),
      owns_name ( false )
  {}
  template <>
  Sensor<>::Sensor(Sensor<>&& s)
    : id ( s.id ),
      type ( s.type ),
      reporting_mode ( s.reporting_mode ),
      name_length ( s.name_length ),
      data_type ( std::move(s.data_type) ),
      name ( s.name ),
      owns_name ( s.owns_name )
  { s.owns_name = false; }


  template <>
  Sensor<>::Sensor(uint16_t _id, SensorType _type, SensorReportingMode _mode, uint8_t _name_length, DataType&& _data_type, const char* _name)
    : id ( _id ),
      type ( _type ),
      reporting_mode ( _mode ),
      name_length ( _name_length ),
      data_type ( std::move(_data_type) ),
      name ( const_cast<char*>(_name) ),
      owns_name ( false )
  {}
      
    
  template <>
  Sensor<>::~Sensor()
  { if ( owns_name && name )
      delete[] name;
    name = nullptr;
  }

  template <>
  Sensor<>&
  Sensor<>::operator =(Sensor&& s)
  {
    id = s.id;
    type = s.type;
    reporting_mode = s.reporting_mode;
    name_length = s.name_length;
    data_type = static_cast<typename DataType::BaseType>(s.data_type);

    name = s.name;
    owns_name = s.owns_name;
    s.owns_name = false;

    return *this;
  }



  NAMED_TYPED_COMMON_FUNC_DEFNS(Sensor<>)
}
