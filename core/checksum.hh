#ifndef checksum_hh
#define checksum_hh 1

#include <cstdint>
#include <cstddef>


namespace Robot
{
  uint32_t crc32(const void* data, size_t length);
}


#endif	/* checksum_hh */
