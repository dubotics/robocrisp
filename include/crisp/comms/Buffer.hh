#ifndef Buffer_hh
#define Buffer_hh 1

#include <cstring>
#include <cstdlib>
#include <crisp/util/RefCountedObject.hh>
#include <crisp/comms/common.hh>

namespace crisp
{
  namespace comms
  {
    /** Basic data-buffer object with intrusive-pointer semantics.
     */
    class Buffer : public crisp::util::RefCountedObject
    {
    public:
      typedef void(*FreeFunctionType)(void*);

      /** Pointer to the raw character buffer. */
      char* data;

      /** Length of the raw character buffer, in bytes */
      size_t length;

      /** Specifies that the buffer should delete/free the raw character vector at destruction. */
      bool owns_data;

      /** Function to be used to free `data` pointer, if and when appropriate. */
      FreeFunctionType free_function;

    public:

      /** Construct from pointer: does not take ownership.
       *
       * @param _data Pointer to the byte-buffer to reference.
       *
       * @param _length Size of the buffer at @p _data.
       */
      inline
      Buffer(const char* _data, size_t _length)
	: data ( const_cast<char*>(_data) ),
	  length ( _length ),
	  owns_data ( false ),
	  free_function ( nullptr )
      {}


      /** Construct from pointer, taking ownership.  Buffers constructed in this way will, when
       * destroyed, call the provided function on the data pointer.
       *
       * @param _data Pointer to the byte-buffer to take ownership of.
       *
       * @param _length Size of the buffer at @p _data.
       *
       * @param ff Function to be called to free the memory pointed to by @p _data.
       */
      inline
      Buffer(char* _data, size_t _length, FreeFunctionType&& ff)
	: data ( _data ),
	  length ( _length ),
	  owns_data ( true ),
	  free_function ( ff )
      {}

      /** Constructor to allocate new memory region of specified size.  Will free allocated buffer
       * at destruction.
       *
       * @param _length Desired buffer size.
       */
      inline
      Buffer(size_t _length)
      : data ( static_cast<char*>(malloc(_length)) ),
	length ( _length ),
	owns_data ( true ),
	free_function ( free )
      {
	assert(data != nullptr);
	memset(data, 0, _length);
      }

      /** Move constructor.  Constructs a Buffer from a temporary (rvalue) variable.
       *
       * @param b R-value reference to the value from which to construct.
       */
      inline
      Buffer(Buffer&& b)
      : data ( b.data ),
	length ( b.length ),
	owns_data ( b.owns_data ),
	free_function ( b.free_function )
      {
	b.owns_data = false;
      }

      /** Default constructor.
       *
       * @post The buffer contains no allocated data space.
       */
      inline
      Buffer()
	: data ( nullptr ),
	  length ( 0 ),
	  owns_data ( false ),
	  free_function ( nullptr )
      {}

      /** Copy constructor.  This function duplicates the data stored by the given buffer
       *	object.
       *
       * @param b Buffer to duplicate.
       */
      inline
      Buffer(const Buffer& b)
      : data ( static_cast<char*>(malloc(b.length)) ),
	length ( b.length ),
	owns_data ( true ),
	free_function ( free )
      {
	memcpy(data, b.data, length);
      }

      ~Buffer() throw ( std::runtime_error );

      /** Move-assignment operator.  This is essentially a passthrough to
       * `reset`.
       *
       * @return `*this`.
       */
      inline Buffer&
      operator =(Buffer&& b)
      { reset(std::move(b));
	return *this; }

      /** Create a new Buffer, taking ownership of the given data pointer.
       */
      static inline Buffer
      take_ownership(char* data, size_t length, FreeFunctionType&& ff)
      { return Buffer(data, length, std::move(ff)); }


      /** Create a new Buffer that references the given memory (but does not
       *	manage it).
       */
      static inline Buffer
      from_pointer(const  char* data, size_t length)
      { return Buffer(data, length); }


      /** Create a new Buffer object of the given capacity. */
      static inline Buffer
      create(size_t size)
      {  return Buffer(size); }
      

      /** Create a new Buffer as a copy of the given raw data buffer.
       */
      static inline Buffer
      copy(const char* data, size_t length)
      {
	Buffer out ( create(length) );
	memcpy(out.data, data, length);
	return out;
      }

      static inline Buffer*
      copy_new(const char* data, size_t length)
      {
	Buffer* out ( new Buffer(length) );
	memcpy(out->data, data, length);
	return out;
      }


      static inline Buffer
      copy(const Buffer& buf)
      {
	Buffer out ( create(buf.length) );
	memcpy(out.data, buf.data, buf.length);
	return out;
      }

      static inline Buffer*
      copy_new(const Buffer& buf)
      {
	Buffer* out ( new Buffer(buf.length) );
	memcpy(out->data, buf.data, buf.length);
	return out;
      }


      void
      resize(size_t capacity);

      void
      reset(Buffer&& b);
    };

    /** Immutable data buffer used for decoding values.  */
    struct DecodeBuffer
    {
      crisp::util::RefTraits<Buffer>::stored_ref buffer;

      char* const& data;
      const size_t& length;
      size_t offset;

      inline
      DecodeBuffer(size_t _size)
      : buffer ( new Buffer(_size) ),
	data ( buffer->data ),
	length ( buffer->length ),
	offset ( 0 )
      {}

      /* inline void reset(const boost::intrusive_ptr<Buffer>&& _buffer, size_t _offset = 0)
       * { buffer.reset(_buffer.get());
       *   offset = _offset; } */
      
      inline
      DecodeBuffer(boost::intrusive_ptr<Buffer> b, size_t _offset = 0)
	: buffer ( b ),
	  data ( b->data ),
	  length ( b->length ),
	  offset ( _offset )
      {}

      inline
      DecodeBuffer(char* const& _data, const size_t& _length, size_t _offset = 0)
	: buffer ( nullptr ),
	  data ( _data ),
	  length ( _length ),
	  offset ( _offset )
      {}

      template < typename _T >
      DecodeResult read(_T* dest, size_t size)
      { if ( length - offset < size ) return DecodeResult::BUFFER_UNDERFLOW;
	memcpy(const_cast<typename std::remove_const<_T>::type*>(dest), data + offset, size);
	offset += size;
	return DecodeResult::SUCCESS;
      }

      template < typename _T >
      _T read()
      { _T out; read(&out, sizeof(out)); return out; }

      char operator [](size_t i) const
      { return data[i]; }
    };

    /** Mutable buffer interface.  */
    struct EncodeBuffer
    {
      /** Copy an arbitrary number of bytes to the buffer's
       *	implementation-defined output.
       *
       * @param buf Pointer to first byte of the object to be copied.
       *
       * @param size Number of bytes to be copied.
       *
       * @return An encode-result error code.
       */
      virtual EncodeResult write(const void* buf, size_t size) = 0;


      /** Virtual destructor; does nothing.  This is provided to allow
	  polymorphic use of derived encode-buffer types. */
      virtual ~EncodeBuffer();

      /** `write` overload that determines size based on argument type.
       *
       * @tparam _T Type of object being copied to the buffer.
       *
       * @param obj Object to be copied.
       *
       * @return An encode-result error code.
       */
      template < typename _T >
      inline EncodeResult
      write(_T obj)
      { return write(&obj, sizeof(_T)); }
    };

    /** An encode-buffer for use with C++ standard library's `ostream`.
     */
    struct StreamEncodeBuffer : public EncodeBuffer
    {
      std::ostream stream;
      StreamEncodeBuffer(std::streambuf* buf);
      virtual ~StreamEncodeBuffer();

      EncodeResult write(const void* buf, size_t size);
    };

    /** An encode-buffer that writes to a memory block. */
    struct MemoryEncodeBuffer : public EncodeBuffer
    {
      crisp::util::RefTraits<Buffer>::stored_ref buffer;

      char*& data;
      const size_t& length;
      size_t offset;

      inline void reset(size_t _offset = 0)
      { offset = _offset; }

      MemoryEncodeBuffer(size_t size);
      MemoryEncodeBuffer(Buffer* b);
      MemoryEncodeBuffer(char*& use_data, const size_t& use_length);
      virtual ~MemoryEncodeBuffer();

      EncodeResult
      write(const void* buf, size_t size);

      char operator [](size_t i) const
      { return data[i]; }

      char& operator [](size_t i)
      { return data[i]; }
    };

  }
}

#endif	/* Buffer_hh */
