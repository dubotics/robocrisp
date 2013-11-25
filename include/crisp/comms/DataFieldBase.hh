#ifndef DataFieldBase_hh
#define DataFieldBase_hh 1

#include "DataDeclarationBase.hh"
#include "APIElement.hh"

#define DATA_FIELD_COMMON_DECLS()					\
  using APIElement::symbol;						\
  using APIElement::use;						\
  using APIElement::description;					\
  using BaseType::type;							\
  using BaseType::width;						\
  using BaseType::is_array;						\
  using BaseType::is_signed;						\
  using BaseType::has_neutral_value;					\
  using BaseType::has_minimum_value;					\
  using BaseType::has_maximum_value;					\
  using BaseType::neutral_value;					\
  using BaseType::minimum_value;					\
  using BaseType::maximum_value;					\
  using BaseType::encode;						\
  using BaseType::decode;						\
  using BaseType::decode_copy;						\
  using BaseType::get_encoded_size;

namespace Robot
{
  /** Generic data field representation. */
  template < typename _ValueType >
  struct __attribute__ (( packed ))
  DataFieldBase<_ValueType, typename std::enable_if<std::is_void<_ValueType>::value, _ValueType>::type>
    : public DataDeclarationBase< _ValueType >, public APIElement
  {
    typedef DataFieldBase<> TranscodeAsType;
    typedef DataDeclarationBase<_ValueType> BaseType;

    DATA_FIELD_COMMON_DECLS()

    inline
    DataFieldBase()
      : BaseType(),
        APIElement()
    {}

    DataFieldBase(const DataFieldBase&) = delete;

    inline
    DataFieldBase(const char* _symbol, const char* _use, const char* _description,
		  BaseType&& base)
      : BaseType(base),
        APIElement(_symbol, _use, _description, true)
    {}

    inline
    DataFieldBase(DataFieldBase&& obj)
      : BaseType(static_cast<BaseType&&>(obj)),
        APIElement(static_cast<APIElement&&>(obj))
    {}


    inline bool
    operator ==(const DataFieldBase<>& other) const
    { return
	APIElement::operator ==(other) &&
	BaseType::operator ==(other);
    }

    inline bool
    operator !=(const DataFieldBase<>& other) const
    { return ! operator == (other); }


    inline
    operator DataField<>() const
    { return DataField<>(*this); }

    inline EncodeResult
    encode(EncodeBuffer& buf) const
    {
      EncodeResult r;
      return (r = APIElement::encode(buf)) == EncodeResult::SUCCESS
	? BaseType::encode(buf)
	: r;
    }

    inline DecodeResult
    decode(DecodeBuffer& buf)
    {
      DecodeResult r;
      return (r = APIElement::decode(buf)) == DecodeResult::SUCCESS
	? BaseType::decode(buf)
	: r;
    }

    static inline TranscodeAsType
    decode_copy(DecodeBuffer& buf)
    { TranscodeAsType out; out.decode(buf);
      return out; }

    inline size_t
    get_encoded_size() const
    { return APIElement::get_encoded_size() + BaseType::get_encoded_size(); }
  };

  /** Non-generic data field representation. */
  template < typename _ValueType >
  struct __attribute__ (( packed ))
  DataFieldBase<_ValueType, typename std::enable_if<!std::is_void<_ValueType>::value, _ValueType>::type>
    : private DataDeclarationBase< _ValueType >, public APIElement
  {
    typedef DataFieldBase<> TranscodeAsType;
    typedef DataDeclarationBase<_ValueType> BaseType;

    DATA_FIELD_COMMON_DECLS()

    /** Conversion-to-generic operator. */
    inline
    operator DataFieldBase<>() const
    {
      DataFieldBase<> out ( symbol, use, description, static_cast<typename BaseType::TranscodeAsType>(*this) );
      return std::move(out);
    }

    /** Copy constructor is deleted. */
    DataFieldBase(const DataFieldBase&) = delete;
    DataFieldBase(const char* _symbol, const char* _use, const char* _description,
		  BaseType&& base)
      : BaseType(base), APIElement(_symbol, _use, _description)
      {}

    /** Boost.Parameter-enabled constructor. */
    template < typename _ArgTypes >
    DataFieldBase(_ArgTypes&& args)
      : DataDeclarationBase<_ValueType>(args), APIElement(args)
      {}
  };

}

#endif	/* DataFieldBase_hh */
