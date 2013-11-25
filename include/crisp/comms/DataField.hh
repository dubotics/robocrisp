#ifndef DataField_hh
#define DataField_hh 1

#include "DataDeclaration.hh"

namespace Robot
{

  template < typename _ValueType = void, typename _Enable = _ValueType >
  struct DataField;

  template < typename _ValueType = void, typename _Enable = _ValueType >
  struct DataFieldBase;
}



#include "DataFieldBase.hh"


namespace Robot{

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

    BOOST_PARAMETER_CONSTRUCTOR(DataField<ValueType>, (BaseType), ::Robot::keywords::tag,
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


#endif	/* DataField_hh */
