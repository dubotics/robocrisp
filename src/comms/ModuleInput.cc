#include <crisp/comms/ModuleInput.hh>

namespace crisp
{
  namespace comms
  {
    /* ****************************************************************
     * ModuleInput
     */
    template <>
    const size_t ModuleInput<>::HeaderSize =
      offsetof(ModuleInput<>, data_type) + DataDeclaration<>::HeaderSize;

    /* generic declare without id */
    template <>
    ModuleInput<>::ModuleInput(const char* _name, DataType&& _data_type/* , ModuleInputMode _mode */)
      : input_id ( 0 ),
	/* mode ( _mode ), */
	name_length ( _name ? static_cast<uint8_t>(strlen(_name)) : 0 ),
	data_type ( std::move(_data_type) ),
	name ( const_cast<char*>(_name) ),
	owns_name ( false )
    {}
  
    /* generic declare with id */
    template <>
    ModuleInput<>::ModuleInput(const char* _name, uint8_t id, DataType&& _data_type/* , ModuleInputMode _mode */)
      : input_id ( id ),
	/* mode ( _mode ), */
	name_length ( _name ? static_cast<uint8_t>(strlen(_name)) : 0 ),
	data_type ( std::move(_data_type) ),
	name ( const_cast<char*>(_name) ),
	owns_name ( false )
    {}

    /* takes ownership of _name */
    template <>
    ModuleInput<>::ModuleInput(char* _name, uint8_t id, DataType&& _data_type/* , ModuleInputMode _mode */)
      : input_id ( id ),
	/* mode ( _mode ), */
	name_length ( _name ? static_cast<uint8_t>(strlen(_name)) : 0 ),
	data_type ( std::move(_data_type) ),
	name ( _name ),
	owns_name ( true )
    {}

    /* move constructor */
    template <>
    ModuleInput<>::ModuleInput(ModuleInput&& mi)
    : input_id ( mi.input_id ),
      /* mode ( mi.mode ), */
      name_length ( std::move(mi.name_length) ),
      data_type ( std::move(mi.data_type) ),
      name ( mi.name ),
      owns_name ( std::move(mi.owns_name) )
    {
      mi.name = nullptr;
    }

    /* copy constructor */
    /* template<>
     * ModuleInput<>::ModuleInput(const ModuleInput& mi)
     * : input_id ( mi.input_id ),
     *   /\* mode ( mi.mode ), *\/
     *   name_length ( mi.name_length ),
     *   data_type ( mi.data_type ),
     *   name ( mi.name ),
     *   owns_name ( false )
     * {} */

    template<>
    ModuleInput<>::ModuleInput()
      : input_id ( 0 ),
	/* mode ( ), */
	name_length ( 0 ),
	data_type ( ),
	name ( nullptr ),
	owns_name ( false )
    {}

    template <>
    ModuleInput<>::~ModuleInput()
    {
      reset();
    }

    NAMED_TYPED_COMMON_FUNC_DEFNS(ModuleInput<>)
  }
}
