#include "wf_r18_scsp.h"
#include "wf_r18_scsp_runtime.h"
#include "../../audio/native/wf_native_yaul.h"
#include "../../audio/native/wf_native_elf.h"
#include "../../audio/native/wf_sfx_upload.h"
#include "wf_r18_cdfs.h"
#include "../../audio/native/wf_music_elf.h"
#include "wf_saturn_memory.h"
#include <yaul.h>
#ifdef WF_R18_DIAG_RATE
extern void wf_saturn_allocator_stats_capture(void);
extern volatile uint32_t wf_saturn_allocator_live_stats[16];
#endif

static wf_native_host_t host;
static volatile uint32_t *clock_frames;
static volatile uint32_t scsp_timing[8];
static volatile uint32_t scsp_io[2];
static volatile unsigned owner;
static uint32_t now(void *u);
static bool booting,music_native;
static volatile wf_music_module_shared_t *const music=(volatile wf_music_module_shared_t *)(uintptr_t)WF_MUSIC_MODULE_SHARED;
static void music_write(uint32_t address,uint16_t value,void *u)
{(void)u;*(volatile uint16_t *)(uintptr_t)address=value;}
extern unsigned char __wf_music_hot_start[],__wf_music_hot_end[];
static bool music_boot(void)
{
 uint32_t bytes;wf_music_image_t image;
 uint32_t workspace=(uint32_t)(uintptr_t)__wf_music_hot_start;
 uint32_t workspace_bytes=(uint32_t)(uintptr_t)__wf_music_hot_end-workspace;
 void *scratch=(void *)WF_SATURN_LWRAM_SCENE;
 if(wf_r18_file_size("MUSIC.ELF",&bytes)||bytes>WF_SATURN_LWRAM_SCENE_BYTES||
    wf_r18_file_read("MUSIC.ELF",0,scratch,bytes)||!wf_music_elf_image(scratch,bytes,&image)||
    !wf_music_elf_copy(&image,music_write,NULL))return false;
 scsp_timing[5]=*clock_frames;
 cpu_cache_area_purge((void *)WF_MUSIC_MODULE_BASE,WF_MUSIC_MODULE_END-WF_MUSIC_MODULE_BASE);
 music->workspace=workspace;
 cpu_cache_area_purge((void *)(uintptr_t)workspace,workspace_bytes);
 ((void (*)(void))(uintptr_t)image.prepare)();
 cpu_cache_area_purge((void *)(uintptr_t)workspace,workspace_bytes);
 cpu_cache_area_purge((void *)WF_MUSIC_MODULE_BASE,WF_MUSIC_MODULE_END-WF_MUSIC_MODULE_BASE);
 if(music->state!=WF_MUSIC_BOOTING||music->workspace!=workspace)return false;
 /* The module starts in an uncached ASM trampoline, purges slave CCR there,
  * then jumps to the cached C body without changing the caller's PR. */
 cpu_dual_slave_set((cpu_dual_slave_entry_t)(uintptr_t)(image.run|0x20000000u));cpu_dual_slave_notify();
 scsp_timing[6]=*clock_frames;
 uint32_t started=now(NULL);
 while(music->state==WF_MUSIC_BOOTING){
  music->master_frame=(uint16_t)*clock_frames;
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
static uint32_t now(void *u)
{
    (void)u;
    if(!booting)return *clock_frames*16667u;
    /* During boot poll FRT frequently, including while video is blank.
     * No FRT IRQ or long-lived timer takeover. The video backend may
     * subsequently initialize FRT as before; runtime deadlines use VBlank. */
    uint16_t sample=cpu_frt_count_get();
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
    if(!frames)return false;
    wf_r18_scsp_runtime_init(&host,music,frames,false);
    if(!wf_native_elf_image(elf,bytes,segments,&image)){wf_r18_scsp_disable(WF_R18_AUDIO_BOOT);return false;}
    clock_frames=frames;owner=0;music_native=false;booting=true;boot_us=fraction=0;
    scsp_timing[0]=*frames;
    scsp_io[0]=scsp_io[1]=0;
    cpu_frt_init(CPU_FRT_CLOCK_DIV_128);last_frt=cpu_frt_count_get();
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
    if(ok&&host.mail->reserved[2]==WF_NATIVE_MUSIC_PROFILE){
        music_native=true;wf_r18_scsp_runtime_init(&host,music,frames,true);
        scsp_timing[4]=*frames;ok=music_boot();
    }
    if(ok)ok=wf_native_queue_init(&wf_r18_scsp_queue,&host);
    booting=false;
#ifdef WF_R18_DIAG_RATE
    wf_saturn_allocator_stats_capture();
    if(ok&&music_native&&(wf_saturn_allocator_live_stats[1]||
        wf_saturn_allocator_live_stats[3]<4096u||wf_saturn_allocator_live_stats[4]<4096u))ok=false;
#endif
    if(!ok)wf_r18_scsp_disable(WF_R18_AUDIO_BOOT);
    return ok;
}
