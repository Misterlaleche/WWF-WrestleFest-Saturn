#include "wp02_bgm_codec.h"
#include <string.h>

static uint16_t be16(const uint8_t *p)
{ return (uint16_t)((unsigned)p[0] * 256u + p[1]); }
static uint32_t be32(const uint8_t *p)
{ return ((uint32_t)be16(p) << 16) | be16(p + 2); }

bool wp02_bgm_header_read(const uint8_t *s, unsigned bytes, wp02_bgm_header_t *out)
{
    if (!s || !out || bytes < WP02_BGM_HEADER_BYTES ||
        memcmp(s, "WP02BGM1", 8) || be32(s + 8) != 44100u ||
        be32(s + 16) != WP02_BGM_BLOCK_SAMPLES || be32(s + 28) != 0u)
        return false;
    out->samples = be32(s + 12);
    out->blocks = be32(s + 20);
    out->payload_bytes = be32(s + 24);
    return out->samples > 0u && out->samples <= 44100u * 60u &&
           out->blocks == (out->samples + WP02_BGM_BLOCK_SAMPLES - 1u) /
                          WP02_BGM_BLOCK_SAMPLES &&
           out->payload_bytes == bytes - WP02_BGM_HEADER_BYTES;
}

typedef struct {
    const uint8_t *p, *end;
    uint32_t word;
    unsigned available;
} reader_t;

/* SH-2 has no general register-count shift instruction. A variable C shift
 * invokes a libgcc helper. Refills use a hardware multiply instead; the
 * residual decoders below specialize k so every hot-path shift is constant. */
static const uint32_t refill_multiplier[17] = {
    65536u,32768u,16384u,8192u,4096u,2048u,1024u,512u,256u,
    128u,64u,32u,16u,8u,4u,2u,1u
};
static inline bool refill(reader_t *r)
{
    if (r->available > 16u || r->p == r->end) return false;
    uint32_t next = (uint32_t)*r->p++ << 8;
    unsigned added = 8;
    if (r->p != r->end) { next |= *r->p++; added = 16; }
    r->word |= next * refill_multiplier[r->available];
    r->available += added;
    return true;
}

#define DEFINE_DECODER(ORDER, K) \
static bool decode_o##ORDER##_k##K(const uint8_t *s, unsigned payload, \
                                  uint8_t *out, unsigned n) \
{ \
    reader_t r = {s + WP02_BGM_BLOCK_HEADER_BYTES, \
                  s + WP02_BGM_BLOCK_HEADER_BYTES + payload, 0, 0}; \
    int32_t before = (int16_t)be16(s + 6), last = before; \
    out[0] = s[6]; out[1] = s[7]; \
    if (ORDER == 2 && n > 1u) { \
        last = (int16_t)be16(s + 8); out[2] = s[8]; out[3] = s[9]; \
    } \
    for (unsigned i = ORDER; i < n; ++i) { \
        unsigned q = 0; \
        for (;;) { \
            if (!r.available && !refill(&r)) return false; \
            unsigned bit = r.word >> 31; \
            r.word <<= 1; --r.available; \
            if (bit) break; \
            if (++q > (262140u >> K)) return false; \
        } \
        uint32_t u = q << K; \
        if (K != 0) { \
            if ((int)r.available < K && !refill(&r)) return false; \
            if (K == 17 && r.available < 17u && !refill(&r)) return false; \
            if ((int)r.available < K) return false; \
            u |= r.word >> ((32u - K) & 31u); \
            r.word <<= K; r.available -= K; \
        } \
        if (u > 262140u) return false; \
        int32_t residual = (int32_t)(u >> 1) ^ -(int32_t)(u & 1u); \
        int32_t v = (ORDER == 1 ? last : 2 * last - before) + residual; \
        if (v < -32768 || v > 32767) return false; \
        out[i * 2u] = (uint8_t)((uint16_t)v >> 8); \
        out[i * 2u + 1u] = (uint8_t)v; \
        before = last; last = v; \
    } \
    unsigned used = (unsigned)(r.p - (s + WP02_BGM_BLOCK_HEADER_BYTES)) * 8u - r.available; \
    return (used + 7u) / 8u == payload; \
}
#define DEFINE_K(K) DEFINE_DECODER(1,K) DEFINE_DECODER(2,K)
DEFINE_K(0) DEFINE_K(1) DEFINE_K(2) DEFINE_K(3) DEFINE_K(4) DEFINE_K(5)
DEFINE_K(6) DEFINE_K(7) DEFINE_K(8) DEFINE_K(9) DEFINE_K(10) DEFINE_K(11)
DEFINE_K(12) DEFINE_K(13) DEFINE_K(14) DEFINE_K(15) DEFINE_K(16) DEFINE_K(17)

typedef bool (*decode_fn)(const uint8_t *, unsigned, uint8_t *, unsigned);
#define DECODER_ROW(O) {decode_o##O##_k0,decode_o##O##_k1,decode_o##O##_k2, \
    decode_o##O##_k3,decode_o##O##_k4,decode_o##O##_k5,decode_o##O##_k6, \
    decode_o##O##_k7,decode_o##O##_k8,decode_o##O##_k9,decode_o##O##_k10, \
    decode_o##O##_k11,decode_o##O##_k12,decode_o##O##_k13,decode_o##O##_k14, \
    decode_o##O##_k15,decode_o##O##_k16,decode_o##O##_k17}
static const decode_fn decoders[2][18] = {DECODER_ROW(1), DECODER_ROW(2)};

bool wp02_bgm_block_decode(const uint8_t *s, unsigned bytes,
                          uint8_t *out, unsigned capacity,
                          unsigned *consumed, unsigned *samples)
{
    if (!s || !out || !consumed || !samples || bytes < WP02_BGM_BLOCK_HEADER_BYTES)
        return false;
    unsigned n = be16(s), payload = be16(s + 2), mode = s[4], k = s[5];
    if (n == 0u || n > WP02_BGM_BLOCK_SAMPLES || n * 2u > capacity ||
        payload > n * 2u || payload + WP02_BGM_BLOCK_HEADER_BYTES > bytes ||
        mode > 2u || k > 17u || be16(s + 10) != 0u)
        return false;
    if (mode == 0u) {
        if (payload != n * 2u || k != 0u) return false;
        memcpy(out, s + WP02_BGM_BLOCK_HEADER_BYTES, payload);
    } else {
        if (!decoders[mode - 1u][k](s, payload, out, n)) return false;
    }
    *consumed = WP02_BGM_BLOCK_HEADER_BYTES + payload;
    *samples = n;
    return true;
}
