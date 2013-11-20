#ifndef DataStructure_hh
#define DataStructure_hh 1

#include <support/SArray.hh>
#include <initializer_list>

#include "APIElement.hh"
#include "DataField.hh"

namespace Robot
{
  struct DataStructure : public APIElement
  {
    typedef DataStructure TranscodeAsType;
    spt::SArray<DataField<>> fields;

    DataStructure() = default;
    DataStructure(const char* _symbol, const char* _use, const char* _description,
		  const std::initializer_list<DataField<void>>& _fields);
    ~DataStructure();

    /** Free all memory dynamically allocated for this object. */
    void reset();

    /** Get the total encoded size of this structure description. */
    size_t get_encoded_size() const;

    /** Encode (serialize) this structure description into a buffer. */
    EncodeResult
    encode(EncodeBuffer& buf) const;

    /** Decode into this structure an encoded structure instance.
     * @param buf The buffer from which to decode the structure description.
     * @return `DecodeResult::SUCCESS` on successful decode, or another value corresponding to
     *   the error condition encountered.
     */
    DecodeResult
    decode(DecodeBuffer& buf);

    static inline TranscodeAsType&&
    decode_copy(DecodeBuffer& buf)
    { TranscodeAsType out; out.decode(buf);
      return std::move(out); }

    bool operator == (const DataStructure&) const;

  };
}


#endif	/* DataStructure_hh */
