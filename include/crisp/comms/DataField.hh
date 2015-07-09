#ifndef crisp_comms_DataField_hh
#define crisp_comms_DataField_hh 1

#include <crisp/comms/DataDeclaration.hh>

namespace crisp
{
  namespace comms
  {

  template < typename _ValueType = void, typename _Enable = _ValueType >
  struct DataField;

  template < typename _ValueType = void, typename _Enable = _ValueType >
  struct DataFieldBase;
}



#include <crisp/comms/DataFieldBase.hh>


namespace crisp
{
  namespace comms
  {

    /* Generic DataField implementation. */
    template < typename _ValueType >
    struct __attribute__ (( packed ))
    DataField<_ValueType, typename std::enable_if<std::is_void<_ValueType>::value, _ValueType>::type>
      : public DataFieldBase<_ValueType>
    {
      typedef DataFieldBase<> BaseType;
      typedef DataField<void,void> TranscodeAsType;
      typedef _ValueType ValueType;

      DATA_FIELD_COMMON_DECLS()
	using BaseType::operator ==;
    
      DataField() = default;
      /* DataField(const DataField&) = delete; */

      /** Move constructor for constructing a generic DataField from its base type. */
      DataField(BaseType&& obj)
	: BaseType(std::move(obj))
      {}

      /** Move constructor. */
      DataField(const DataField& obj)
	: BaseType(obj)
      {}

      DataField(DataField&& obj)
	: BaseType(std::move(obj))
      {}
    };


    /* Non-generic DataField implementation. */
    template < typename _ValueType >
    struct __attribute__ (( packed ))
    DataField<_ValueType, typename std::enable_if<!std::is_void<_ValueType>::value, _ValueType>::type>
      : public DataFieldBase<_ValueType>
    {
      typedef DataFieldBase<_ValueType> BaseType;
      typedef DataField<void> TranscodeAsType;
      typedef _ValueType ValueType;

      DATA_FIELD_COMMON_DECLS()

	BOOST_PARAMETER_CONSTRUCTOR(DataField<ValueType>, (BaseType), ::crisp::comms::keywords::tag,
				    (required
				     (symbol,(const char*))
				     (use,(const char*))
				     (description,(const char*)))
				    (optional
				     (neutral,(ValueType))
				     (minimum,(ValueType))
				     (maximum,(ValueType))
				     (array,(bool))
				     (width,(uint8_t))
				     (array_size_width,(uint8_t)))
				    )
      
	inline operator DataField<>() const
	{ return static_cast<typename BaseType::TranscodeAsType>(*this); }

    };
  }
}

#endif	/* crisp_comms_DataField_hh */
