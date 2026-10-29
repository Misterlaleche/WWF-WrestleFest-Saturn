#include "wp02_bgm_codec.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned read_all(const char *name, uint8_t **out)
{
    FILE *f = fopen(name, "rb"); assert(f);
    assert(fseek(f, 0, SEEK_END) == 0);
    long n = ftell(f); assert(n > 0);
    rewind(f); *out = malloc((size_t)n); assert(*out);
    assert(fread(*out, 1, (size_t)n, f) == (size_t)n);
    fclose(f); return (unsigned)n;
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    uint8_t *pack, *expected;
    unsigned packed = read_all(argv[1], &pack);
    unsigned bytes = read_all(argv[2], &expected);
    wp02_bgm_header_t h;
    assert(wp02_bgm_header_read(pack, packed, &h));
    assert(h.samples * 2u == bytes);
    uint8_t decoded[WP02_BGM_BLOCK_SAMPLES * 2u];
    unsigned position = WP02_BGM_HEADER_BYTES, samples = 0, blocks = 0;
    while (position < packed) {
        unsigned consumed = 0, count = 0;
        assert(wp02_bgm_block_decode(pack + position, packed - position,
                                     decoded, sizeof(decoded), &consumed, &count));
        assert(count > 0 && count <= WP02_BGM_BLOCK_SAMPLES);
        assert(consumed >= WP02_BGM_BLOCK_HEADER_BYTES);
        assert(memcmp(decoded, expected + samples * 2u, count * 2u) == 0);
        /* Truncated bodies and undersized destinations must fail safely. */
        unsigned a, b;
        assert(!wp02_bgm_block_decode(pack + position, consumed - 1u,
                                      decoded, sizeof(decoded), &a, &b));
        assert(!wp02_bgm_block_decode(pack + position, consumed,
                                      decoded, count * 2u - 1u, &a, &b));
        position += consumed; samples += count; ++blocks;
    }
    assert(position == packed && samples == h.samples && blocks == h.blocks);
    /* A17-bit remainder can need a second refill after unary consumed the
     * last cached bit. This legal boundary is not used by the current music. */
    const uint8_t edge[] = {
        0x00,0x0f,0x00,0x1e,0x02,0x11,0x80,0x00,0x7f,0xff,0x00,0x00,
        0xff,0xff,0x67,0xff,0xdc,0x00,0x05,0x40,0x00,0x7f,0xff,0xfc,
        0x00,0x05,0x00,0x01,0xbf,0xff,0xe0,0x00,0x13,0xff,0xfc,0xff,
        0xff,0x00,0x00,0x70,0x00,0x00
    };
    const int16_t edge_expected[15] = {
        -32768,32767,32767,16384,-32768,0,-32768,-16384,16384,0,
        16384,16384,0,-16384,16384
    };
    unsigned edge_bytes, edge_samples;
    assert(wp02_bgm_block_decode(edge,sizeof(edge),decoded,sizeof(decoded),
                                 &edge_bytes,&edge_samples));
    assert(edge_bytes==sizeof(edge) && edge_samples==15u);
    for(unsigned i=0;i<15u;i++)
        assert((int16_t)(((unsigned)decoded[i*2u]<<8)|decoded[i*2u+1u])==edge_expected[i]);
    uint8_t bad[WP02_BGM_BLOCK_HEADER_BYTES + 2u] = {0};
    unsigned a, b;
    assert(!wp02_bgm_block_decode(bad, sizeof(bad), decoded, sizeof(decoded), &a, &b));
    bad[1] = 1; bad[3] = 2; bad[4] = 3;
    assert(!wp02_bgm_block_decode(bad, sizeof(bad), decoded, sizeof(decoded), &a, &b));
    bad[4] = 1; bad[5] = 18;
    assert(!wp02_bgm_block_decode(bad, sizeof(bad), decoded, sizeof(decoded), &a, &b));
    pack[0] ^= 1;
    assert(!wp02_bgm_header_read(pack, packed, &h));
    free(pack); free(expected);
    puts("PASS lossless BGM blocks: complete sample equality, limits, truncation, invalid modes");
    return 0;
}
