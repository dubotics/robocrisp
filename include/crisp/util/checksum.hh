#ifndef crisp_util_checksum_hh
#define crisp_util_checksum_hh 1

#ifndef __AVR__
# include <cstdint>
# include <cstddef>
#else
# include <stdint.h>
# include <stddef.h>
#endif

namespace crisp
{
  namespace util
  {
    uint32_t crc32(const void* data, size_t length);
  }
}

#endif	/* crisp_util_checksum_hh */
