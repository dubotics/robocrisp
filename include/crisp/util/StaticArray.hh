/**
 * @file
 *
 * Defines a `StaticArray<_Tp, _N>` type similar to `std::array<_Tp,_N>`.
 */
#ifndef crisp_util_StaticArray_hh
#define crisp_util_StaticArray_hh 1

#ifndef __AVR__
# include <cstddef>
#else
# include <stddef.h>
#endif

namespace crisp
{
  namespace util
  {
    /** A statically-sized array.  Unlike std::array<_Tp,_N>, an instance of
        `StaticArray` may contain fewer elements than its declared maximum.  */
    template < typename _Tp, size_t _N >
    struct StaticArray
    {
      typedef _Tp ValueType;
      typedef _Tp* iterator;
      typedef const _Tp* const_iterator;

      /** Number of initialized elements. */
      size_t size;

      ValueType data[_N];

      /** Get the maximum number of elements an instance of this type can contain. */
      inline constexpr size_t
      capacity() const { return _N; }

      /** Check if the array is empty. */
      inline bool
      empty() { return size == 0; }

      /** Element accessor. */
      inline ValueType&
      operator [](size_t i)
      { return data[i]; }

      /** Element accessor (for `const` instances). */
      inline const ValueType&
      operator [](size_t i) const
      { return data[i]; }

      inline ValueType&
      front()
      { return data[0]; }

      inline ValueType&
      back()
      { return data[size - 1]; }

      inline const ValueType&
      front() const
      { return data[0]; }

      inline const ValueType&
      back() const
      { return data[size - 1]; }

      inline iterator
      begin()
      { return &data[0]; }

      inline const_iterator
      begin() const
      { return &data[0]; }

      inline iterator
      end()
      { return &data[size]; }

      inline const_iterator
      end() const
      { return &data[size]; }

      template < typename... Args >
      bool emplace(Args... args)
      { if ( size < _N )
          {
            data[size++] = ValueType(args...);
            return true;
          }
        else
          return false;
      }

    };
  }
}


#endif  /* crisp_util_StaticArray_hh */
