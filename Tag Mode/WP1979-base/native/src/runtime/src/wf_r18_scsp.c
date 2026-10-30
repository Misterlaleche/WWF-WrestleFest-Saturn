#include "wf_r18_scsp.h"
#include "../../audio/native/wf_native_yaul.h"
#include "../../audio/native/wf_native_elf.h"
#include "../../audio/native/wf_sfx_upload.h"
#include "wf_r18_cdfs.h"
#include "../../audio/native/wf_music_elf.h"
#include "wf_saturn_memory.h"
#include <yaul.h>

static wf_native_host_t host;
wf_native_queue_t wf_r18_scsp_queue;
static volatile uint32_t *clock_frames;
static volatile uint32_t scsp_timing[8];
static volatile uint32_t scsp_io[2];
static volatile unsigned owner;
static uint32_t now(void *u);
static bool booting,music_native;
static volatile wf_music_module_shared_t *const music=(volatile wf_music_module_shared_t *)(uintptr_t)WF_MUSIC_MODULE_SHARED;
static void music_write(uint32_t address,uint16_t value,void *u)
{(void)u;*(volatile uint16_t *)(uintptr_t)address=value;}
static bool music_boot(void)
{
 uint32_t bytes;wf_music_image_t image;
 void *scratch=(void *)WF_SATURN_LWRAM_SCENE;
 if(wf_r18_file_size("MUSIC.ELF",&bytes)||bytes>WF_SATURN_LWRAM_SCENE_BYTES||
    wf_r18_file_read("MUSIC.ELF",0,scratch,bytes)||!wf_music_elf_image(scratch,bytes,&image)||
    !wf_music_elf_copy(&image,music_write,NULL))return false;
 scsp_timing[5]=*clock_frames;
 cpu_cache_area_purge((void *)WF_MUSIC_MODULE_BASE,WF_MUSIC_MODULE_END-WF_MUSIC_MODULE_BASE);
 ((void (*)(void))(uintptr_t)image.prepare)();
 cpu_cache_area_purge((void *)WF_MUSIC_MODULE_BASE,WF_MUSIC_MODULE_END-WF_MUSIC_MODULE_BASE);
 cpu_dual_slave_set((cpu_dual_slave_entry_t)(uintptr_t)image.run);cpu_dual_slave_notify();
 scsp_timing[6]=*clock_frames;
 uint32_t started=now(NULL);
 while(music->state==WF_MUSIC_BOOTING){
  if((uint32_t)(now(NULL)-started)>5000000u){music->shutdown=1;return false;}
 }
 scsp_timing[7]=*clock_frames;
 return music->state==WF_MUSIC_RUNNING;
}
static uint16_t last_frt;
static uint32_t boot_us,fraction;
static bool take(void *u)
{
    (void)u;
    if(owner)return false;
    owner=1;
    return true;
}
static void release(void *u){(void)u;owner=0;}
bool wf_r18_scsp_smpc_owned(void){return owner!=0;}
/* FRT high/low are read separately by the SDK. Retry at a byte rollover
 * so a torn sample cannot become a spurious full16-bit elapsed interval. */
static uint16_t stable_frt_count(void)
{
    volatile cpu_ioregs_t *const regs=(volatile cpu_ioregs_t *)CPU_IOREG_BASE;
    uint16_t high,low,check;
    do{high=regs->frch;low=regs->frcl;check=regs->frch;}while(high!=check);
    return (uint16_t)((high<<8)|low);
}
static uint32_t now(void *u)
{
    (void)u;
    if(!booting)return *clock_frames*16667u;
    /* During boot poll FRT frequently, including while video is blank.
     * No FRT IRQ or long-lived timer takeover. The video backend may
     * subsequently initialize FRT as before; runtime deadlines use VBlank. */
    uint16_t sample=stable_frt_count();
    uint32_t numerator=(uint16_t)(sample-last_frt)*1000u+fraction;
    last_frt=sample;
    boot_us+=numerator/CPU_FRT_NTSC_320_128_COUNT_1MS;
    fraction=numerator%CPU_FRT_NTSC_320_128_COUNT_1MS;
    return boot_us;
}
static void idle(void *u){(void)u;__asm__ volatile("nop");}
static void delay(uint32_t us,void *u)
{
    uint32_t start=now(u);
    while((uint32_t)(now(u)-start)<us)idle(u);
}
static bool read_sfx(uint32_t at,void *dst,size_t bytes,void *u)
{
    (void)u;
    uint32_t started=*clock_frames;
    bool ok=wf_r18_file_read("SFX.DAT",at,dst,bytes)==0;
    scsp_io[0]+=*clock_frames-started;
    scsp_io[1]++;
    return ok;
}
bool wf_r18_scsp_boot(volatile uint32_t *frames,const void *elf,size_t bytes)
{
    wf_native_platform_t platform={0};
    wf_native_segment_t segments[3];
    wf_native_image_t image;
    if(!frames||!wf_native_elf_image(elf,bytes,segments,&image))return false;
    clock_frames=frames;owner=0;music_native=false;booting=true;boot_us=fraction=0;
    scsp_timing[0]=*frames;
    scsp_io[0]=scsp_io[1]=0;
    cpu_frt_init(CPU_FRT_CLOCK_DIV_128);last_frt=stable_frt_count();
    platform.now_us=now;platform.wait_us=delay;platform.idle=idle;
    platform.enter_owner=take;platform.leave_owner=release;
    wf_native_yaul_complete_platform(&platform);
    /* No earlier callback may run while either executable is replaced. */
    if(!take(NULL))return false;
    cpu_dual_slave_clear();cpu_dual_comm_mode_set(CPU_DUAL_ENTRY_POLLING);
    release(NULL);
    bool ok=wf_native_host_init(&host,(volatile wf_native_mail_t *)0x25a0c040u,&platform)&&
        wf_native_host_boot(&host,&image);
    scsp_timing[1]=*frames;
    if(ok&&(host.mail->reserved[2]==WF_NATIVE_SFX_PROFILE||host.mail->reserved[2]==WF_NATIVE_MUSIC_PROFILE)){
        uint32_t pack_bytes;
        scsp_timing[2]=*frames;
        ok=wf_r18_file_size("SFX.DAT",&pack_bytes)==0&&
            wf_sfx_upload_bulk(&host,pack_bytes,read_sfx,NULL,
                (uint8_t *)WF_SATURN_LWRAM_SCENE,32768u);
        scsp_timing[3]=*frames;
    }
    if(ok&&host.mail->reserved[2]==WF_NATIVE_MUSIC_PROFILE){music_native=true;scsp_timing[4]=*frames;ok=music_boot();}
    if(ok)ok=wf_native_queue_init(&wf_r18_scsp_queue,&host);
    booting=false;
    return ok;
}
void wf_r18_scsp_vblank(void){wf_native_host_pcm_tick(&host);}
bool wf_r18_scsp_service(void)
{
    if(music_native&&music->state!=WF_MUSIC_RUNNING)return false;
    return wf_native_queue_service(&wf_r18_scsp_queue,now(NULL));
}
wf_status_t wf_r18_scsp_audio(wf_runtime_t *rt,uint16_t raw,void *user)
{
    (void)rt;(void)user;
    unsigned command=raw&255u;
    if(music_native&&(!command||(command>=1&&command<=18)||command==0x1f)){
        uint16_t producer=music->producer;
        if(music->state!=WF_MUSIC_RUNNING||(uint16_t)(producer-music->consumer)>=64u)return WF_BAD_STATE;
        /* Each stream owns its stop: mailbox0 stops only effects0..2;
         * slave generation stops FM and percussion3, preserving later commands. */
        if(!command&&!wf_native_queue_push(&wf_r18_scsp_queue,raw))return WF_BAD_STATE;
        music->commands[producer&63u]=(uint16_t)command;
        (void)music->commands[producer&63u];music->producer=(uint16_t)(producer+1u);
        return WF_OK;
    }
    return wf_native_queue_push(&wf_r18_scsp_queue,raw)?WF_OK:WF_BAD_STATE;
}
