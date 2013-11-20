#ifndef SArray_hh
#define SArray_hh 1

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <initializer_list>

namespace spt
{
  /** Simple array type with basic memory management.
   *
   * SArray ("Simple Array") is a bare-bones implementation of a contiguous-memory object array.
   * Because we don't expect anyone to extend SArray, no encapsulation functions are provided for the
   * internal state variables;  we aren't here to protect programmers from themselves, anyway.
   *
   * @tparam _Tp Element type.
   */
  template < typename _Tp >
  struct SArray
  {
    typedef _Tp ValueType;

    ValueType* data;
    size_t size;
    size_t capacity;
    bool owns_data;

    /**@begin STL-style accessors
     *@{
     */
    inline ValueType& front() { return *data; }
    inline ValueType& back() { return *(data + size - 1); }
    inline ValueType* begin() { return data; }
    inline ValueType* end() { return data + size; }

    inline const ValueType& front() const { return *data; }
    inline const ValueType& back() const { return *(data + size - 1); }
    inline const ValueType* begin() const { return data; }
    inline const ValueType* end() const { return data + size; }
    /**@}*/

    ValueType&
    push(ValueType&& v)
    { ensure_capacity(size + 1);
      static_assert(std::is_rvalue_reference<ValueType&&>::value, "push argument resolves to non-Rvalue reference!");
      new ( data + size++ ) ValueType(std::move(v));
      return *(data + size - 1);
    }

    template < typename... Args >
    ValueType&
    emplace(Args... args)
    { ensure_capacity(size + 1);
      new ( data + size++ ) ValueType(args...);
      return *(data + size - 1);
    }


    /** Ensure that the array can hold at least the given number of items.
     *
     * @param reqd_capacity Minimum desired capacity.
     */
    inline void
    ensure_capacity(size_t reqd_capacity)
    { if ( capacity < reqd_capacity )
	{
	  size_t old_capacity ( capacity );

	  /* Unless current capacity is zero, actual added capacity is twice the difference
	     (desired - current).  I wonder if there are any use cases where that will lead to
	     ridiculous memory consumption (probably not).  */
	  if ( old_capacity > 0 )
	    capacity += 2 * (reqd_capacity - capacity);
	  else
	    capacity = reqd_capacity;

	  data = static_cast<ValueType*>(realloc(data, capacity * sizeof(ValueType)));
	  memset(data + old_capacity, 0, sizeof(ValueType) * (capacity - old_capacity));
	} }

    /** Clear the contents of the array, but keep the allocated memory. */
    inline void
    clear()
    {
      if ( data && owns_data )
	{
	  for ( size_t i = 0; i < size; ++i )
	    data[i].~ValueType();
	  size = 0;
	}
    }

    inline SArray() noexcept
      : data ( nullptr ),
      size ( 0 ),
      capacity ( 0 ),
      owns_data ( true )
    {}


    inline SArray(size_t s)
    : data ( s > 0 ? static_cast<ValueType*>(malloc(s * sizeof(ValueType))) : nullptr),
      size ( 0 ),
      capacity ( s ),
      owns_data ( true )
    {
      if ( data )
	memset(data, 0, capacity * sizeof(ValueType));
    }


    inline ~SArray()
    {
      if ( data && owns_data )
	{
	  clear();
	  if ( data ) free(data);
	  data = nullptr;
	  owns_data = false;
	}
    }

    /** Move constructor. */
    inline SArray(SArray&& sa)
    : data ( sa.data ),
      size ( sa.size ),
      capacity ( sa.capacity ),
      owns_data ( sa.owns_data )
    {
      sa.data = nullptr;
    }

    /** Constructor for creating an SArray from a std::initializer_list of the same type.
     *
     * @param _data Initializer list to copy.
     */
    inline SArray(const std::initializer_list<ValueType>& _data)
      : SArray(_data.size())
    {
      for ( const ValueType& f : _data )
	push(const_cast<ValueType&&>(std::move(f)));
    }

    /*SArray(const SArray& s)
    : data ( s.data ),
      size ( s.size ),
      capacity ( s.capacity ),
      owns_data ( false )
    {}

    SArray& operator=(const SArray& s)
    {
      data = s.data;
      size = s.size;
      capacity = s.capacity;
      owns_data = false;
      return *this;
      }*/


    /** Rvalue assignment operator. */
    inline SArray&
    operator = (SArray&& sa)
    {
      data = sa.data;
      size = sa.size;
      capacity = sa.capacity;
      owns_data = sa.owns_data;

      sa.data = nullptr;
      sa.owns_data = false;

      return *this;
    }


    inline _Tp&
    operator[](size_t n)
    { return data[n]; }

    inline const _Tp&
    operator[](size_t n) const
    { return data[n]; }
  };
}

#endif	/* defined(SArray_hh) */
