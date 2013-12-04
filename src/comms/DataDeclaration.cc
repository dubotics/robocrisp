#include <cstdint>
#include <crisp/comms/DataDeclaration.hh>

namespace crisp
{
  namespace comms
  {

    template <>
    const size_t DataDeclarationBase<>::HeaderSize =
      offsetof(DataDeclarationBase<>, neutral_value);

    template <>
    bool
    DataDeclarationBase<>::operator ==(const DataDeclarationBase<>& o) const
    {
      return type == o.type && width == o.width && is_signed == o.is_signed
	&& has_neutral_value == o.has_neutral_value && ( ! has_neutral_value || !memcmp(neutral_value, o.neutral_value, width) )
	&& has_minimum_value == o.has_minimum_value && ( ! has_minimum_value || !memcmp(minimum_value, o.minimum_value, width) )
	&& has_maximum_value == o.has_maximum_value && ( ! has_maximum_value || !memcmp(maximum_value, o.maximum_value, width) );
    }
    template<>
    DataDeclarationBase<>&
    DataDeclarationBase<>::operator =(DataDeclarationBase<>&& o)
    { type = o.type;
      width = o.width;
      array_size_width = o.array_size_width;
      is_array = o.is_array;
      is_signed = o.is_signed;
      has_neutral_value = o.has_neutral_value;
      has_minimum_value = o.has_minimum_value;
      has_maximum_value = o.has_maximum_value;
      memcpy(neutral_value, o.neutral_value, sizeof(neutral_value));
      memcpy(minimum_value, o.minimum_value, sizeof(minimum_value));
      memcpy(maximum_value, o.maximum_value, sizeof(maximum_value));
      return *this;
    }

    template <>
    const char*
    DataDeclarationBase<>::integer_type_name(uint8_t width, bool is_signed)
    {
      const char* unsigned_type_name;
      switch ( 8 * width )
	{
	case 8: unsigned_type_name = "uint8_t"; break;
	case 16: unsigned_type_name = "uint16_t"; break;
	case 32: unsigned_type_name = "uint32_t"; break;
	case 64: unsigned_type_name = "uint64_t"; break;
	default: return NULL;
	}

      /* Since the signed and unsigned type names differ only in that the unsigned names have a
	 "u" prefix, we can save some space in our initialized data section by taking advantage
	 of this. */
      return is_signed ? unsigned_type_name + 1 : unsigned_type_name;
    }

    template <>
    const char*
    DataDeclarationBase<>::float_type_name(uint8_t width)
    {
      switch ( width )
	{
	case sizeof(float): return "float";
	case sizeof(double): return "double";
	case sizeof(long double): return "long double";
	default: return NULL;
	}
    }

    template <>
    const char*
    DataDeclarationBase<>::type_name(TargetLanguage lang) const
    {
      switch ( type )
	{
#if defined(__clang__)
	case DataType::UNDEFINED: return "UNDEFINED";
	  /* C99 defines a boolean type, but it's got a funny name. */
	case DataType::BOOLEAN:   return lang == TargetLanguage::C ? "_Bool" : "bool";
	case DataType::INTEGER:   return integer_type_name(width);
	case DataType::FLOAT:     return float_type_name(width);
	case DataType::STRING:    return "char*";
#else
	  /* GCC, you suck you know that? */
	case static_cast<int>(DataType::UNDEFINED): return "UNDEFINED";
	  /* C99 defines a boolean type, but it's got a funny name. */
	case static_cast<int>(DataType::BOOLEAN): return lang == TargetLanguage::C ? "_Bool" : "bool";
	case static_cast<int>(DataType::INTEGER): return integer_type_name(width);
	case static_cast<int>(DataType::FLOAT):   return float_type_name(width);
	case static_cast<int>(DataType::STRING):  return "char*";
#endif
	}

      return NULL;
    }

  }
}
