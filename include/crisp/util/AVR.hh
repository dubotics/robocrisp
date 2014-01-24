/**
 * @file
 *
 * Fixes and additional code necessary for compiling for the Atmel AVR
 * architecture, for which no standard C++ library is available by default.
 */
#ifndef crisp_util_AVR_hh
#define crisp_util_AVR_hh 1

namespace std
{
  /* Quick 'n dirty implementation of std::remove_reference. */
  template < typename >
  struct remove_reference;

  template < typename _Tp >
  struct remove_reference<_Tp&&>
  {
    typedef _Tp type;
  };

  template < typename _Tp >
  struct remove_reference<_Tp&>
  {
    typedef _Tp type;
  };


  /* Copied from the libstdc++ 4.8.2 `bits/move.h`. */
  /**
   *  @brief  Convert a value to an rvalue.
   *  @param  __t  A thing of arbitrary type.
   *  @return The parameter cast to an rvalue-reference to allow moving it.
   */
  template<typename _Tp>
  constexpr typename std::remove_reference<_Tp>::type&&
  move(_Tp&& __t) noexcept
  { return static_cast<typename std::remove_reference<_Tp>::type&&>(__t); }
}

#endif  /* crisp_util_AVR_hh */
