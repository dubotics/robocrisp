#ifndef common_hh
#define common_hh 1

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <type_traits>

#include <crisp/comms/config.h>


/* Huh, why is this required?
   My best guess is that it was needed to work around some bug I encountered in Clang.
   Leaving it here for now. */
#if ! defined(GCC_VERSION_AT_LEAST) && defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GCC_VERSION_AT_LEAST(major, minor) \
((__GNUC__ > (major)) || \
 (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define GCC_VERSION_AT_LEAST(major, minor) 0
#endif

#ifndef SWIG

/** Convenient macro for fetching the ID of the currently-executing thread. */
#define THREAD_ID							\
  (__extension__							\
   ({ std::thread::id _id ( std::this_thread::get_id() );		\
     *reinterpret_cast<unsigned int*>(&_id);				\
   }))
#endif


#if defined(USE_PACKED_FIELDS) && USE_PACKED_FIELDS
#  define _FIELD_WIDTH(bits) : bits
#else
#  define _FIELD_WIDTH(bits)
#endif

#ifdef SWIG
#  define ENUM_CLASS(name,base,...)		\
  %feature("nspace") name;			\
  enum class					\
  name : base					\
  { __VA_ARGS__ }
#else
#  define ENUM_CLASS(name,base,...) enum class name : base { __VA_ARGS__ }
#endif

#include <cassert>

namespace crisp
{
  namespace comms
  {
    ENUM_CLASS(NodeRole,uint8_t,
	       MASTER = 0,
	       SLAVE);

    ENUM_CLASS(DecodeResult, uint8_t,
	       SUCCESS = 0,
	       BUFFER_UNDERFLOW,
	       INVALID_DATA
	       );

    ENUM_CLASS(EncodeResult, uint8_t,
	       SUCCESS = 0,
	       INSUFFICIENT_SPACE,
	       CONSISTENCY_ERROR,
	       STREAM_ERROR
	       );

    static inline const char*
    encode_result_to_string(EncodeResult er)
    {
      assert(er <= EncodeResult::STREAM_ERROR);
      const char* strings[] = { "Success",
                                "Insufficient space",
                                "Consistency error",
                                "Stream error" };
      return strings[static_cast<uint8_t>(er)];
    }

    static inline const char*
    decode_result_to_string(DecodeResult dr)
    {
      assert(dr <= DecodeResult::INVALID_DATA);
      const char* strings[] = { "Success",
                                "Buffer underflow",
                                "Invalid data" };
      return strings[static_cast<uint8_t>(dr)];
    }


  }
}

#endif	/* common_hh */
