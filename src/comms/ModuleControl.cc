#include <algorithm>
#include <crisp/comms/ModuleControl.hh>
#include <crisp/comms/Module.hh>
#include <crisp/comms/Configuration.hh>

#if defined(ENABLE_ASSERT) && ENABLE_ASSERT
#  include <cassert>
#endif

namespace crisp
{
  namespace comms
  {

    const size_t ModuleControl::HeaderSize =
      offsetof(ModuleControl, module);

    const MessageType ModuleControl::Type =
      MessageType::MODULE_CONTROL;

    ModuleControl::ModuleControl()
      : module_id ( 0 ),
	input_ids ( 0 ),
	module ( nullptr ),
	values ( 0 )
    {
    }


    ModuleControl::ModuleControl(const Module* _module, size_t _values_capacity)
      : module_id ( _module->id ),
	input_ids ( 0 ),
	module ( _module ),
	values ( 0 )
    {
      if ( _values_capacity == std::numeric_limits<size_t>::max() )
	values.ensure_capacity(module->num_inputs);
      else if ( _values_capacity > 0 )
	values.ensure_capacity(_values_capacity);
    }

    ModuleControl::ModuleControl(ModuleControl&& mc)
      : module_id ( mc.module_id ),
	input_ids ( mc.input_ids ),
	module ( mc.module ),
	values ( std::move(mc.values) )
    {}

    ModuleControl::~ModuleControl()
    {}


    void
    ModuleControl::reset(const Module* _module)
    {
      module_id = 0;
      input_ids = 0;
      module = _module;
      values.clear();
    }


    ModuleControl&
    ModuleControl::operator =(ModuleControl&& mc)
    {
      module_id = mc.module_id;
      input_ids = mc.input_ids;
      module = mc.module;
      values = std::move(mc.values);
 
      return *this;
    }


    size_t
    ModuleControl::get_encoded_size() const
    {
      size_t out ( HeaderSize );
      size_t num_values ( get_num_values() );
      for ( size_t i ( 0 ); i < num_values; ++i )
	out += values[i].value.get_encoded_size();
      return out;
    }

    bool
    ModuleControl::operator ==(const ModuleControl& mc) const
    {
      if ( module_id == mc.module_id && input_ids == mc.input_ids )
	{
	  size_t num_values ( get_num_values() );
	  assert(num_values == values.size);
	  for ( size_t i ( 0 ); i < num_values; ++i )
	    if ( values[i] != mc.values[i] )
	      return false;

	  return true;
	}
      else
	return false;
    }

    ModuleControl&
    ModuleControl::clear(const char* name)
    { assert(get_num_values() == 0);
      input_ids |= 1 << (8 * sizeof(input_ids) - 1);
      const ModuleInput<>* input ( module->find_input(name) );
      if ( input )
	input_ids |= (1 << input->input_id);
      return *this;
    }

    ModuleControl&
    ModuleControl::clear(const ModuleInput<>& input)
    { assert(get_num_values() == 0);
      input_ids |= 1 << (8 * sizeof(input_ids) - 1);
      input_ids |= (1 << input.input_id);
      return *this;
    }


    const DataValue<>*
    ModuleControl::value_for(const  char* name) const
    { const ModuleInput<>* input ( module->find_input(name) );

      if ( input )
	return value_for(*input);
      else
	return NULL;
    }

    DataValue<>*
    ModuleControl::value_for(const char* name)
    { return const_cast<DataValue<>*>(const_cast<const ModuleControl*>(this)->value_for(name));
    }

    DataValue<>*
    ModuleControl::value_for(const ModuleInput<>& input)
    { return const_cast<DataValue<>*>(const_cast<const ModuleControl*>(this)->value_for(input)); }

    const DataValue<>*
    ModuleControl::value_for(const ModuleInput<>& input) const
    {
#if defined(ENABLE_ASSERT) && ENABLE_ASSERT
      size_t num_values ( get_num_values() );
      assert(num_values <= Module::MAX_INPUTS);
      assert(input.input_id < module->num_inputs);
#endif
  
      struct iid_compare { bool operator()(const ValuePair& a, const ValuePair& b)
	{ return a.input->input_id < b.input->input_id; } };

      if ( ! (input_ids & (1 << input.input_id)) )
	{ /* Add a new value-pair.  */
	  input_ids |= (1 << input.input_id);
	  values.push({ input, input.data_type });

	  std::sort(values.begin(), values.end(),
		    [](const ValuePair& a, const ValuePair& b)
		    { return a.input->input_id < b.input->input_id; });
	  return value_for(input);
	}  
      else
	for ( const ValuePair& pair : values )
	  if ( pair.input->input_id == input.input_id )
	    return &(pair.value);

      return nullptr;
    }

    EncodeResult
    ModuleControl::encode(MemoryEncodeBuffer& buf) const
    {
      buf.write(this, HeaderSize);
      if ( ! is_clear() )
	{
	  size_t num_values ( get_num_values() );
	  EncodeResult r;
	  for ( size_t i ( 0 ); i < num_values; ++i )
	    if ( (r = values[i].value.encode(buf)) != EncodeResult::SUCCESS )
	      return r;
	}

      return EncodeResult::SUCCESS;
    }

    DecodeResult
    ModuleControl::decode(DecodeBuffer& buf, const Configuration& config)
    {
      module_id = *reinterpret_cast<uint8_t*>(buf.data + buf.offset);
      reset(&config.modules[module_id]);
      buf.read(this, HeaderSize);

      if ( ! is_clear() )
	{
	  uint8_t num_values ( get_num_values() );
	  if ( num_values > 0 )
	    values.ensure_capacity(num_values);

	  for ( size_t i ( 0 ); i < 8 * sizeof(ModuleControl::input_ids); ++i )
	    if ( input_ids & (1 << i) )
	      {
#if defined(ENABLE_ASSERT) && ENABLE_ASSERT
		assert(config.modules[module_id].num_inputs > i);
#endif
		/* Construct the input-value structures in-place. */
		values.push({ static_cast<const ModuleInput<>&>(config.modules[module_id].inputs[i]),
		      DataValue<>(config.modules[module_id].inputs[i].data_type, buf) });
	      }
	}
      return DecodeResult::SUCCESS;
    }
  


    ModuleControl::ValuePair::ValuePair(ModuleControl::ValuePair&& vp)
      : input ( vp.input ),
	value ( std::move(vp.value) )
    {}

    ModuleControl::ValuePair::ValuePair(const ModuleInput<>& _input, DataValue<>&& dv)
      : input ( &_input ),
	value ( std::move(dv) )
    {}

    bool
    ModuleControl::ValuePair::operator ==(const ModuleControl::ValuePair& vp) const
    { return *input == *(vp.input) && value == vp.value; }

    bool
    ModuleControl::ValuePair::operator !=(const ModuleControl::ValuePair& vp) const
    { return ! ( *this == vp ); }

    ModuleControl::ValuePair&
    ModuleControl::ValuePair::operator =(ModuleControl::ValuePair&& vp)
    { input = vp.input;
      value = std::move(vp.value);
      return *this; }
  }
}
