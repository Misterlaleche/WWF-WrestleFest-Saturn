/* Independent complete-ROM decoder/status/bank check for the host board model.
 * sfx_fixture.h and the audited PCM pack are generated privately, never committed.
 */
#include "reference_oki.h"
#include "sfx_fixture.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t *load(const char *path,size_t *bytes)
{
    FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);*bytes=(size_t)ftell(f);rewind(f);
    uint8_t *data=malloc(*bytes);assert(data&&fread(data,1,*bytes,f)==*bytes);fclose(f);return data;
}
int main(int argc,char **argv)
{
    assert(argc==3);size_t rom_bytes,gold_bytes;
    uint8_t *rom=load(argv[1],&rom_bytes),*gold=load(argv[2],&gold_bytes);
    assert(rom_bytes==0x80000);
    for(unsigned k=0;k<114;k++){
        const fixture_t *f=&fixtures[k];reference_oki_t o;reference_oki_reset(&o,rom,rom_bytes);
        reference_oki_bank(&o,f->bank);assert(reference_oki_status(&o)==0xf0);
        reference_oki_write(&o,0x80|f->id);reference_oki_write(&o,0x10);
        assert(reference_oki_status(&o)==0xf1&&o.voices[0].count==f->frames);
        for(unsigned i=0;i<f->frames;i++){
            reference_oki_tick(&o);unsigned at=f->offset+i*2;assert(at+1<gold_bytes);
            assert((uint16_t)o.voices[0].current==(uint16_t)(gold[at]*256u+gold[at+1]));
            assert((reference_oki_status(&o)&1)==(i+1<f->frames));
            assert(reference_oki_mix(&o)==o.voices[0].current);
        }
        reference_oki_tick(&o);assert(reference_oki_mix(&o)==0&&!o.fault);
    }
    /* A mid-voice bank switch must fetch new ROM bytes while preserving ADPCM
     * signal/index and the original sample start/count. It is not a restart. */
    reference_oki_t a,b;reference_oki_reset(&a,rom,rom_bytes);
    reference_oki_write(&a,0x81);reference_oki_write(&a,0x10);
    for(unsigned i=0;i<25;i++)reference_oki_tick(&a);
    b=a;reference_oki_bank(&a,1);
    unsigned pos=a.voices[0].position,start=a.voices[0].start,count=a.voices[0].count;
    int signal=a.voices[0].signal,index=a.voices[0].index;
    unsigned at=0x40000+start+pos/2,nibble=(pos&1)?rom[at]&15:rom[at]>>4;
    static const int shifts[8]={-1,-1,-1,-1,2,4,6,8};
    int step=reference_oki_steps[index],delta=step/8+((nibble&1)?step/4:0)+((nibble&2)?step/2:0)+((nibble&4)?step:0);
    signal+=(nibble&8)?-delta:delta;if(signal>2047)signal=2047;if(signal< -2048)signal=-2048;
    index+=shifts[nibble&7];if(index>48)index=48;if(index<0)index=0;
    reference_oki_tick(&a);reference_oki_tick(&b);
    assert(a.voices[0].position==pos+1&&a.voices[0].start==start&&a.voices[0].count==count);
    assert(a.voices[0].signal==signal&&a.voices[0].index==index);
    assert(a.bank_changes_while_active==1);
    reference_oki_write(&a,8);assert(reference_oki_status(&a)==0xf0&&reference_oki_mix(&a)==0);
    free(rom);free(gold);puts("PASS:114 complete PCM/status/tail sequences including aliases; active global bank changes retain decoder state");
    return 0;
}
