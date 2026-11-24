#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "wf_jo_tile_slots.h"

static wf_jo_tile_slots_t slots;
static uint32_t rng=0x7128eae1u;
static uint32_t random_word(void)
{rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static unsigned request(uint16_t source)
{unsigned logical;assert(wf_jo_tile_slots_get(&slots,source,&logical));return logical;}
static void validate(void)
{
    unsigned keys=0;uint8_t seen[256]={0};
    for(unsigned bucket=0;bucket<1024u;bucket++)if(slots.index[bucket]){
        unsigned slot=(unsigned)slots.index[bucket]-1u;
        assert(slot<slots.count&&!seen[slot]);seen[slot]=1;++keys;
        assert(wf_jo_tile_slots_find(&slots,slots.source[slot])==bucket);
    }
    assert(keys==slots.count);
}
static void late_residents_are_not_evicted(void)
{
    wf_jo_tile_slots_init(&slots);wf_jo_tile_slots_begin(&slots);
    for(unsigned i=0;i<256u;i++)assert(request((uint16_t)i)==256u+i);
    assert(wf_jo_tile_slots_finish(&slots));validate();
    wf_jo_tile_slots_begin(&slots);
    unsigned fresh=request(256);
    /* The new tile arrives first; all but slot255 are referenced later. */
    for(unsigned i=0;i<255u;i++)assert(request((uint16_t)i)==i);
    assert(wf_jo_tile_slots_finish(&slots));validate();
    assert(wf_jo_tile_slots_resolve(&slots,fresh)==255u&&slots.source[255]==256u);
    wf_jo_tile_slots_begin(&slots);
    for(unsigned i=0;i<64u;i++)assert(request((uint16_t)i)==i);
    assert(wf_jo_tile_slots_finish(&slots)&&slots.count==256u&&slots.pending_count==0u);
    for(unsigned i=0;i<256u;i++)assert(wf_jo_tile_slots_active(&slots,i)==(i<64u));
    validate();
}
static void aborted_build_and_full_frame(void)
{
    wf_jo_tile_slots_begin(&slots);
    for(unsigned i=0;i<27u;i++)request((uint16_t)(900u+i));
    wf_jo_tile_slots_begin(&slots);validate();
    for(unsigned i=0;i<256u;i++)request((uint16_t)(10000u+i));
    unsigned out;assert(!wf_jo_tile_slots_get(&slots,60000u,&out));
    assert(request(10000u)==256u);
    assert(wf_jo_tile_slots_finish(&slots));validate();
}
static void collisions_wrap_and_many_frames(void)
{
    uint16_t source[256];unsigned logical[256];
    /* Concentrate keys across the final bucket and the wrap to bucket0. */
    unsigned count=0;
    for(unsigned key=0;key<=65535u&&count<128u;key++)
        if(wf_jo_tile_slots_hash((uint16_t)key)>=1022u)source[count++]=(uint16_t)key;
    wf_jo_tile_slots_begin(&slots);
    for(unsigned i=0;i<count;i++)logical[i]=request(source[i]);
    assert(wf_jo_tile_slots_finish(&slots));validate();
    for(unsigned i=0;i<count;i++)assert(slots.source[wf_jo_tile_slots_resolve(&slots,logical[i])]==source[i]);
    for(unsigned frame=0;frame<1800u;frame++){
        wf_jo_tile_slots_begin(&slots);count=1u+random_word()%256u;
        for(unsigned i=0;i<count;i++){
            source[i]=(uint16_t)random_word();logical[i]=request(source[i]);
            assert(request(source[i])==logical[i]);
        }
        assert(wf_jo_tile_slots_finish(&slots));validate();
        for(unsigned i=0;i<count;i++){
            unsigned slot=wf_jo_tile_slots_resolve(&slots,logical[i]);
            assert(slot<256u&&slots.source[slot]==source[i]&&wf_jo_tile_slots_active(&slots,slot));
        }
    }
}
int main(void)
{
    late_residents_are_not_evicted();aborted_build_and_full_frame();collisions_wrap_and_many_frames();
    puts("Jo tile slots: late references, inactive residency, full frames, aborts, hash deletion and epoch wrap: PASS");
    return 0;
}
