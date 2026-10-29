#include "ponesound_min.h"
#include <smpc/smc.h>
#include <string.h>
#define PCM_SCSP_FREQUENCY 44100L
static wf_sys_com_t * const m68k_com=(wf_sys_com_t *)(SCSP_RAM_ADDR(0x408 + DRV_SYS_END));
static const int logtbl[256]={
0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
static short pitchword(short rate){
 int div=(int)(PCM_SCSP_FREQUENCY/(rate+1)); if(div<0)div=0; if(div>255)div=255;
 int oct=logtbl[div],shift=PCM_SCSP_FREQUENCY>>oct,fns=((rate-shift)<<10)/shift;
 return (short)((((-oct)&0xF)<<11)|(fns&0x3FF));
}
static short bytes_per_blank(int rate){
 return (short)(rate/((vdp2_tvmd_tv_standard_get()==VDP2_TVMD_TV_STANDARD_PAL)?50:60));
}
static void master_volume(unsigned short v){
 volatile unsigned short *mv=(volatile unsigned short *)SCSP_RAM_ADDR(0x100400);
 if(v>15)v=15; *mv=(unsigned short)(0x200|(v&15));
}
void wf_ponesound_load_driver(const uint8_t *driver,int driver_size){
 smpc_smc_call(SMPC_SMC_SNDOFF); smpc_smc_call(SMPC_SMC_SNDON);
 *(volatile unsigned char *)(0x25B00400UL)=0x02;
 memset((void *)SCSP_RAM_ADDR(0),0,0x80000);
 smpc_smc_call(SMPC_SMC_SNDOFF); smpc_smc_wait(0);
 master_volume(15); memcpy((void *)SCSP_RAM_ADDR(0),driver,(size_t)driver_size);
 m68k_com->drv_adx_coef_1=6631; m68k_com->drv_adx_coef_2=-2685;
 smpc_smc_wait(0); smpc_smc_call(SMPC_SMC_SNDON); m68k_com->start=0xFFFF;
}
void wf_ponesound_copy_to_scsp(const void *pcm,int pcm_size,int off){
 memcpy((void *)SCSP_RAM_ADDR((unsigned)off),pcm,(size_t)pcm_size);
}
void wf_ponesound_configure_8bit_pcm(int pcm_size,int n,int off,int rate){
 m68k_com->pcmCtrl[n].hiAddrBits=(unsigned short)((unsigned)off>>16);
 m68k_com->pcmCtrl[n].loAddrBits=(unsigned short)((unsigned)off&0xFFFF);
 m68k_com->pcmCtrl[n].pitchword=(unsigned short)pitchword((short)rate);
 m68k_com->pcmCtrl[n].playsize=(unsigned short)pcm_size;
 m68k_com->pcmCtrl[n].bytes_per_blank=(unsigned short)bytes_per_blank(rate);
 m68k_com->pcmCtrl[n].bitDepth=PCM_TYPE_8BIT; m68k_com->pcmCtrl[n].loopType=0; m68k_com->pcmCtrl[n].volume=7;
}
void wf_ponesound_load_8bit_pcm(const void *pcm,int pcm_size,int n,int off,int rate){
 wf_ponesound_copy_to_scsp(pcm,pcm_size,off);
 wf_ponesound_configure_8bit_pcm(pcm_size,n,off,rate);
}
void wf_ponesound_pcm_play(short n,char type,char volume){
 if(n<0)return; m68k_com->pcmCtrl[n].sh2_permit=1; m68k_com->pcmCtrl[n].volume=(unsigned char)volume; m68k_com->pcmCtrl[n].loopType=type;
}
void wf_ponesound_pcm_cease(short n){
 if(n<0)return;
 if(m68k_com->pcmCtrl[n].loopType<=0)m68k_com->pcmCtrl[n].volume=0;
 else m68k_com->pcmCtrl[n].sh2_permit=0;
}
void wf_ponesound_vblank(void *work){(void)work;m68k_com->start=1;}
