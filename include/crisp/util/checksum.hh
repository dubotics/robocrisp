#ifndef crisp_util_checksum_hh
#define crisp_util_checksum_hh 1

#include <cstdint>
#include <cstddef>


namespace crisp
{
  namespace util
  {
    uint32_t crc32(const void* data, size_t length);
  }
}

#endif	/* crisp_util_checksum_hh */
