#ifndef WP1980_INFLATE_H
#define WP1980_INFLATE_H
#include <stddef.h>
#include <stdint.h>

/* Independent raw DEFLATE blocks emitted with Z_FIXED. Supports stored and
 * fixed-Huffman blocks; dynamic blocks are deliberately rejected. The caller
 * owns all memory. No allocation, previous-block dictionary or CD I/O. */
int wp1980_inflate_fixed(const uint8_t *source,size_t source_bytes,
                         uint8_t *output,size_t output_bytes);
#endif
