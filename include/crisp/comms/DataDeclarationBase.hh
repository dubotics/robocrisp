#ifndef DataDeclarationBase_hh
#define DataDeclarationBase_hh 1

#include <crisp/comms/common.hh>
#include <crisp/comms/Buffer.hh>
#include <limits>

#ifdef SWIG
#  define __attribute__(x)
#  define constexpr const
#  define _FIELD_WIDTH(x)
#endif

#define DATA_DECLARATION_BASE_COMMON_FUNCS()				\
  EncodeResult								\
  encode(EncodeBuffer& buf) const					\
  { return crisp::comms::detail::DataDeclarationBase::encode(*this, buf); }	\
									\
  DecodeResult								\
  decode(DecodeBuffer& buf)						\
  { crisp::comms::detail::DataDeclarationBase::decode<typename std::remove_reference<Type>::type>(reinterpret_cast<TranscodeAsType*>(this), buf); \
    return DecodeResult::SUCCESS; }					\
									\
  static inline TranscodeAsType						\
  decode_copy(DecodeBuffer& buf)					\
  {  TranscodeAsType out;						\
    crisp::comms::detail::DataDeclarationBase::decode<typename std::remove_reference<Type>::type>(&out, buf); \
    return out; }							\
									\
  inline EncodeResult							\
  encode_tail(EncodeBuffer& buf) const					\
  { return crisp::comms::detail::DataDeclarationBase::encode_tail(*this, buf); } \
									\
									\
  inline DecodeResult							\
  decode_tail(DecodeBuffer& buf)					\
  { return crisp::comms::detail::DataDeclarationBase::decode_tail(*this, buf); } \
									\
  inline size_t								\
  get_tail_size() const							\
  { return crisp::comms::detail::DataDeclarationBase::get_tail_size(*this); }	\
									\
  size_t								\
  get_encoded_size() const						\
  { return crisp::comms::detail::DataDeclarationBase::get_encoded_size(*this); }


namespace crisp
{
  namespace comms
  {
  namespace detail
  {
#ifndef SWIG
    struct DataDeclarationBase
    {
      template < typename _T >
      static inline size_t
      get_encoded_size(const _T& obj)
      { return _T::TranscodeAsType::HeaderSize + obj.get_tail_size(); }


      template < typename _T >
      static inline size_t
      get_tail_size(const _T& obj)
      { return obj.width * ( obj.has_neutral_value + obj.has_minimum_value + obj.has_maximum_value ); }


      template < typename _T, typename _Enable >
      static inline EncodeResult
      encode_tail(const _T& obj, EncodeBuffer& buf);

      template < typename _T >
      static inline typename std::enable_if<!std::is_pointer<decltype(_T::neutral_value)>::value, EncodeResult>::type
      encode_tail(const _T& obj, EncodeBuffer& buf)
      {
	EncodeResult r ( EncodeResult::SUCCESS );
	if ( obj.has_neutral_value && (r = buf.write(&obj.neutral_value, obj.width)) != EncodeResult::SUCCESS )
	  return r;

	if ( obj.has_minimum_value && (r = buf.write(&obj.minimum_value, obj.width)) != EncodeResult::SUCCESS )
	  return r;

	if ( obj.has_maximum_value )
	  r = buf.write(&obj.maximum_value, obj.width);

	return r;
      }

      template < typename _T >
      static inline typename std::enable_if<std::is_pointer<decltype(_T::neutral_value)>::value, EncodeResult>::type
      encode_tail(const _T& obj, EncodeBuffer& buf)
      {
	EncodeResult r ( EncodeResult::SUCCESS );
	if ( obj.has_neutral_value && (r = buf.write(obj.neutral_value, obj.width)) != EncodeResult::SUCCESS )
	  return r;

	if ( obj.has_minimum_value && (r = buf.write(obj.minimum_value, obj.width)) != EncodeResult::SUCCESS )
	  return r;

	if ( obj.has_maximum_value )
	  r = buf.write(obj.maximum_value, obj.width);

	return r;
      }

    
      template < typename _T, typename _Enable >
      static inline EncodeResult
      encode(const _T& obj, EncodeBuffer& buf);

      /** Encode overload for generic DataDeclaration instances. */
      template < typename _T >
      static inline typename std::enable_if<std::is_same<_T, typename _T::TranscodeAsType>::value,EncodeResult>::type
      encode(const _T& obj, EncodeBuffer& buf)
      {
	EncodeResult r;
	
	if ( (r = buf.write(&obj, _T::HeaderSize)) != EncodeResult::SUCCESS )
	  return r;
	else
	  return encode_tail(obj, buf);
      }

      /** Encode overload for non-generic DataDeclaration instances. */
      template < typename _T >
      static inline typename std::enable_if<!std::is_same<_T, typename _T::TranscodeAsType>::value,EncodeResult>::type
      encode(const _T& obj, EncodeBuffer& buf)
      { typename _T::TranscodeAsType&& generic ( static_cast<typename _T::TranscodeAsType>(obj) );
	return encode(generic, buf); }


      /** DataDeclaration decode function for all declaration types.  */
      template < typename _T >
      static inline DecodeResult
      decode(typename _T::TranscodeAsType* out, DecodeBuffer& buf)
      {
	memset(out, 0, sizeof(*out));
	DecodeResult dr (  buf.read(out, _T::TranscodeAsType::HeaderSize) );

	return dr == DecodeResult::SUCCESS
	  ? decode_tail(*out, buf)
	  : dr;
      }

      template < typename _T, typename _Enable >
      static DecodeResult
      decode_tail(_T& obj, DecodeBuffer& buf);

      /** Tail decoder for generic DataDeclaration instances.  */
      template < typename _T >
      static inline typename std::enable_if<std::is_pointer<decltype(_T::neutral_value)>::value, DecodeResult>::type
      decode_tail(_T& obj, DecodeBuffer& buf)
      {
	if ( buf.length - buf.offset < get_tail_size(obj) ) return DecodeResult::BUFFER_UNDERFLOW;
	DecodeResult dr ( DecodeResult::SUCCESS );

	if ( obj.has_neutral_value )
	  dr = buf.read(obj.neutral_value, obj.width);

	if ( dr == DecodeResult::SUCCESS && obj.has_minimum_value )
	  dr = buf.read(obj.minimum_value, obj.width);

	if ( dr == DecodeResult::SUCCESS && obj.has_maximum_value )
	  dr = buf.read(obj.maximum_value, obj.width);

	return dr;
      }

      /** Tail decoder for typed DataDeclaration instances.  */
      template < typename _T >
      static inline typename std::enable_if<!std::is_pointer<decltype(_T::neutral_value)>::value, DecodeResult>::type
      decode_tail(_T& obj, DecodeBuffer& buf)
      {
	if ( buf.length - buf.offset < get_tail_size(obj) ) return DecodeResult::BUFFER_UNDERFLOW;
	DecodeResult dr ( DecodeResult::SUCCESS );

	if ( obj.has_neutral_value )
	  dr = buf.read(&obj.neutral_value, obj.width);

	if ( dr == DecodeResult::SUCCESS && obj.has_minimum_value )
	  dr = buf.read(&obj.minimum_value, obj.width);

	if ( dr == DecodeResult::SUCCESS && obj.has_maximum_value )
	  dr = buf.read(&obj.maximum_value, obj.width);

	return dr;
      }

    };
#endif

  }
  /** Generic data-type declaration.  This type is used to communicate the data types used when
   *  passing module control and sensor inputs between robot and control.
   *
   * Each ModuleInput, and each Sensor, has an associated DataDeclaration that specifies its
   * input or output data types, respectively.  These declarations are intended to specify the
   * types from/to which the appropriate data is to be en/decoded at each end.
  */
#ifdef SWIG
  template < typename _ValueType, typename _Enable = _ValueType >
  struct DataDeclarationBase
#else
  template < typename _ValueType >
  struct __attribute__ (( packed ))
  DataDeclarationBase<_ValueType, typename std::enable_if<std::is_void<_ValueType>::value, _ValueType>::type>
#endif
  {
    /** Generic types are their own transcode type. */
    typedef DataDeclarationBase<> TranscodeAsType;
    typedef DataDeclarationBase<> Type;

    DATA_DECLARATION_BASE_COMMON_FUNCS()
#ifdef SWIG
      ;
#endif
    DataDeclarationBase(DataDeclarationBase&& obj)
      : type ( obj.type ),
	width ( obj.width ),
	is_array ( obj.is_array ),
	is_signed ( obj.is_signed ),
	has_neutral_value ( obj.has_neutral_value ),
	has_minimum_value ( obj.has_minimum_value ),
	has_maximum_value ( obj.has_maximum_value ),
	array_size_width ( obj.array_size_width ),
	neutral_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	minimum_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        maximum_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
      {
	memcpy(neutral_value, obj.neutral_value, sizeof(neutral_value) + sizeof(minimum_value) + sizeof(maximum_value));
      }

    DataDeclarationBase(const DataDeclarationBase& obj)
      : type ( obj.type ),
	width ( obj.width ),
	is_array ( obj.is_array ),
	is_signed ( obj.is_signed ),
	has_neutral_value ( obj.has_neutral_value ),
	has_minimum_value ( obj.has_minimum_value ),
	has_maximum_value ( obj.has_maximum_value ),
	array_size_width ( obj.array_size_width ),
	neutral_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	minimum_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        maximum_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
      {
	memcpy(neutral_value, obj.neutral_value, sizeof(neutral_value) + sizeof(minimum_value) + sizeof(maximum_value));
      }


    inline
    DataDeclarationBase()
      : type ( DataType::UNDEFINED ),
        width ( 0 ),
        is_array ( false ),
        is_signed ( false ),
        has_neutral_value ( false ),
	has_minimum_value ( false ),
	has_maximum_value ( false ),
        array_size_width ( 0 ),
        neutral_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        minimum_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	maximum_value { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    {}

    /** Move-assignment operator. */
    DataDeclarationBase& operator =(DataDeclarationBase&& o);

    bool operator == (const DataDeclarationBase& o) const;
    inline bool
    operator != (const DataDeclarationBase& o) const
    { return ! operator ==(o); }

    template < typename _U >
    _U&
    neutral() 
    { return *(reinterpret_cast<_U*>(neutral_value)); }
    
    template < typename _U >
    _U&
    minimum() 
    { return *(reinterpret_cast<_U*>(minimum_value)); }

    template < typename _U >
    _U&
    maximum() 
    { return *(reinterpret_cast<_U*>(maximum_value)); }

    template < typename _U >
    const _U&
    neutral() const
    { return *(reinterpret_cast<const _U*>(neutral_value)); }
    
    template < typename _U >
    const _U&
    minimum() const
    { return *(reinterpret_cast<const _U*>(minimum_value)); }

    template < typename _U >
    const _U&
    maximum() const
    { return *(reinterpret_cast<const _U*>(maximum_value)); }

    /** Determine a type name to be used for an integer of the given byte width and
     *	signedness.
     *
     * @param width Type size, in bytes, of the integer type to be described.
     *
     * @param is_signed @c true if the type is signed.
     *
     * @return A string constant that can be used as a type name in generated code.
     */
    static const char*
    integer_type_name(uint8_t width, bool is_signed = false);


    /** Determine a type name to be used for a floating-point value of the given byte width.
     *
     * @param width Type size, in bytes, of the floating-point type to be described.
     *
     * @return A string constant that can be used as a type name in generated code.
     */
    static const char*
    float_type_name(uint8_t width);



    /** Determine a type name to be used for this data type.
     *
     * @return A string constant that can be used as a type name in generated code.
     */
    const char*
    type_name(TargetLanguage lang = TargetLanguage::CXX) const;


    /** Header consists of the bytes always present at the start of an encoded DataDeclaration.
     */
    static const size_t HeaderSize;

    DataType type _FIELD_WIDTH(6);		/**< Type of data being declared. */

    uint8_t width _FIELD_WIDTH(5);		/**< For non-array data declarations, the number of
						 * bytes needed to store a value for this
						 * declaration.
						 *
						 * Valid values 4 (float), 8 (double), 16 (long
						 * double) for floating-point data types, and 1
						 * (char/int8_t), 2 (short), 4 (int), 8 (long), 16
						 * (long long) for integer data types.
						 *
						 * For array types, this field shall indicate the
						 * width of each element in the array.
						 */
    bool
      is_array _FIELD_WIDTH(1),	 		/**< Indicates that the data form an array value.
						 */
      is_signed _FIELD_WIDTH(1),		/**< Flag indicating that the value should be read
						 * as signed (integer types only).  This value will
						 * always be `1` for floating-point types. */

      has_neutral_value _FIELD_WIDTH(1),	/**< Indicates that the encoded structure is
						 * followed by a value (of the type specified by
						 * `data_type`, `data_width`, and `is_signed`
						 * indicating the value to be interpreted as
						 * "neutral". */

      has_minimum_value _FIELD_WIDTH(1),	/**< Indicates that the encoded structure is
						 * followed by a value (of appropriate type)
						 * indicating the minimum permissible value under
						 * this declaration. */
      has_maximum_value _FIELD_WIDTH(1);	/**< Indicates the encoded structure is followed by
						 * a value (of appropriate type) indicating the
						 * maximum permissible value under this declaration.
						 */

    uint8_t array_size_width _FIELD_WIDTH(8);   /**< For array data declarations only, the number of
						 * bytes used to encode the encoded array length (in
						 * number of elements).  Valid values 1, 2, 4, 8,
						 * 16, corresponding to the integer data-type widths
						 * specified above.
						 */
    uint8_t
      neutral_value[16],
      minimum_value[16],
      maximum_value[16];
  };

#ifndef SWIG

  /** Data-type-declaration specialized for integer types.  This type is intended for use
   * in static declarations (e.g., in the sensor or module code itself).
   */
  template <typename _ValueType>
  struct __attribute__ (( packed, align(1) ))
  DataDeclarationBase<_ValueType, typename std::enable_if<std::is_integral<_ValueType>::value, _ValueType>::type>
  {
    /** Type that should be used when decoding values of this type.
     *  This will always be the generic DataDeclaration type.
     */
    typedef DataDeclarationBase<> TranscodeAsType;
    typedef DataDeclarationBase Type;

    DATA_DECLARATION_BASE_COMMON_FUNCS()

    static constexpr DataType type = DataType::INTEGER;
    static constexpr bool is_signed = std::is_signed<_ValueType>::value;


    DataDeclarationBase()
      : width ( sizeof(_ValueType) ),
        is_array ( false ),
	has_neutral_value ( false ),
	has_minimum_value ( false ),
	has_maximum_value ( false ),
	neutral_value ( std::numeric_limits<_ValueType>::max() ),
	minimum_value ( std::numeric_limits<_ValueType>::max() ),
	maximum_value ( std::numeric_limits<_ValueType>::max() )
    {}


    /** Constructor for use with parameterized DataDeclaration&lt;_ValueType&gt; constructor.
     */
    template < typename _ArgumentPack, typename _Enable >
      DataDeclarationBase(_ArgumentPack&& args);

    template < typename _ArgumentPack >
#ifdef __clang__
      DataDeclarationBase<_ArgumentPack, typename std::enable_if<!std::is_base_of<Type,_ArgumentPack>::value>::type>
#else
      DataDeclarationBase
#endif
      (_ArgumentPack&& args)
      : width ( args[keywords::_width | sizeof(_ValueType)] ),
        is_array ( args[keywords::_array | false] ),
        has_neutral_value ( false ),
	has_minimum_value ( false ),
	has_maximum_value ( false ),
	neutral_value ( args[keywords::_neutral | std::numeric_limits<_ValueType>::max()] ),
	minimum_value ( args[keywords::_minimum | std::numeric_limits<_ValueType>::max()] ),
	maximum_value ( args[keywords::_maximum | std::numeric_limits<_ValueType>::max()] )
    {
      const _ValueType max ( std::numeric_limits<_ValueType>::max() );
      if ( neutral_value != max )
	has_neutral_value = true;
      if ( minimum_value != max )
	has_minimum_value = true;
      if ( maximum_value != max )
	has_maximum_value = true;
    }


    /** Convert to a a generic DataDeclaration.
     */
    inline
    operator DataDeclarationBase<>() const
    {
      DataDeclarationBase<> out;
      out.type = type;
      out.width = width;
      out.is_array = is_array;
      out.is_signed = is_signed;
      out.has_neutral_value = has_neutral_value;
      out.has_minimum_value = has_minimum_value;
      out.has_maximum_value = has_maximum_value;

      if ( has_neutral_value )
	memcpy(out.neutral_value, &neutral_value, sizeof(_ValueType));
      if ( has_minimum_value )
	memcpy(out.minimum_value, &minimum_value, sizeof(_ValueType));
      if ( has_maximum_value )
	memcpy(out.maximum_value, &maximum_value, sizeof(_ValueType));
	  
      return std::move(out);
    }


    inline bool
    operator == (const DataDeclaration<>& o) const
    {
      return o.type == this->type && o.width == width && o.is_signed == is_signed &&
	o.has_neutral_value == has_neutral_value && (! has_neutral_value || o.neutral_value == o.neutral_value ) &&
	o.has_minimum_value == has_minimum_value && (! has_minimum_value || o.minimum_value == o.minimum_value ) &&
	o.has_maximum_value == has_maximum_value && (! has_maximum_value || o.maximum_value == o.maximum_value );
    }

    void set_neutral_value(_ValueType n)
    { has_neutral_value = true;
      neutral_value = n; }

    void set_minimum(_ValueType n)
    { has_minimum_value = true;
      minimum_value = n; }
      
    void set_maximum(_ValueType n)
    { has_maximum_value = true;
      maximum_value = n; }

    uint16_t width 	_FIELD_WIDTH(12);

    uint8_t array_size_width _FIELD_WIDTH(8);   /**< For array data declarations only, the number of bytes
						 * used to encode the encoded array length (in number of
						 * elements).
						 */
    bool
      is_array 		_FIELD_WIDTH(1),
      has_neutral_value _FIELD_WIDTH(1),
      has_minimum_value _FIELD_WIDTH(1),
      has_maximum_value _FIELD_WIDTH(1);

    _ValueType
      neutral_value,
      minimum_value,
      maximum_value;
  };

  /** Data-type-declaration specialized for floating-point types.
   */
  template <typename _ValueType>
  struct __attribute__ (( packed, align(1) ))
  DataDeclarationBase<_ValueType, typename std::enable_if<std::is_floating_point<_ValueType>::value, _ValueType>::type>
  {
    /** Type that should be used when decoding values of this type. */
    typedef DataDeclarationBase<> TranscodeAsType;
    typedef DataDeclarationBase Type;


    DATA_DECLARATION_BASE_COMMON_FUNCS()
#ifdef SWIG
      ;
#endif

    static constexpr DataType type = DataType::FLOAT;
    static constexpr bool is_signed = std::is_signed<_ValueType>::value;

    DataDeclarationBase()
      : is_array ( false ),
        has_neutral_value ( false ),
        has_minimum_value ( false ),
        has_maximum_value ( false ),
	neutral_value ( std::numeric_limits<_ValueType>::quiet_NaN() ),
	minimum_value ( std::numeric_limits<_ValueType>::quiet_NaN() ),
	maximum_value ( std::numeric_limits<_ValueType>::quiet_NaN() )
    {}

    /** Constructor for use with parameterized DataDeclaration&lt;_ValueType&gt; constructor.
     */
    template < typename _ArgumentPack, typename _Enable >
      DataDeclarationBase(_ArgumentPack&& args);
    template < typename _ArgumentPack >
#ifdef __clang__
    DataDeclarationBase<_ArgumentPack, typename std::enable_if<!std::is_base_of<Type,_ArgumentPack>::value>::type>
#else
    DataDeclarationBase
#endif
      (_ArgumentPack&& args)
      : width ( args[keywords::_width | sizeof(_ValueType)] ),
        is_array ( args[keywords::_array | false] ),
	has_neutral_value ( false ),
	has_minimum_value ( false ),
	has_maximum_value ( false ),
	neutral_value ( args[keywords::_neutral | std::numeric_limits<_ValueType>::quiet_NaN()] ),
	minimum_value ( args[keywords::_minimum | std::numeric_limits<_ValueType>::quiet_NaN()] ),
	maximum_value ( args[keywords::_maximum | std::numeric_limits<_ValueType>::quiet_NaN()] )
    {
      if ( ! isnan(neutral_value) )
	has_neutral_value = true;
      if ( ! isnan(minimum_value) )
	has_minimum_value = true;
      if ( ! isnan(maximum_value) )
	has_maximum_value = true;
      is_array = args[keywords::_array | false];
    }

    /** Convert to a a generic DataDeclaration. */
    inline
    operator DataDeclarationBase<>() const
    {
      DataDeclarationBase<> out;

      out.type = type;
      out.width = width;
      out.is_array = is_array;
      out.is_signed = is_signed;
      out.has_neutral_value = has_neutral_value;
      out.has_minimum_value = has_minimum_value;
      out.has_maximum_value = has_maximum_value;

      if ( has_neutral_value )
	memcpy(out.neutral_value, &neutral_value, sizeof(_ValueType));
      if ( has_minimum_value )
	memcpy(out.minimum_value, &neutral_value, sizeof(_ValueType));
      if ( has_maximum_value )
	memcpy(out.maximum_value, &maximum_value, sizeof(_ValueType));
	  
      return out;
    }

    inline bool
    operator == (const DataDeclaration<>& o) const
    {
      return o.type == this->type && o.width == width && o.is_signed == is_signed &&
	o.has_neutral_value == has_neutral_value && (! has_neutral_value || o.neutral_value == o.neutral_value ) &&
	o.has_minimum_value == has_minimum_value && (! has_minimum_value || o.minimum_value == o.minimum_value ) &&
	o.has_maximum_value == has_maximum_value && (! has_maximum_value || o.maximum_value == o.maximum_value );
    }

    void set_neutral_value(_ValueType n)
    { has_neutral_value = true;
      neutral_value = n; }

    void set_minimum(_ValueType n)
    { has_minimum_value = true;
      minimum_value = n; }
      
    void set_maximum(_ValueType n)
    { has_maximum_value = true;
      maximum_value = n; }

    uint16_t width 	_FIELD_WIDTH(12);

    uint8_t array_size_width _FIELD_WIDTH(8);   /**< For array data declarations only, the number of bytes
						 * used to encode the encoded array length (in number of
						 * elements).
						 */
    bool
      is_array 		_FIELD_WIDTH(1),
      has_neutral_value _FIELD_WIDTH(1),
      has_minimum_value _FIELD_WIDTH(1),
      has_maximum_value _FIELD_WIDTH(1);

    _ValueType
      neutral_value,
      minimum_value,
      maximum_value;
  };
#endif
  }
}

#endif	/* DataDeclarationBase_hh */
