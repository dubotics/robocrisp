#ifndef ModuleInput_hh
#define ModuleInput_hh 1

#include <crisp/comms/decl-defn-macros.hh>
#include <crisp/comms/DataDeclaration.hh>
#include <crisp/util/arith.hh>

#ifdef SWIG
#  define __attribute__(x)
#  define constexpr const
%include "decl-defn-macros.hh"
#endif

namespace crisp
{
  namespace comms
  {
  ENUM_CLASS(ModuleInputMode, uint8_t,
	     ABSOLUTE,
	     RELATIVE
	     );

#ifndef SWIG
  template < typename _T = void, typename _Enable = _T >
  struct ModuleInput;
#endif


  /* ****************************************************************
   * ModuleInput
   */

#define MODULE_INPUT_COMMON_FUNCS()					\
  bool operator ==(const ModuleInput& o) const				\
  {									\
    return input_id == o.input_id && /* mode == o.mode && */		\
      crisp::util::p_abs(name_length) == crisp::util::p_abs(o.name_length) && data_type == o.data_type && \
      (name_length == 0 || ! memcmp(name, o.name, crisp::util::p_abs(name_length))); \
  }


#ifdef SWIG
  template < typename _ValueType, typename _Enable = _ValueType >
  struct ModuleInput
#else
  template < typename _T >
  struct __attribute__ (( packed, align(1) ))
  ModuleInput<_T, typename std::enable_if<std::is_void<_T>::value, _T>::type>
#endif
  {
    typedef DataDeclaration<> DataType;
    typedef ModuleInput TranscodeAsType;

    /** Header is the bytes always present at the start of an encoded ModuleInput. */
    static const size_t HeaderSize;

    inline size_t
    get_name_length() const
    { return name_length; }

    MODULE_INPUT_COMMON_FUNCS()
    NAMED_TYPED_COMMON_FUNC_DECLS();

    ModuleInput();
/* #ifndef SWIG */
    ModuleInput(ModuleInput&& mi);
/* #endif */

    /* declare without id */
    ModuleInput(const char* _name, DataType&& _data_type/* , ModuleInputMode _mode = ModuleInputMode::ABSOLUTE */);

    /* declare with id */
    ModuleInput(const char* _name, uint8_t id, DataType&& _data_type/* , ModuleInputMode _mode = ModuleInputMode::ABSOLUTE */);
    /* takes ownership of _name */
    ModuleInput(char* _name, uint8_t id, DataType&& _data_type/* , ModuleInputMode _mode = ModuleInputMode::ABSOLUTE */);

    /* ModuleInput(const ModuleInput&) = default; */


    ~ModuleInput();

    ModuleInput&
    operator = (ModuleInput&& m)
    {
      reset();

      input_id = m.input_id;
      name_length = m.name_length;
      data_type = static_cast<typename DataType::BaseType&&>(m.data_type);
      name = m.name;
      owns_name = m.owns_name;

      m.name = nullptr;
      m.owns_name = false;
      return *this;
    }

    inline bool
    operator != (const ModuleInput& o) const
    { return ! ( *this == o ); }

    uint8_t input_id;		/**< Input ID/sequence number.
				 * Range: 0..15
				 *
				 * Note that this value is unique _only_ as an input number
				 * within the parent module!
				 */
    /* ModuleInputMode mode _FIELD_WIDTH(3); /\**< How the module will interpret values on this input.
     * 					   * Numeric range: 0..7
     * 					   *\/ */

#ifdef SWIG
%immutable;
#endif
    uint8_t name_length;	/**< Number of characters in the ASCII name string that follows
				 * this structure.  Note that the _encoded_ name will NOT be terminated by
				 * a nul (\0) character.
				 */
    DataType data_type; /**< Kind of data this input takes. */

#ifdef SWIG
%immutable;
#endif
    char* name;
    bool owns_name;
  };

#ifndef SWIG
  template < typename _T >
  struct __attribute__ (( packed, align(1) ))
  ModuleInput<_T, typename std::enable_if<!std::is_void<_T>::value, _T>::type>
  {
    typedef DataDeclaration<_T> DataType;
    typedef ModuleInput<> TranscodeAsType;


    MODULE_INPUT_COMMON_FUNCS()
    NAMED_TYPED_COMMON_FUNC_DECLS();

    inline
      ModuleInput(const char* _name, DataType _data_type/* , ModuleInputMode _mode = ModuleInputMode::ABSOLUTE */)
      : /* mode ( _mode ), */
        data_type ( _data_type ),
        name ( _name )
    {}

    inline size_t
    get_name_length() const
    { return strlen(name); }

    /** Converts to generic ModuleInput type. */
    inline operator ModuleInput<>() const
    { return { name, input_id, data_type/* , mode */ }; }


    uint8_t input_id _FIELD_WIDTH(5); 	/**< Input ID/sequence number.
				 * Range: 0..15
				 *
				 * Note that this value is unique _only_ as an input number
				 * within the parent module!
				 */
    /* ModuleInputMode mode _FIELD_WIDTH(3);	/\**< How the module will interpret values on this input.
     * 						 * Numeric range: 0..7
     * 						 *\/ */

    DataDeclaration<_T> data_type; /**< Kind of data this input takes. */

    const char* name;
  };
#endif
  }
}

#endif	/* ModuleInput_hh */
