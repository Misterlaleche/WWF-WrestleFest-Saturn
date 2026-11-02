/* Isolated R18 integration: replaces only the existing audio hook owner.
 * No game producer, input, scheduler, drawing or animation code is changed.
 * Cold boot leases the future tile cache; recovery requires a cold restart. */
#include <yaul.h>
#include <string.h>
#include "wf_audio2.h"
#include "wf_music_elf.h"
#include "wf_native_yaul.h"
#include "wf_audio2_game_queue.h"
#include "wf_audio2_cadence.h"
#include "wf_r18_scsp.h"
#include "wf_r18_cdfs.h"
#include "wf_saturn_memory.h"

extern uint8_t __audio2_hot_start[],__audio2_hot_end[];
extern uint8_t __audio2_slave_stack_bottom[],__audio2_slave_stack_top[];
volatile uint32_t wf_audio2_enabled=1;
volatile uint32_t wf_audio2_integration_diag[24];
volatile wf_audio2_cadence_t wf_audio2_cadence;
static wf_runtime_t *volatile cadence_runtime;
typedef struct {uint32_t frame;uint16_t word,reserved;} trace_event_t;
volatile trace_event_t wf_audio2_game_trace[512];
wf_audio2_game_queue_t wf_audio2_game_queue;
static wf_audio2_t audio;
static volatile uint32_t *clock_frames;
static volatile unsigned smpc_owner;
static volatile uint16_t worker_exited __attribute__((section(".uncached.audio2_worker")))=1;
static volatile uint32_t worker_run __attribute__((section(".uncached.audio2_worker")));
static uint32_t module_prepare;
static bool booting;
static uint16_t last_frt;
static uint32_t boot_us,fraction;
volatile uint32_t wf_master_cost_profile[16];


static uint16_t stable_frt(void)
{
    volatile cpu_ioregs_t *r=(volatile cpu_ioregs_t *)CPU_IOREG_BASE;
    uint16_t high,low,check;
    do{high=r->frch;low=r->frcl;check=r->frch;}while(high!=check);
    return (uint16_t)((high<<8)|low);
}
static uint32_t now(void *u)
{
    (void)u;
    if(!booting)return *clock_frames*16667u;
    uint16_t sample=stable_frt();
    uint32_t n=(uint16_t)(sample-last_frt)*1000u+fraction;
    last_frt=sample;boot_us+=n/CPU_FRT_NTSC_320_128_COUNT_1MS;
    fraction=n%CPU_FRT_NTSC_320_128_COUNT_1MS;return boot_us;
}
static void idle(void *u){(void)u;__asm__ volatile("nop");}
static void delay(uint32_t us,void *u)
{uint32_t start=now(u);while((uint32_t)(now(u)-start)<us)idle(u);}
static bool take(void *u){(void)u;if(smpc_owner)return false;smpc_owner=1;return true;}
static void release(void *u){(void)u;smpc_owner=0;}
bool wf_r18_scsp_smpc_owned(void){return smpc_owner!=0;}
static void write_module(uint32_t address,uint16_t value,void *u)
{(void)u;*(volatile uint16_t *)(uintptr_t)address=value;}
static void worker_entry(void)
{
    ((void (*)(void))(uintptr_t)(worker_run|0x20000000u))();
    worker_exited=1;
}
static bool quiescent(void *u){(void)u;return worker_exited!=0;}
static bool start_worker(uint32_t workspace,void *u)
{
    (void)u;
    if(!worker_exited||workspace!=(uint32_t)(uintptr_t)__audio2_hot_start)return false;
    volatile wf_music_module_shared_t *s=(void *)WF_MUSIC_MODULE_SHARED;
    cpu_cache_area_purge(__audio2_hot_start,18432);
    cpu_cache_area_purge((void *)WF_MUSIC_MODULE_BASE,WF_MUSIC_MODULE_END-WF_MUSIC_MODULE_BASE);
    s->workspace=workspace;
    ((void (*)(void))(uintptr_t)module_prepare)();
    if(s->state!=WF_MUSIC_BOOTING)return false;
    cpu_cache_area_purge(__audio2_hot_start,18432);
    cpu_cache_area_purge((void *)WF_MUSIC_MODULE_BASE,WF_MUSIC_MODULE_END-WF_MUSIC_MODULE_BASE);
    cpu_dual_slave_clear();cpu_dual_comm_mode_set(CPU_DUAL_ENTRY_POLLING);
    worker_exited=0;
    cpu_dual_slave_set((cpu_dual_slave_entry_t)((uintptr_t)worker_entry|0x20000000u));
    cpu_dual_slave_notify();return true;
}
static void diagnostics(void)
{
    wf_audio2_integration_diag[0]=wf_audio2_enabled;
    wf_audio2_integration_diag[2]=(uint32_t)audio.status.state;
    wf_audio2_integration_diag[3]=(uint32_t)audio.status.error;
    wf_audio2_integration_diag[4]=audio.status.worker_state;
    wf_audio2_integration_diag[5]=audio.status.worker_error;
    wf_audio2_integration_diag[6]=audio.status.accepted_commands;
    wf_audio2_integration_diag[7]=wf_audio2_game_queue.used;
    wf_audio2_integration_diag[8]=wf_audio2_game_queue.completed;
    wf_audio2_integration_diag[9]=wf_audio2_game_queue.overflows;
    wf_audio2_integration_diag[10]=wf_audio2_game_queue.unsupported;
    wf_audio2_integration_diag[15]=audio.status.max_mail_latency_us;
    if(wf_audio2_game_queue.used>wf_audio2_integration_diag[18])
        wf_audio2_integration_diag[18]=wf_audio2_game_queue.used;
}
bool wf_r18_scsp_boot(volatile uint32_t *frames,const void *elf,size_t bytes)
{
    clock_frames=frames;wf_audio2_integration_diag[0]=wf_audio2_enabled;
    wf_audio2_integration_diag[1]=1;
    if(!wf_audio2_enabled){wf_audio2_integration_diag[1]=9;return true;}
    const uint32_t hot=(uint32_t)(uintptr_t)__audio2_hot_start;
    if(__audio2_hot_end-__audio2_hot_start!=18432||
       __audio2_slave_stack_top-__audio2_slave_stack_bottom!=8192)return false;
    void *scratch=(void *)WF_SATURN_LWRAM_TILE_CACHE;
    uint32_t module_bytes,pack_bytes;wf_music_image_t module;
    if(wf_r18_file_size("MUSIC4.ELF",&module_bytes)||module_bytes>WF_SATURN_LWRAM_TILE_BYTES||
       wf_r18_file_read("MUSIC4.ELF",0,scratch,module_bytes)||
       !wf_music_elf_image(scratch,module_bytes,hot,18432,&module))return false;
    /* The validated image owns only the established audio VRAM and explicit
     * new linker reserve. All four loads validate before their first write. */
    if(!wf_music_elf_copy(&module,hot,18432,write_module,NULL))return false;
    module_prepare=module.prepare;worker_run=module.run;
    wf_audio2_integration_diag[22]=module_bytes;
    wf_audio2_integration_diag[1]=2;
    if(wf_r18_file_size("SFX.DAT",&pack_bytes)||pack_bytes>WF_SATURN_LWRAM_TILE_BYTES||
       wf_r18_file_read("SFX.DAT",0,scratch,pack_bytes))return false;
    wf_audio2_integration_diag[21]=pack_bytes;
    wf_audio2_integration_diag[1]=3;
    memset(__audio2_slave_stack_bottom,0xa5,8192);
    wf_native_platform_t platform={0};
    platform.now_us=now;platform.wait_us=delay;platform.idle=idle;
    platform.enter_owner=take;platform.leave_owner=release;
    wf_native_yaul_complete_platform(&platform);
    wf_audio2_config_t config={0};
    config.firmware_elf=elf;config.firmware_bytes=bytes;
    config.sfx_pack=scratch;config.sfx_bytes=pack_bytes;config.workspace=hot;
    config.mail=(void *)0x25a0c040u;config.music=(void *)WF_MUSIC_MODULE_SHARED;
    config.platform=platform;config.worker_start=start_worker;config.worker_quiescent=quiescent;
    booting=true;boot_us=fraction=0;cpu_frt_init(CPU_FRT_CLOCK_DIV_128);last_frt=stable_frt();
    if(!wf_audio2_init(&audio,&config)){diagnostics();return false;}
    wf_audio2_integration_diag[1]=4;
    uint32_t begun=now(NULL);
    do{
        wf_audio2_service(&audio);diagnostics();
        if(audio.status.state==WF_AUDIO2_RUNNING)break;
        if(audio.status.state==WF_AUDIO2_FAULT||now(NULL)-begun>=5000000u)return false;
    }while(1);
    booting=false;
    wf_audio2_integration_diag[1]=5;wf_audio2_integration_diag[20]=1;
    return true;
}
void wf_r18_scsp_vblank(void)
{
    if(!clock_frames)return;
    wf_runtime_t *rt=cadence_runtime;
    if(rt)wf_audio2_cadence_tick(&wf_audio2_cadence,
        *(volatile const uint32_t *)&rt->g.frame_counter,*clock_frames);
    wf_audio2_integration_diag[23]=*clock_frames;
    if(wf_audio2_enabled)wf_audio2_vblank(&audio,(uint16_t)*clock_frames);
}
bool wf_r18_scsp_service(void)
{
    if(!wf_audio2_enabled)return true;
    uint16_t profile_calibration=stable_frt();
    uint16_t profile_start=stable_frt();
    unsigned profile_pending=wf_audio2_game_queue.used;
    if(!wf_audio2_game_queue_service(&wf_audio2_game_queue,&audio)&&
       !wf_audio2_integration_diag[14])wf_audio2_integration_diag[14]=*clock_frames;
    diagnostics();
    uint32_t profile_elapsed=(uint16_t)(stable_frt()-profile_start);
    if(cadence_runtime){
        ++wf_master_cost_profile[0];wf_master_cost_profile[1]+=profile_elapsed;
        wf_master_cost_profile[2]+=(uint16_t)(profile_start-profile_calibration);
        if(profile_elapsed>wf_master_cost_profile[3])wf_master_cost_profile[3]=profile_elapsed;
        if(profile_pending){++wf_master_cost_profile[4];wf_master_cost_profile[5]+=profile_elapsed;}
        wf_master_cost_profile[6]=*clock_frames;
        wf_master_cost_profile[7]=cadence_runtime->g.frame_counter;
        wf_master_cost_profile[8]=CPU_FRT_NTSC_320_128_COUNT_1MS;
    }
    /* A latched audio failure remains measurable and cannot freeze combat.
     * Any fault/cancelled command prevents acceptance of this candidate. */
    return true;
}
wf_status_t wf_r18_scsp_audio(wf_runtime_t *rt,uint16_t raw,void *user)
{
    (void)user;
    if(rt&&!cadence_runtime)cadence_runtime=rt;
    const uint32_t frame=rt?rt->g.frame_counter:UINT32_MAX;
    uint32_t count=wf_audio2_integration_diag[12]++;
    if(count<512){wf_audio2_game_trace[count].frame=frame;
        wf_audio2_game_trace[count].word=raw;}
    wf_audio2_integration_diag[13]=(wf_audio2_integration_diag[13]^raw^frame)*16777619u;
    wf_audio2_integration_diag[19]=raw;
    if(wf_audio2_enabled&&!wf_audio2_game_queue_push(&wf_audio2_game_queue,raw))
        wf_audio2_integration_diag[11]++;
    return WF_OK;
}
