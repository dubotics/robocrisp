#undef _GLIBCXX_USE_FLOAT128
#include <crisp/comms/Buffer.hh>


namespace crisp
{
  namespace comms
  {
    Buffer::~Buffer() throw ( std::runtime_error )
    {
      if ( owns_data && data )
	free_function(data);
      data = nullptr;
    }

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

    EncodeBuffer::~EncodeBuffer()
    {}

    StreamEncodeBuffer::StreamEncodeBuffer(std::streambuf* buf)
      : stream ( buf )
    {}

    StreamEncodeBuffer::~StreamEncodeBuffer()
    {}


    EncodeResult
    StreamEncodeBuffer::write(const void* buf, size_t size)
    { stream.write(reinterpret_cast<const char*>(buf), size);
      return EncodeResult::SUCCESS; }


    MemoryEncodeBuffer::MemoryEncodeBuffer(size_t size)
      : buffer ( new Buffer(size) ),
	data ( buffer->data ),
	length ( buffer->length ),
	offset ( 0 )
    {}

    MemoryEncodeBuffer::MemoryEncodeBuffer(Buffer* b)
      : buffer ( b ),
	data ( b->data ),
	length ( b->length ),
	offset ( 0 )
    {}

    MemoryEncodeBuffer::MemoryEncodeBuffer(char*& use_data, const size_t& use_length)
      : buffer ( nullptr ),
	data ( use_data ),
	length ( use_length ),
	offset ( 0 )
    {}

    MemoryEncodeBuffer::~MemoryEncodeBuffer()
    {}

    EncodeResult
    MemoryEncodeBuffer::write(const void* buf, size_t size)
    {
      if ( length - offset < size)
	return EncodeResult::INSUFFICIENT_SPACE;
      memcpy(data + offset, buf, size);
      offset += size;
      return EncodeResult::SUCCESS;
    }


  }

}
