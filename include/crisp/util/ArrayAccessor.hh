#ifndef ArrayAccessor_hh
#define ArrayAccessor_hh 1

#include <vector>
#include <cstddef>
#include <iterator>

/** Template class for allowing array-style access -- but not deletion, insertion, etc. -- to a referenced container.
 *
 * The container must be a variable of type `_U` that provides the types
 *
 *   - _U::value_type
 *
 *   - _U::size_type
 *
 *   - _U::iterator
 *
 *   - _U::const_iterator
 *
 * and methods
 *
 *   - _U::value_type& operator [](size_t)
 *
 *   - _U::iterator begin()
 *
 *   - _U::iterator end()
 *
 *   - _U::iterator begin() const
 *
 *   - _U::iterator end() const
 *
 *   - _U::size_type size() const
 *
 *   - bool empty() const
 */
template < typename _T, typename _U = std::vector<_T> >
class ArrayAccessor
{
  static_assert(std::is_same<_T, typename _U::value_type>::value, "Value type mismatch for accessor-wrapped container");

  _U& m_ary;

public:
  typedef typename _U::value_type value_type;
  typedef typename _U::size_type size_type;
  typedef typename _U::iterator iterator;
  typedef typename _U::const_iterator const_iterator;
  
  iterator begin()
  { return m_ary.begin(); }

  const_iterator begin() const
  { return m_ary.begin(); }

  iterator end()
  { return m_ary.end(); }

  const_iterator end() const
  { return m_ary.end(); }

  inline ArrayAccessor(_U& obj) : m_ary ( obj )
  {}

  size_type
  size() const { return m_ary.size(); }

  bool
  empty() const { return m_ary.empty(); }

  inline value_type&
  operator[](size_t idx)
  {
    return m_ary[idx];
  }

  inline const value_type&
  operator[](size_t idx) const
  {
    return m_ary[idx];
  }
};

template < typename _ChildIterator >
struct DereferencedIterator;

/** Array-accessor type for use with pointer-containers, when exposing the
 * pointers themselves isn't necessary.  Use with care, since one could easily .
 */
template < typename _T, typename _Pointer, typename _U = std::vector<_Pointer> >
class DereferencedArrayAccessor
{
  _U& m_ary;

public:
  typedef DereferencedIterator<typename _U::iterator> iterator;
  typedef DereferencedIterator<typename _U::const_iterator> const_iterator;

  typedef _T value_type;
  typedef typename _U::size_type size_type;

  DereferencedArrayAccessor(_U& obj) : m_ary ( obj )
  {}

  size_type
  size() const { return m_ary.size(); }

  bool
  empty() const { return m_ary.empty(); }

  iterator begin()
  { return m_ary.begin(); }

  const_iterator begin() const
  { return m_ary.begin(); }

  iterator end()
  { return m_ary.end(); }

  const_iterator end() const
  { return m_ary.end(); }


  inline value_type&
  operator[](size_t idx)
  { return *m_ary[idx]; }

  inline const value_type&
  operator[](size_t idx) const
  { return *m_ary[idx]; }
};

template < typename _Base >
struct DereferencedIterator : _Base
{
  typedef typename std::conditional<std::is_pointer< typename _Base::value_type>::value,
                                    typename std::remove_pointer<typename _Base::value_type>::type,
                                    typename _Base::value_type::element_type>::type value_type;
  typedef typename _Base::value_type base_value_type;

  DereferencedIterator(const _Base& b) : _Base(b)
  {}
  DereferencedIterator(_Base&& b) : _Base(std::move(b))
  {}

  inline base_value_type&
  base_dereference() const
  { return _Base::operator*(); }

  inline value_type&
  operator *() const
  { return *base_dereference(); }

  inline base_value_type&
  base_dereference()
  { return _Base::operator*(); }

  inline value_type&
  operator *()
  { return *base_dereference(); }
};





#endif	/* ArrayAccessor_hh */
