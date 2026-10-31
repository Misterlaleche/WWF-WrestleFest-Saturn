/* WP2015 variation:16-bit symbol/residue slots plus byte frequencies. */
/* WP2015 private lossless token codec. No game/animation semantics live here.
 * Byte rANS state equations follow Fabian Giesen's public-domain reference
 * (rans-reference/PROVENANCE.json). The bounded token format and packed lookup
 * entries are specific to this ROM-derived sprite corpus.
 * Initial state is big-endian; symbols retain the original DEFLATE LZ pairs.
 */
#include <stdint.h>
#include <stddef.h>

#define WP2015_RANS_L (1u<<23)
#define WP2015_RANS_LITERAL_SLOTS 1024u
#define WP2015_RANS_DISTANCE_SLOTS 128u

#include "wp2015_secondary.h"
#define WP2015_RANS_TABLE_WORDS (sizeof(wp2015_rans_compact_t)/sizeof(uint32_t))
_Static_assert(sizeof(wp2015_rans_compact_t)==2620u,"compact table budget");

static const uint8_t rans_length_bits[29]={
 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
static const uint16_t rans_length_base[29]={
 3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const uint8_t rans_distance_bits[30]={
 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
static const uint16_t rans_distance_base[30]={
 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const uint16_t rans_masks[14]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191};

int wp2015_rans_tables(uint32_t *tables,unsigned words,
                       const uint16_t literal[286],const uint16_t distance[30])
{
    if(!tables||!literal||!distance||words<WP2015_RANS_TABLE_WORDS||
       ((uintptr_t)tables&3u)||literal[256])return -3;
    wp2015_rans_compact_t *context=(wp2015_rans_compact_t *)tables;
    for(unsigned model=0;model<2u;model++) {
        const uint16_t *frequency=model?distance:literal;
        unsigned symbols=model?30u:286u,slots=model?128u:1024u,offset=0;
        uint16_t *table=context->slots+(model?1024u:0u);
        uint8_t *saved=context->frequency+(model?286u:0u);
        for(unsigned symbol=0;symbol<symbols;symbol++) {
            unsigned count=frequency[symbol];
            if(count>127u||count>slots-offset)return -3;
            saved[symbol]=(uint8_t)count;
            for(unsigned residue=0;residue<count;residue++)
                table[offset++]=(uint16_t)((residue<<9)|symbol);
        }
        if(offset!=slots)return -3;
    }
    return 0;
}

/* Only indices 1..13 are used. Index zero has no shift operation. */
static const uint32_t rans_shift_multiplier[14]={0x00000000u,0x80000000u,0x40000000u,0x20000000u,0x10000000u,0x08000000u,0x04000000u,0x02000000u,0x01000000u,0x00800000u,0x00400000u,0x00200000u,0x00100000u,0x00080000u};

#if defined(__sh__)
/* GCC otherwise expands byte postincrement to an offset load on this target.
 * The source bound is checked immediately before each invocation. */
#define RANS_READ_BYTE(value) __asm__ volatile ( \
    "mov.b @%1+,%0\n\textu.b %0,%0" \
    : "=&r"(value), "+r"(input) : : "memory")
#else
#define RANS_READ_BYTE(value) ((value)=*input++)
#endif

#define RANS_REFILL() do { \
    while(state<WP2015_RANS_L) { \
        if(input==input_end)return -3; \
        uint32_t byte;RANS_READ_BYTE(byte);state=(state<<8)|byte; \
    } \
} while(0)

int wp2015_rans_decode(void *dest,unsigned *dest_bytes,const void *source,
                       unsigned source_bytes,const uint32_t *tables)
{
    if(!dest||!dest_bytes||!source||!tables||source_bytes<4u||source_bytes>4704u||
       !*dest_bytes||*dest_bytes>4096u||((uintptr_t)source&3u)||((uintptr_t)tables&3u))return -3;
    const wp2015_rans_compact_t *context=(const wp2015_rans_compact_t *)tables;
    const uint8_t *input=source,*const input_end=input+source_bytes;
    uint32_t state;
    __builtin_memcpy(&state,__builtin_assume_aligned(input,4),4);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    state=__builtin_bswap32(state);
#elif __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
#error "An explicit byte order is required"
#endif
    input+=4;
    if(state<WP2015_RANS_L||state>=0x80000000u)return -3;
    uint8_t *output=dest,*const output_start=output,*const output_end=output+*dest_bytes;
    while(output!=output_end) {
        unsigned entry=context->slots[state&1023u];
        unsigned symbol=entry&511u;
        state=(uint32_t)context->frequency[symbol]*(state>>10)+(entry>>9);
        RANS_REFILL();
        if(symbol<256u) {*output++=(uint8_t)symbol;continue;}
        if(symbol-257u>28u)return -3;
        symbol-=257u;
        unsigned length=rans_length_base[symbol],bits=rans_length_bits[symbol];
        if(bits) {
            length+=state&rans_masks[bits];state=(uint32_t)(((uint64_t)state*rans_shift_multiplier[bits])>>32);RANS_REFILL();
        }
        entry=context->slots[1024u+(state&127u)];symbol=entry&511u;
        if(symbol>=30u)return -3;
        state=(uint32_t)context->frequency[286u+symbol]*(state>>7)+(entry>>9);
        RANS_REFILL();
        unsigned offset=rans_distance_base[symbol];bits=rans_distance_bits[symbol];
        if(bits) {
            offset+=state&rans_masks[bits];state=(uint32_t)(((uint64_t)state*rans_shift_multiplier[bits])>>32);RANS_REFILL();
        }
        if(offset>(unsigned)(output-output_start))return -3;
        if(length>(unsigned)(output_end-output))return -5;
        const uint8_t *previous=output-offset;
        #if defined(__sh__)
        /* Positive length and source/output bounds were checked above.
         * The byte order is intentional: overlapping LZ copies remain exact,
         * including distance one. The delayed add runs on the final byte too. */
        __asm__ volatile (
            "1:\n\tmov.b @%1+,r0\n\tdt %2\n\tmov.b r0,@%0\n"
            "\tbf/s 1b\n\tadd #1,%0"
            : "+&r"(output), "+&r"(previous), "+&r"(length)
            : : "r0", "t", "memory");
#else
        do {*output++=*previous++;} while(--length);
#endif
    }
    return state==WP2015_RANS_L&&input==input_end?0:-3;
}
#undef RANS_REFILL
