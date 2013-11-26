#ifndef decl_defn_macros_hh
#define decl_defn_macros_hh 1

#include <cstdio>
#include <crisp/comms/Buffer.hh>

#define NAMED_TYPED_COMMON_FUNC_INLINES()				\
  inline void								\
  reset()								\
  {									\
    if ( owns_name && name ) delete[] name;				\
    name = nullptr;							\
    owns_name = false;							\
  }									\
									\
  inline size_t								\
  get_encoded_size() const						\
  { return TranscodeAsType::HeaderSize + get_tail_size(); }		\
									\
  inline size_t								\
  get_tail_size() const							\
  { return get_name_length() + data_type.get_tail_size(); }		\
									\
									\
  inline EncodeResult							\
  encode(MemoryEncodeBuffer& buf) const					\
  {									\
    /* Here be dragons! */						\
    using Type =							\
      typename std::remove_reference<typename std::remove_const<decltype(*this)>::type>::type; \
    typename std::conditional<std::is_same<Type, TranscodeAsType>::value, \
			      const Type, const TranscodeAsType>::type& \
      generic ( *this );						\
									\
    if ( buf.length - buf.offset < get_encoded_size() )			\
      return EncodeResult::INSUFFICIENT_SPACE;				\
									\
    buf.write(&generic, TranscodeAsType::HeaderSize);			\
									\
    data_type.encode_tail(buf);						\
									\
    if ( generic.name_length && generic.name )				\
      buf.write(generic.name, generic.name_length);			\
									\
									\
    return EncodeResult::SUCCESS;					\
  }									\
									\
  inline DecodeResult							\
  decode(DecodeBuffer& buf)						\
  {									\
    reset();								\
    *this = decode_copy(buf);						\
    return DecodeResult::SUCCESS;					\
  }									\
									\
  static inline TranscodeAsType						\
  decode_copy(DecodeBuffer& buf)					\
  {									\
    TranscodeAsType out;						\
									\
    buf.read(&out, TranscodeAsType::HeaderSize);			\
    out.data_type.decode_tail(buf);					\
									\
    if ( out.name_length )						\
      {									\
	out.owns_name = true;						\
	out.name = new char[out.name_length + 1];			\
	buf.read(out.name, out.name_length);				\
	out.name[out.name_length] = '\0';				\
      }									\
									\
    return out;								\
  }

#define NAMED_TYPED_COMMON_FUNC_DECLS()					\
  inline void								\
  reset()								\
  {									\
    if ( owns_name && name ) delete[] name;				\
    name = nullptr;							\
    owns_name = false;							\
  }									\
									\
  inline size_t								\
  get_encoded_size() const						\
  { return TranscodeAsType::HeaderSize + get_tail_size(); }		\
									\
  inline size_t								\
  get_tail_size() const							\
  { return get_name_length() + data_type.get_tail_size(); }		\
									\
									\
  EncodeResult								\
  encode(MemoryEncodeBuffer& buf) const;				\
									\
  DecodeResult								\
  decode(DecodeBuffer& buf);						\
									\
  static TranscodeAsType						\
  decode_copy(DecodeBuffer& buf)

#define NAMED_TYPED_COMMON_FUNC_DEFNS(_class)				\
  template<>								\
  EncodeResult								\
  _class::encode(MemoryEncodeBuffer& buf) const				\
  {									\
    /* Here be dragons! */						\
    using Type =							\
      typename std::remove_reference<typename std::remove_const<decltype(*this)>::type>::type; \
    typename std::conditional<std::is_same<Type, TranscodeAsType>::value, \
			      const Type, const TranscodeAsType>::type& \
      generic ( *this );						\
									\
    if ( buf.length - buf.offset < get_encoded_size() )			\
      return EncodeResult::INSUFFICIENT_SPACE;				\
									\
    buf.write(&generic, TranscodeAsType::HeaderSize);			\
									\
    data_type.encode_tail(buf);						\
									\
    if ( generic.name_length && generic.name )				\
      buf.write(generic.name, generic.name_length);			\
									\
    return EncodeResult::SUCCESS;					\
  }									\
									\
  template<>								\
  _class::TranscodeAsType						\
  _class::decode_copy(DecodeBuffer& buf)				\
  {									\
    TranscodeAsType out;						\
									\
    buf.read(&out, TranscodeAsType::HeaderSize);			\
									\
    out.data_type.decode_tail(buf);					\
									\
    if ( out.name_length )						\
      {									\
	out.owns_name = true;						\
	out.name = new char[out.name_length + 1];			\
	buf.read(out.name, out.name_length);				\
	out.name[out.name_length] = '\0';				\
      }									\
									\
    return out;								\
  }									\
									\
  template<>								\
  DecodeResult								\
  _class::decode(DecodeBuffer& buf)					\
  {									\
    reset();								\
    *this = decode_copy(buf);						\
    return DecodeResult::SUCCESS;					\
  }



#endif	/* decl_defn_macros_hh */
