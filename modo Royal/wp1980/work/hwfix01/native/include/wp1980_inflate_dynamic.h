#ifndef WP1980_INFLATE_DYNAMIC_H
#define WP1980_INFLATE_DYNAMIC_H
#include <stddef.h>
#include <stdint.h>

/* Independent bounded raw RFC1951 decoder. Stored, fixed and dynamic blocks
 * are accepted; a zlib/gzip wrapper or preset dictionary is not accepted.
 * Both buffers are caller-owned and must be disjoint. Sizes are <=65536.
 * Returns0 only for exact output size and complete compressed-byte use.
 * No allocation, recursion, global workspace or previous-stream history. */
int wp1980_inflate_dynamic(const uint8_t *source,size_t source_bytes,
                           uint8_t *output,size_t output_bytes);
#endif
