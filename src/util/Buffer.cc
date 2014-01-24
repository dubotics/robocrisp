#include <crisp/util/Buffer.hh>
#ifdef __AVR__
# include <string.h>
#else
# include <cstddef>
# include <cstring>
#endif

namespace crisp
{
  namespace util
  {
    Buffer::Buffer(const char* _data, size_t _length)
      : data ( const_cast<char*>(_data) ),
        length ( _length ),
        owns_data ( false ),
        free_function ( nullptr )
    {}


#ifndef __AVR__
    Buffer::Buffer(char* _data, size_t _length, FreeFunctionType&& ff)
      : data ( _data ),
        length ( _length ),
        owns_data ( true ),
        free_function ( ff )
    {}

    Buffer::Buffer(size_t _length)
      : data ( static_cast<char*>(malloc(_length)) ),
	length ( _length ),
	owns_data ( true ),
	free_function ( free )
    {
      assert(data != nullptr);
      memset(data, 0, _length);
    }

    Buffer::Buffer(const Buffer& b)
      : data ( static_cast<char*>(malloc(b.length)) ),
	length ( b.length ),
	owns_data ( true ),
	free_function ( free )
    {
      memcpy(data, b.data, length);
    }
#endif

    Buffer::Buffer(Buffer&& b)
      : data ( b.data ),
	length ( b.length ),
	owns_data ( b.owns_data ),
	free_function ( b.free_function )
    {
      b.owns_data = false;
    }

    Buffer::Buffer()
      : data ( nullptr ),
        length ( 0 ),
        owns_data ( false ),
        free_function ( nullptr )
    {}

    Buffer::~Buffer()
#ifndef __AVR__
      throw ( std::runtime_error )
#endif
    {
      if ( owns_data && data )
	free_function(data);
      data = nullptr;
    }

#ifndef __AVR__
    void
    Buffer::resize(size_t capacity)
    {
      if ( ! owns_data )
	{
	  char* ndata ( static_cast<char*>(malloc(capacity * sizeof( char))) );
	  memcpy(ndata, data, std::min(length, capacity));
	  if ( length < capacity )
	    memset(ndata + length, 0, capacity - length);
	  data = ndata; length = capacity; owns_data = true;
	}
      else
	{
	  data = static_cast<char*>(realloc(data, capacity * sizeof( char)));
	  length = capacity;
	}
    }
#endif

    void
    Buffer::reset(Buffer&& b)
    { if ( data && owns_data && free_function )
	free_function(data);

      data = b.data;
      length = b.length;
      owns_data = b.owns_data;
      free_function = b.free_function;

      b.data = nullptr;
      b.owns_data = false;
      b.free_function = nullptr;
    }


#ifndef __AVR__
    Buffer
    Buffer::copy(const char* data, size_t length)
    {
      Buffer out ( create(length) );
      memcpy(out.data, data, length);
      return out;
    }

    Buffer*
    Buffer::copy_new(const char* data, size_t length)
    {
      Buffer* out ( new Buffer(length) );
      memcpy(out->data, data, length);
      return out;
    }


    Buffer
    Buffer::copy(const Buffer& buf)
    {
      Buffer out ( create(buf.length) );
      memcpy(out.data, buf.data, buf.length);
      return out;
    }

    Buffer*
    Buffer::copy_new(const Buffer& buf)
    {
      Buffer* out ( new Buffer(buf.length) );
      memcpy(out->data, buf.data, buf.length);
      return out;
    }
#endif
  }
}
