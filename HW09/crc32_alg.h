#ifndef CRC32_TABLE_H
#define CRC32_TABLE_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_alg(uint32_t c, const void *data, size_t len);

#endif
