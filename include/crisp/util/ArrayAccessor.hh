#ifndef ArrayAccessor_hh
#define ArrayAccessor_hh 1

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
 */
template < typename _T, typename _U = std::vector<_T> >
class ArrayAccessor
{
  /* We expect the typename _U to be deduced from  */
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

#endif	/* ArrayAccessor_hh */
