#ifndef WP1980_LZ4_H
#define WP1980_LZ4_H
#include <stddef.h>
#include <stdint.h>
/* Exact bounded LZ4 block decoder, with caller-owned output and no allocation. */
int wp1980_lz4_decode(const uint8_t *source,size_t source_bytes,
                      uint8_t *output,size_t output_bytes);
#endif
