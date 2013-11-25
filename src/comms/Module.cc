#include <cstdlib>
#include <cassert>
#include <cstdio>

#include "Module.hh"
#include "Configuration.hh"
#include "config.h"

namespace Robot
{

  /* ****************************************************************
   * Module
   */

  const size_t Module::HeaderSize =
    offsetof(Module, name);

  const uint8_t Module::MAX_INPUTS = MODULE_MAX_INPUTS;


  Module::Module()
      : id ( 0 ),
        num_inputs ( 0 ),
	num_sensors ( 0 ),
        name_length ( 0 ),
        name ( nullptr ),
        inputs ( ),
	sensors ( ),
        next_input_id ( 0 ),
	next_sensor_id ( 0 ),
	owns_name ( false )
    {}

  /* Module::Module(const Module& m)
   *   : id ( m.id ),
   *     num_inputs ( m.num_inputs ),
   *     num_sensors ( m.num_sensors ),
   *     name_length ( m.name_length ),
   *     name ( m.name ),
   *     inputs ( m.inputs ),
   *     sensors ( m.sensors ),
   *     next_input_id ( m.next_input_id ),
   *     next_sensor_id ( m.next_sensor_id ),
   *     owns_name ( false )
   * {} */


  Module::Module(Module&& m)
    : id ( m.id ),
      num_inputs ( m.num_inputs ),
      num_sensors ( m.num_sensors ),
      name_length ( m.name_length ),
      name ( m.name ),
      inputs ( std::move(m.inputs) ),
      sensors ( std::move(m.sensors) ),
      next_input_id ( m.next_input_id ),
      next_sensor_id ( m.next_sensor_id ),
      owns_name ( m.owns_name )
  {
    m.owns_name = false;
  }


  Module::Module(const char* _name, uint8_t _inputs_capacity, uint8_t _sensors_capacity)
    : id ( 0 ),
      num_inputs ( 0 ),
      num_sensors ( 0 ),
      name_length ( _name ? static_cast<uint8_t>(strlen(_name)) : 0 ),
      name ( _name ),
      inputs ( _inputs_capacity ),
      sensors ( _sensors_capacity ),
      next_input_id ( 0 ),
      next_sensor_id ( 0 ),
      owns_name ( false )
  {}

  /* Module::Module(uint8_t inputs_alloc, uint8_t sensors_alloc)
   *   : id ( 0 ),
   *     num_inputs ( 0 ),
   *     num_sensors ( 0 ),
   *     name_length ( 0 ),
   *     name ( nullptr ),
   *     inputs ( inputs_alloc ),
   *     sensors ( sensors_alloc ),
   *     next_input_id ( 0 ),
   *     next_sensor_id ( 0 ),
   *     owns_name ( true )
   * {} */

  Module::~Module()
  {
    reset();
  }

  void
  Module::reset()
  {
    if ( owns_name && name )
      {
	delete[] name;
	name = nullptr;
	name_length = 0;
      }
    num_inputs = 0;
    num_sensors = 0;
    inputs.clear();
    sensors.clear();

    next_input_id = 0;
    next_sensor_id = 0;
    owns_name = false;
  }


  bool Module::operator ==(const Module& o) const
  {
    return (!memcmp(this, &o, HeaderSize)) && ( name_length == 0 || (name && !memcmp(name, o.name, name_length)) ) &&
      (__extension__
       ({ bool u ( true );
	 for ( uint8_t i ( 0 ); i < num_sensors; ++i )
	   if ( sensors[i] != o.sensors[i] )
	     { u = false; break; }
	 u;
       })) &&
      (__extension__
       ({ bool u ( true );
	 for ( uint8_t i ( 0 ); i < num_inputs; ++i )
	   if ( inputs[i] != o.inputs[i] )
	     { u = false; break; }
	 u;
       }));
  }

  size_t
  Module::get_encoded_size() const
  {
    size_t out ( HeaderSize + name_length );

    for ( const ModuleInput<>& input : inputs )
      out += input.get_encoded_size();
    for ( const Sensor<>& sensor : sensors )
      out += sensor.get_encoded_size();

    return out;
  }

  EncodeResult
  Module::encode(MemoryEncodeBuffer& buf) const
  {
    /* size_t initial_offset = buf.offset; */
    
    buf.write(this, HeaderSize);

    if ( name_length && name )
      buf.write(name, name_length);

    if ( num_inputs > 0 )
      for ( const ModuleInput<>& input : inputs )
	{
	  EncodeResult r;
	  size_t offset_pre ( buf.offset );
	  if ( (r = input.encode(buf)) != EncodeResult::SUCCESS )
	    return r;

	  size_t offset_post ( buf.offset );
	  assert(offset_post - offset_pre == input.get_encoded_size());
	}

    if ( num_sensors > 0 )
      for ( const Sensor<>& sensor : sensors )
	{
	  EncodeResult r;
	  size_t offset_pre ( buf.offset );
	  if ( (r = sensor.encode(buf)) != EncodeResult::SUCCESS )
	    return r;

	  size_t offset_post ( buf.offset );
	  assert(offset_post - offset_pre == sensor.get_encoded_size());
	}


    /* DecodeBuffer db ( buf.buffer, initial_offset );
     * Module check ( Module::decode(db) ); */
  
    return EncodeResult::SUCCESS; //( check == *this ? EncodeResult::SUCCESS : EncodeResult::CONSISTENCY_ERROR );
  }


  DecodeResult
  Module::decode(DecodeBuffer& buf)
  {
    reset();
    buf.read(this, HeaderSize);
    if ( name_length > 0 )
      {
	owns_name = true;
	name = new char[name_length + 1];

	buf.read(const_cast<char*>(name), name_length);
	const_cast<char*>(name)[name_length] = '\0';
      }
    else
      { name = nullptr;
	owns_name = false; }

    if ( num_inputs > 0 )
      {
	inputs.ensure_capacity(num_inputs);
	for ( size_t i ( 0 ); i < num_inputs; ++i )
	  inputs.push(ModuleInput<>::decode_copy(buf));
      }
    if ( num_sensors > 0 )
      {
	sensors.ensure_capacity(num_sensors);
	for ( size_t i ( 0 ); i < num_sensors; ++i )
	  sensors.push(Sensor<>::decode_copy(buf));
      }
    

    if ( num_inputs && inputs.data )
      next_input_id = inputs.back().input_id + 1;
    if ( num_sensors && sensors.data )
      next_sensor_id = sensors.back().id + 1;

    return DecodeResult::SUCCESS;
  }
}
