/** @file
 *
 * Defines some arithmetic functions that are (arguably) useful in certain
 * situations.
 */
#ifndef crisp_util_arith_hh
#define crisp_util_arith_hh 1

namespace crisp
{
  namespace util
  {
    /** Get the negative absolute value of a number. */
    template < typename _T >
    inline typename std::enable_if<std::is_signed<_T>::value, _T>::type
    n_abs(_T v)
    { return static_cast<typename std::make_signed<_T>::type>(v < 0 ? v : -v); }

    /** Get the negative absolute value of a number. */
    template < typename _T >
    inline typename std::enable_if<std::is_unsigned<_T>::value, typename std::make_signed<_T>::type>::type
    n_abs(_T v)
    { return -static_cast<typename std::make_signed<_T>::type>(v); }


    /** Get the positive absolute value of a number. */
    template < typename _T >
    inline typename std::enable_if<std::is_unsigned<_T>::value, _T>::type
    p_abs(_T v)
    { return static_cast<typename std::make_unsigned<_T>::type>(v); }


    /** Get the positive absolute value of a number. */
    template < typename _T >
    inline typename std::enable_if<std::is_signed<_T>::value, typename std::make_unsigned<_T>::type>::type
    p_abs(_T v)
    { return static_cast<typename std::make_unsigned<_T>::type>(v < 0 ? -v : v); }


    /** Get the positive *signed* absolute value of a number. */
    template < typename _T >
    inline typename std::make_signed<_T>::type
    p_sabs(_T v)
    { return static_cast<typename std::make_signed<_T>::type>(v) < 0 ? -v : v; }
  }
}


#endif	/* crisp_util_arith_hh */
