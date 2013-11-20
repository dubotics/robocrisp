#ifndef common_hh
#define common_hh 1

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#include "config.h"


#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GCC_VERSION_AT_LEAST(major, minor) \
((__GNUC__ > (major)) || \
 (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define GCC_VERSION_AT_LEAST(major, minor) 0
#endif

#ifndef SWIG
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
#  define ENUM_CLASS(name,base,...) enum class __attribute__(( packed )) name : base { __VA_ARGS__ }
#endif

namespace Robot
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
}

namespace Robot
{
  /** Ensure that a data buffer has room for at least a given number of elements.
   *
   * @param ptr Pointer to the buffer
   *
   * @param current_capacity Current capacity of the buffer
   *
   * @param reqd_capacity Desired capacity of the buffer, in number of elements
   */
  template < typename _T >
  inline void
  ensure_capacity(_T** ptr, uint8_t* current_capacity, uint8_t reqd_capacity)
  {
    if ( *current_capacity < reqd_capacity )
      {
	uint8_t old_capacity ( *current_capacity );
	(*current_capacity) += 2 * (reqd_capacity - *current_capacity);
	*ptr = static_cast<_T*>(realloc(*ptr, *current_capacity * sizeof(_T)));
	memset((*ptr) + old_capacity, 0, sizeof(_T) * (*current_capacity - old_capacity));
      }
  }


#ifndef SWIG

  /** Get the negative absolute value of a number. */
  template < typename _T >
  inline typename std::enable_if<std::is_signed<_T>::value, _T>::type
  n_abs(_T v)
  { return static_cast<typename std::make_signed<_T>::type>(v < 0 ? v : -v); }

  template < typename _T >
  inline typename std::enable_if<std::is_unsigned<_T>::value, typename std::make_signed<_T>::type>::type
  n_abs(_T v)
  { return -static_cast<typename std::make_signed<_T>::type>(v); }


  /** Get the positive absolute value of a number. */
  template < typename _T >
  inline typename std::enable_if<std::is_unsigned<_T>::value, _T>::type
  p_abs(_T v)
  { return static_cast<typename std::make_unsigned<_T>::type>(v); }


  template < typename _T >
  inline typename std::enable_if<std::is_signed<_T>::value, typename std::make_unsigned<_T>::type>::type
  p_abs(_T v)
  { return static_cast<typename std::make_unsigned<_T>::type>(v < 0 ? -v : v); }


  /** Get the positive *signed* absolute value of a number. */
  template < typename _T >
  inline typename std::make_signed<_T>::type
  p_sabs(_T v)
  { return static_cast<typename std::make_signed<_T>::type>(v) < 0 ? -v : v; }

#endif
}
#endif	/* common_hh */
