#ifndef crisp_comms_Buffer_hh
#define crisp_comms_Buffer_hh 1

#include <cstring>
#include <cstdlib>
#include <crisp/util/Buffer.hh>
#include <crisp/comms/common.hh>

namespace crisp
{
  namespace comms
  {
    /** Immutable data buffer used for decoding values.  */
    struct DecodeBuffer
    {
      using Buffer = crisp::util::Buffer;
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
      using Buffer = crisp::util::Buffer;
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

#endif	/* crisp_comms_Buffer_hh */
