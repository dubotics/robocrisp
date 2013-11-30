#ifndef crisp_util_Buffer_hh
#define crisp_util_Buffer_hh 1

#include <crisp/util/RefCountedObject.hh>

namespace crisp
{
  namespace util
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
      Buffer(const char* _data, size_t _length);

      /** Construct from pointer, taking ownership.  Buffers constructed in this way will, when
       * destroyed, call the provided function on the data pointer.
       *
       * @param _data Pointer to the byte-buffer to take ownership of.
       *
       * @param _length Size of the buffer at @p _data.
       *
       * @param ff Function to be called to free the memory pointed to by @p _data.
       */
      Buffer(char* _data, size_t _length, FreeFunctionType&& ff);

      /** Constructor to allocate new memory region of specified size.  Will free allocated buffer
       * at destruction.
       *
       * @param _length Desired buffer size.
       */
      Buffer(size_t _length);

      /** Move constructor.  Constructs a Buffer from a temporary (rvalue) variable.
       *
       * @param b R-value reference to the value from which to construct.
       */
      Buffer(Buffer&& b);

      /** Default constructor.
       *
       * @post The buffer contains no allocated data space.
       */
      Buffer();

      /** Copy constructor.  This function duplicates the data stored by the given buffer
       *	object.
       *
       * @param b Buffer to duplicate.
       */
      Buffer(const Buffer& b);

      /** Destructor. Frees any memory owned by the buffer.
       *
       * @throws std::runtime_error if the underlying RefCountedObject has a
       *     non-zero reference count.
       */
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

      /** Create a new Buffer, taking ownership of the given raw data buffer.
       *
       * @param data Pointer to first character of memory region to take
       *     ownership of.
       *
       * @param length Length of the memory region to take ownership of.
       *
       * @param ff Function to be used to free the data at buffer destruction:
       *
       *   - Memory allocated with `new` should be freed with `delete`
       *     (`::operator delete`)
       *
       *   - Memory allocated with `new[]` (e.g. `new char[32]`) should be freed
       *       with `delete[]` (`::operator delete[]`).
       *
       *   - Memory allocated with `malloc` or `realloc` should be freed with
       *     `free`.
       *
       * @return A Buffer object will free the given data region in its own
       *     destructor.
       */
      static inline Buffer
      take_ownership(char* data, size_t length, FreeFunctionType&& ff)
      { return Buffer(data, length, std::move(ff)); }


      /** Create a new Buffer that references the given memory (but does not
       *  manage it).
       */
      static inline Buffer
      from_pointer(const  char* data, size_t length)
      { return Buffer(data, length); }


      /** Create a new Buffer object of the given capacity. */
      static inline Buffer
      create(size_t size)
      {  return Buffer(size); }
      

      /** Create a Buffer as a copy of the given raw data buffer.
       *
       * @param data Pointer to first character of memory region to copy.
       *
       * @param length Length of the memory region to copy.
       *
       * @return A copy of @p buf.
       */
      static Buffer
      copy(const char* data, size_t length);


      /** Create a buffer as a copy of the given Buffer object.
       *
       * @param buf Buffer to copy-construct from.
       *
       * @return A copy of @p buf.
       */
      static Buffer
      copy(const Buffer& buf);


      /** Create a new Buffer on the heap as a copy of the given raw data
       *  buffer.
       *
       * @param data Pointer to first character of memory region to copy.
       *
       * @param length Length of the memory region to copy.
       *
       * @return A newly-allocated Buffer instance.
       */
      static Buffer*
      copy_new(const char* data, size_t length);


      /** Create a new Buffer on the heap as a copy of the given raw data
       *  buffer.
       *
       * @param buf Buffer to copy-construct from.
       *
       * @return A newly-allocated Buffer instance.
       */
      static Buffer*
      copy_new(const Buffer& buf);


      /** Unconditionally resize the buffer.
       *
       * @param capacity New capacity.
       */
      void
      resize(size_t capacity);


      /** Reset a buffer by freeing its resources and taking ownership of
       *  the given (Rvalue) buffer's contents.
       *
       * @param b Buffer Rvalue to take ownership of.
       */
      void
      reset(Buffer&& b);
    };
  }
}

#endif  /* crisp_util_Buffer_hh */
