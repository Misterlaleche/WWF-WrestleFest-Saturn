#include "wp02_pcm16.h"
#include "ponesound_min.h"
#include <yaul.h>

/* Same shared descriptor/owner as the pinned 8732-byte poneSound driver.
 * Yaul's corresponding ponesound.h defines native PCM16 as bitDepth=0 and
 * playsize in samples. Upstream load_16bit_pcm doubles bytes_per_blank.
 * This helper never writes a physical SCSP slot or KYONEX. */
static wf_sys_com_t * const com =
    (wf_sys_com_t *)SCSP_RAM_ADDR(0x408 + DRV_SYS_END);

void wp02_pcm16_configure(unsigned voice, unsigned samples, uint32_t offset)
{
    volatile wf_pcm_ctrl_t *c = &com->pcmCtrl[voice];
    c->hiAddrBits = (uint16_t)(offset >> 16);
    c->loAddrBits = (uint16_t)offset;
    c->LSA = 0;
    c->playsize = (uint16_t)samples;
    /* Nearest representable SCSP rate to 8000 Hz: OCT=-3, FNS=462.
     * 44100/8 * (1 + 462/1024) = 7999.5849609375 Hz. */
    c->pitchword = 0x69CE;
    c->pan = 0;
    c->bytes_per_blank =
        (vdp2_tvmd_tv_standard_get() == VDP2_TVMD_TV_STANDARD_PAL) ? 320 : 266;
    c->bitDepth = 0;
    c->loopType = 0;
    c->volume = 0;
    /* sh2_permit and icsr_target remain under the existing driver protocol. */
}
