#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "wf_sh2_bits.h"
static uint64_t comparisons;
#define CONSTANT(n) do {assert((uint32_t)WF_ZLIB_DROP(value,n)==(value>>(n)));++comparisons;}while(0)
static void check(uint32_t value) {
    for(unsigned n=0;n<32;++n){
        volatile unsigned width=n;
        assert((uint32_t)wf_zlib_drop(value,width)==(value>>n));
        assert((uint32_t)WF_ZLIB_DROP(value,width)==(value>>n));
        assert((value&(wf_zlib_power[width]-1u))==(value&((UINT32_C(1)<<n)-1u)));
        comparisons+=3;
    }
    CONSTANT(0);CONSTANT(1);CONSTANT(2);CONSTANT(3);CONSTANT(4);CONSTANT(5);CONSTANT(6);CONSTANT(7);
    CONSTANT(8);CONSTANT(9);CONSTANT(10);CONSTANT(11);CONSTANT(12);CONSTANT(13);CONSTANT(14);CONSTANT(15);
    CONSTANT(16);CONSTANT(17);CONSTANT(18);CONSTANT(19);CONSTANT(20);CONSTANT(21);CONSTANT(22);CONSTANT(23);
    CONSTANT(24);CONSTANT(25);CONSTANT(26);CONSTANT(27);CONSTANT(28);CONSTANT(29);CONSTANT(30);CONSTANT(31);
}
int main(void) {
    check(0);check(UINT32_MAX);
    for(unsigned n=0;n<32;++n){check(UINT32_C(1)<<n);check(~(UINT32_C(1)<<n));}
    uint32_t random=0x5a17c3e9u;
    for(unsigned i=0;i<65536;++i){random=random*1664525u+1013904223u;check(random);}
    for(unsigned byte=0;byte<256;++byte)for(unsigned n=0;n<=24;++n){
        assert((uint32_t)(byte*wf_zlib_power[n])==((uint32_t)byte<<n));++comparisons;
    }
    printf("PASS: %llu independent bit-operation comparisons\n",(unsigned long long)comparisons);
    return 0;
}
