#ifndef WF_SH2_BITS_H
#define WF_SH2_BITS_H

/* Reconstructed SH-2 optimization. DEFLATE values passed here are 32-bit;
 * widths are 0..31. Multiplication by 2^(32-n), taking the high word, equals
 * a logical right shift for n>0. Zero is handled before power[32] is indexed.
 * Keep constant shifts visible to GCC, which already lowers those cheaply. */
static const unsigned wf_zlib_power[32] = {
    0x1u, 0x2u, 0x4u, 0x8u, 0x10u, 0x20u, 0x40u, 0x80u,
    0x100u, 0x200u, 0x400u, 0x800u, 0x1000u, 0x2000u, 0x4000u, 0x8000u,
    0x10000u, 0x20000u, 0x40000u, 0x80000u, 0x100000u, 0x200000u, 0x400000u, 0x800000u,
    0x1000000u, 0x2000000u, 0x4000000u, 0x8000000u, 0x10000000u, 0x20000000u, 0x40000000u, 0x80000000u
};
static inline __attribute__((always_inline)) unsigned long
wf_zlib_drop(unsigned long value,unsigned count)
{
    return count ? (unsigned long)(((unsigned long long)value * wf_zlib_power[32u-count]) >> 32) : value;
}
#define WF_ZLIB_DROP(value,count) \
    (__builtin_constant_p(count) ? ((unsigned long)(value) >> (count)) : \
     wf_zlib_drop((unsigned long)(value),(unsigned)(count)))

#endif
