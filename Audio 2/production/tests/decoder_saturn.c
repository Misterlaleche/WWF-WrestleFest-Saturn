/* Isolated hardware decoder observer. No game, slave worker or audio scheduler. */
#include <yaul.h>
#include "wf_native_host.h"
#include "wf_native_elf.h"
#include "wf_native_yaul.h"
extern const uint8_t decoder_firmware[],decoder_firmware_end[],decoder_pack[],decoder_pack_end[];
volatile uint32_t decoder_host_diag[32] __attribute__((section(".uncached.decoder")));
volatile uint32_t decoder_host_boot __attribute__((section(".uncached.decoder")));
static uint16_t last_frt;static uint32_t micros,fraction;static bool owned;
static uint32_t now(void *unused)
{
 (void)unused;volatile cpu_ioregs_t *r=(volatile cpu_ioregs_t *)CPU_IOREG_BASE;
 uint16_t high,low,check;do{high=r->frch;low=r->frcl;check=r->frch;}while(high!=check);
 uint16_t n=(uint16_t)((high<<8)|low);uint32_t amount=(uint16_t)(n-last_frt)*1000u+fraction;
 last_frt=n;micros+=amount/CPU_FRT_NTSC_320_128_COUNT_1MS;fraction=amount%CPU_FRT_NTSC_320_128_COUNT_1MS;return micros;
}
static void idle(void *u){(void)now(u);__asm__ volatile("nop");}
static void delay(uint32_t n,void *u){uint32_t start=now(u);while(now(u)-start<n)idle(u);}
static bool enter(void *u){(void)u;if(owned)return false;owned=true;return true;}
static void leave(void *u){(void)u;owned=false;}
void user_init(void)
{
 vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);
 vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(0,0x1fffe),RGB1555(1,1,2,4));
 dbgio_init();dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);dbgio_dev_font_load();vdp2_tvmd_display_set();
 cpu_frt_init(CPU_FRT_CLOCK_DIV_128);(void)now(0);
}
int main(void)
{
 wf_native_platform_t p={0};wf_native_host_t host;wf_native_segment_t segment[3];wf_native_image_t image;
 p.now_us=now;p.wait_us=delay;p.idle=idle;p.enter_owner=enter;p.leave_owner=leave;wf_native_yaul_complete_platform(&p);
 if(!wf_native_elf_image(decoder_firmware,(size_t)(decoder_firmware_end-decoder_firmware),segment,&image)||
    !wf_native_host_init(&host,(void *)0x25a0c040,&p)||!wf_native_host_boot(&host,&image)){
  decoder_host_boot=2;for(;;){}
 }
 for(unsigned i=0;i<(unsigned)(decoder_pack_end-decoder_pack);i+=2)
  *(volatile uint16_t *)(0x25a14000u+i)=(uint16_t)((decoder_pack[i]<<8)|decoder_pack[i+1]);
 volatile uint32_t *diag=(volatile uint32_t *)(0x25a00000u+DECODER_DIAG_ADDRESS);
 diag[20]=1;decoder_host_boot=1;
 for(;;){
  for(unsigned i=0;i<32;i++)decoder_host_diag[i]=diag[i];
  dbgio_printf("\x1b[HDECODER 68000 EXACT TEST\nSTATE %lu ERROR %lu\nSAMPLES %lu/1034226\nENTRIES %lu/114\nBASE %lu CANDIDATE %lu FS\nBLOCKS %lu\nGAIN GATE %lu\n",(unsigned long)diag[1],(unsigned long)diag[2],(unsigned long)diag[4],(unsigned long)diag[5],(unsigned long)diag[6],(unsigned long)diag[7],(unsigned long)diag[8],(unsigned long)diag[19]);
  dbgio_flush();vdp2_sync();vdp2_sync_wait();
 }
}
