#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include "ponesound_min.h"
#include "wp02_pcm16.h"
static unsigned is_pal;
unsigned vdp2_tvmd_tv_standard_get(void){return is_pal;}
int main(void) {
    void *p=mmap((void *)SCSP_RAM_ADDR(0),0x80000,PROT_READ|PROT_WRITE,
                 MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);
    assert(p!=(void *)-1);
    wf_sys_com_t *com=(wf_sys_com_t *)SCSP_RAM_ADDR(0x408+DRV_SYS_END);
    com->pcmCtrl=(wf_pcm_ctrl_t *)SCSP_RAM_ADDR(0xC000);
    volatile wf_pcm_ctrl_t *c=&com->pcmCtrl[2];
    c->bitDepth=1;c->sh2_permit=7;c->icsr_target=19;
    wp02_pcm16_configure(2,32920,0x44560);
    assert(c->bitDepth==0 && c->playsize==32920);
    assert(c->hiAddrBits==4 && c->loAddrBits==0x4560);
    assert(c->pitchword==0x69CE && c->bytes_per_blank==266);
    assert(c->LSA==0 && c->pan==0);
    assert(c->sh2_permit==7 && c->icsr_target==19); /* M68K ownership preserved. */
    is_pal=VDP2_TVMD_TV_STANDARD_PAL;
    wp02_pcm16_configure(2,2824,0x14000);
    assert(c->bytes_per_blank==320 && c->playsize==2824);
    assert(munmap(p,0x80000)==0);
    puts("PASS: native PCM16 descriptor, sample units, closest 8 kHz pitch, driver ownership");
    return 0;
}
