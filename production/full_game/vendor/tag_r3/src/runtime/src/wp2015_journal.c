/* Compact observer, with an explicit boundary before the secondary sprite lease.
 * Each committed record preserves the complete32-bit actor hash and16-bit
 * VBlank delta. Frames are consecutive; any violation freezes the observer.
 * Header32bytes;9000records of6bytes;16untouched alignment bytes.
 */
#include "wp2015_journal.h"
_Static_assert(sizeof(uint32_t)==4,"journal word size");
_Static_assert(_Alignof(wf_runtime_t)>=4,"runtime alignment");
_Static_assert(offsetof(wf_runtime_t,actors)%4==0,"actor base alignment");
_Static_assert(sizeof(wf_actor_raw_t)==268,"actor checksum contract");
_Static_assert(32+6*WP2015_JOURNAL_CAPACITY<=WP2015_JOURNAL_BYTES,"journal lease overflow");
#ifdef WP2015_JOURNAL_TEST
extern volatile uint32_t *wp2015_journal_test_memory;
#define JOURNAL wp2015_journal_test_memory
#else
#define JOURNAL ((volatile uint32_t *)WP2015_JOURNAL_ADDRESS)
#endif
static uint32_t actor_hash(const wf_runtime_t *rt){
    uint32_t h=2166136261u;
    for(unsigned i=0;i<4;i++){
        const unsigned slot=(i&1u)|((i&2u)<<1);
        const uint8_t *p=rt->actors[slot].b;
        for(unsigned n=0;n<sizeof(wf_actor_raw_t);n+=4){
            uint32_t word;
            __builtin_memcpy(&word,__builtin_assume_aligned(p+n,4),4);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            word=__builtin_bswap32(word);
#elif __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
#error "journal requires explicit byte order"
#endif
            h=(h^word)*16777619u;
        }
    }
    return h;
}
void wp2015_journal_record(const wf_runtime_t *rt,uint32_t vblank){
    volatile uint32_t *const journal=JOURNAL;
    if(journal[3])return;
    const uint32_t count=journal[2];
    if(count>=WP2015_JOURNAL_CAPACITY){journal[3]=1;return;}
    if(rt->g.frame_counter!=journal[4]+count){journal[3]=2;return;}
    const uint32_t delta=count?vblank-journal[7]:0;
    if(delta>65535u){journal[3]=4;return;}
    if(count && !delta){journal[3]=8;return;}
    const uint32_t hash=actor_hash(rt);
    volatile uint16_t *const entry=(volatile uint16_t *)(journal+8)+count*3;
    entry[0]=(uint16_t)delta;entry[1]=(uint16_t)(hash>>16);entry[2]=(uint16_t)hash;
    journal[6]=rt->g.frame_counter;journal[7]=vblank;
    journal[2]=count+1; /* Commit last; an interrupted record is never counted. */
}
void wp2015_journal_begin(const wf_runtime_t *rt,uint32_t vblank){
    volatile uint32_t *const journal=JOURNAL;
    journal[0]=0;journal[1]=2;journal[2]=0;journal[3]=0;
    journal[4]=rt->g.frame_counter;journal[5]=vblank;
    journal[6]=rt->g.frame_counter;journal[7]=vblank;
    journal[0]=WP2015_JOURNAL_MAGIC;
    wp2015_journal_record(rt,vblank);
}
