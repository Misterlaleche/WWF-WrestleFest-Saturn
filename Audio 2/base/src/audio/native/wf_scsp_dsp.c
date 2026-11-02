#include "wf_scsp_dsp.h"
#include "wf_dsp_program.h"
static wf_sfx_write16_fn writer;static void *context;
static bool put(uint32_t a,uint16_t v){return writer&&writer(a,v,context);}
bool wf_scsp_dsp_load(wf_sfx_write16_fn fn,void *user)
{
 writer=fn;context=user;
 if(!put(0x100402u,0)||!put(0xb012,0xe700)||!put(0xb014,0xe519)||!put(0xb016,0x8000))return false;
 for(unsigned i=0;i<64;i++)if(!put(0x100700u+i*2u,0))return false;
 if(!put(0x100700,0x8000)||!put(0x100714,21627))return false;
 for(unsigned i=0;i<32;i++)if(!put(0x100780u+i*2u,0))return false;
 if(!put(0x100780,0xb012/2)||!put(0x100782,0xb014/2)||!put(0x100784,WF_DSP_CLOCK/2))return false;
 /* MADRS points at the ring midpoint because ADRS is a signed12 offset.
  * The assembler biases only the address latch; phase and4096+sentinel stay. */
 for(unsigned v=0;v<4;v++)if(!put(0x100786u+v*2u,(uint16_t)((WF_SFX_PCM_BASE+v*8194u)/2u+2048u)))return false;
 if(!put(0x10078e,0xb016/2))return false;
 /* The DSP runs between68000 stores. Keep MWT disabled until the complete
  * address/format words are installed; publishing word2 first otherwise
  * writes the clock into MADRS0 and changes its constant from e700 to6100. */
 for(unsigned i=0;i<128;i++)for(unsigned w=0;w<4;w++){
  uint16_t value=wf_dsp_program[i][w];
  if(w==2)value&=(uint16_t)~0x4000u;
  if(!put(0x100800u+i*8u+w*2u,value))return false;
 }
 for(unsigned i=0;i<128;i++)if(wf_dsp_program[i][2]&0x4000u)
  if(!put(0x100804u+i*8u,wf_dsp_program[i][2]))return false;
 if(!put(0x100bf0,0)||!put(WF_DSP_CLOCK,0))return false;
 return true;
}
void wf_scsp_dsp_clock_run(void){(void)put(0x100712,0x8000);}
bool wf_scsp_dsp_voice(unsigned voice,bool start,unsigned volume,void *user)
{
 static const unsigned amplitudes[16]={32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0};
 (void)user;if(voice>=4||volume>=16)return false;
 if(!start)return put(0x10070au+voice*2u,0)&&put(0x100702u+voice*2u,0);
 int gain=-(int)((21627u*amplitudes[volume]+16u)/32u);
 return put(0x100702u+voice*2u,0x8000)&&put(0x10070au+voice*2u,(uint16_t)gain);
}
