#ifndef DataDeclaration_hh
#define DataDeclaration_hh 1

#ifndef BOOST_PARAMETER_MAX_ARITY
#define BOOST_PARAMETER_MAX_ARITY 20
#endif	/* BOOST_PARAMETER_MAX_ARITY */

#include <boost/parameter.hpp>
#include <type_traits>
#include <cstring>
#include <cstdint>

#include <crisp/comms/common.hh>

#ifdef SWIG
#  define __attribute__(x)
#  define constexpr const
#  define _FIELD_WIDTH(x)
#endif

namespace crisp
{
  namespace comms
  {
    ENUM_CLASS(TargetLanguage, uint8_t,
	       C,		/**< C */
	       CXX		/**< C++ */
	       );

    ENUM_CLASS(DataType, uint8_t,
	       UNDEFINED,		/**< Uninitialized data-type value */
	       BOOLEAN,			/**< On-or-off value.  */
	       INTEGER,			/**< Integral data type. */
	       FLOAT,			/**< Floating-point type. */
	       STRING			/**< String-valued type (assume UTF-8 encoding if not ASCII). */
	       );

    template < typename _ValueType = void, typename _Enable = _ValueType >
    struct DataDeclaration;

    template < typename _ValueType = void, typename _Enable = _ValueType >
    struct DataDeclarationBase;

    namespace keywords
    {
      BOOST_PARAMETER_NAME(minimum)
      BOOST_PARAMETER_NAME(maximum)
	BOOST_PARAMETER_NAME(array)
	BOOST_PARAMETER_NAME(array_size_width)
	BOOST_PARAMETER_NAME(neutral)
	BOOST_PARAMETER_NAME(width)
	}

#ifdef SWIG
    template < typename _ValueType, typename _Enable = _ValueType >
    struct DataDeclaration : public DataDeclarationBase<_ValueType>
#else
    template < typename _ValueType >
    struct __attribute__ (( packed ))
			     DataDeclaration<_ValueType, typename std::enable_if<std::is_void<_ValueType>::value, _ValueType>::type>
			     : public DataDeclarationBase<_ValueType>
#endif
    {
      typedef DataDeclarationBase<_ValueType> BaseType;
      typedef DataDeclaration TranscodeAsType;
      typedef _ValueType ValueType;

      using BaseType::HeaderSize;
      using BaseType::width;

      using BaseType::get_encoded_size;
      using BaseType::encode;
      using BaseType::decode;

      using BaseType::is_array;
      using BaseType::has_neutral_value;
      using BaseType::has_minimum_value;
      using BaseType::has_maximum_value;

      using BaseType::neutral;
      using BaseType::minimum;
      using BaseType::maximum;

      DataDeclaration()
	: BaseType()
	{}

      DataDeclaration(const BaseType& obj)
	: BaseType(obj)
      {}


      DataDeclaration(DataDeclaration&& obj)
	: BaseType(std::move(obj))
      {}

      template < typename _T >
	DataDeclaration(const DataDeclaration<_T>& obj)
	: BaseType(static_cast<const BaseType&>(obj))
      {}

      DataDeclaration(const DataDeclaration& obj)
	: BaseType(obj)
      {}


      DataDeclaration(BaseType&& obj)
	: BaseType(std::move(obj))
      {}

      inline DataDeclaration&
	operator =(DataDeclaration<>&& obj)
	{ static_cast<BaseType&>(*this) = static_cast<BaseType&&>(obj);
	  return *this; }
    };

#if !defined(SWIG) && !defined(SWIG_VERSION)
    template < typename _ValueType >
    struct __attribute__ (( packed ))
    DataDeclaration<_ValueType, typename std::enable_if<!std::is_void<_ValueType>::value, _ValueType>::type>
      : public DataDeclarationBase<_ValueType>
    {
      typedef DataDeclarationBase<_ValueType> BaseType;
      typedef DataDeclaration<void> TranscodeAsType;
      typedef _ValueType ValueType;

      using BaseType::type;
      using BaseType::width;
      using BaseType::array_size_width;

      using BaseType::is_array;
      using BaseType::has_neutral_value;
      using BaseType::has_minimum_value;
      using BaseType::has_maximum_value;

      BOOST_PARAMETER_CONSTRUCTOR(DataDeclaration, (BaseType), ::crisp::comms::keywords::tag,
				  (optional
				   (neutral,(ValueType))
				   (minimum,(ValueType))
				   (maximum,(ValueType))
				   (array,(bool))
				   (width,(uint8_t))
				   (array_size_width,(uint8_t))))

	using BaseType::operator DataDeclarationBase<>;
    };
#endif
  }
}


#include <crisp/comms/DataDeclarationBase.hh>

#endif	/* DataDeclaration_hh */
