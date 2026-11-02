/* Isolated Audio 2 diagnostic. Does not replace any gameplay or scene runtime. */
#include <yaul.h>
#include <stdint.h>
#include <stdbool.h>
#include "wf_native_host.h"
#include "wf_native_elf.h"
#include "wf_native_yaul.h"
#include "wf_music_module.h"
#include "wf_scsp_sfx.h"
#include "wf_audio2.h"
#include "wf_audio2_yaul.h"

extern const uint8_t audio2_firmware[],audio2_firmware_end[],audio2_sfx[],audio2_sfx_end[];
extern uint8_t wf_hotcode_start[],wf_hotcode_end[];
extern void wf_music_module_prepare(void),wf_music_module_run(void);
#ifndef AUDIO2_COMMAND
#define AUDIO2_COMMAND 1
#endif
#ifndef AUDIO2_SECONDS
#define AUDIO2_SECONDS 35
#endif
#ifdef AUDIO2_REPLAY
#include "audio2_replay.h"
volatile uint32_t audio2_replay_observed[AUDIO2_REPLAY_COUNT][4]
    __attribute__((section(".uncached.audio2_replay")));
#endif
static wf_audio2_t audio;
static volatile wf_music_module_shared_t *const music=(void *)WF_MUSIC_MODULE_SHARED;
static volatile uint32_t video_frames;
/* A fixed-address uncached diagnostic mirror is read by the external capture tool.
 * It is observation only: it is never the clock used by the audio engine. */
volatile uint32_t audio2_diag[32] __attribute__((section(".uncached.audio2_diag")));
extern volatile uint32_t wf_music_fm_discarded_events,wf_music_fm_discarded_batches;
extern volatile uint32_t wf_music_fm_rejected_events,wf_music_ring_rejected_events;
#ifndef AUDIO2_SFX_ENGINE_ADDR
#error Supply the exact linked 68000 wf_sfx_engine address
#endif
_Static_assert(offsetof(wf_scsp_sfx_t,underruns)==32,"diagnostic SFX field layout");
static uint32_t read32(const volatile uint32_t *p)
{return *(volatile uint32_t *)((uintptr_t)p|0x20000000u);}
static uint16_t frt_last;
static uint32_t boot_us,frt_fraction;
static bool owner;
static uint32_t now_us(void *unused)
{
    (void)unused;uint16_t n=cpu_frt_count_get();
    uint32_t amount=(uint16_t)(n-frt_last)*1000u+frt_fraction;frt_last=n;
    boot_us+=amount/CPU_FRT_NTSC_320_128_COUNT_1MS;
    frt_fraction=amount%CPU_FRT_NTSC_320_128_COUNT_1MS;
    return boot_us;
}
static void idle(void *unused){(void)unused;(void)now_us(NULL);__asm__ volatile("nop");}
static void delay(uint32_t us,void *unused){uint32_t begin=now_us(unused);while(now_us(unused)-begin<us)idle(unused);}
static bool enter(void *unused){(void)unused;if(owner)return false;owner=true;return true;}
static void leave(void *unused){(void)unused;owner=false;}
static void vblank(void *unused){(void)unused;++video_frames;wf_audio2_vblank(&audio,(uint16_t)video_frames);}
static bool boot_audio(void)
{
    wf_native_platform_t ops={0};wf_audio2_config_t config={0};
    ops.now_us=now_us;ops.wait_us=delay;ops.idle=idle;ops.enter_owner=enter;ops.leave_owner=leave;
    wf_native_yaul_complete_platform(&ops);
    config.firmware_elf=audio2_firmware;config.firmware_bytes=(size_t)(audio2_firmware_end-audio2_firmware);
    config.sfx_pack=audio2_sfx;config.sfx_bytes=(size_t)(audio2_sfx_end-audio2_sfx);
    config.workspace=(uint32_t)(uintptr_t)wf_hotcode_start;
    config.mail=(void *)0x25a0c040;config.music=music;config.platform=ops;
    config.worker_start=wf_audio2_yaul_worker_start;
    config.worker_quiescent=wf_audio2_yaul_worker_quiescent;
    if(!wf_audio2_init(&audio,&config))return false;
    uint32_t begin=now_us(NULL);
    while(audio.status.state==WF_AUDIO2_STARTING&&now_us(NULL)-begin<5000000u){wf_audio2_service(&audio);idle(NULL);}
    return audio.status.state==WF_AUDIO2_RUNNING;
}
void user_init(void)
{
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_240);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(0,0x1fffe),RGB1555(1,1,2,4));
    dbgio_init();dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);dbgio_dev_font_load();
    vdp_sync_vblank_in_set(vblank,NULL);vdp2_tvmd_display_set();
    cpu_frt_init(CPU_FRT_CLOCK_DIV_128);frt_last=cpu_frt_count_get();
}
int main(void)
{
    audio2_diag[0]=0x57464132;audio2_diag[1]=1;
    bool ok=boot_audio();audio2_diag[1]=ok?2:3;audio2_diag[3]=music->error;
    uint32_t start=video_frames,scsp_elapsed=0;uint16_t scsp_last=*(volatile uint16_t *)0x25a0b010;
    bool sent=false,finished=false;
#ifdef AUDIO2_REPLAY
    unsigned replay_index=0;
#endif
    for(;;){
        uint32_t frame=video_frames;
        wf_audio2_service(&audio);
        uint16_t scsp_now=*(volatile uint16_t *)0x25a0b010;
        if(sent)scsp_elapsed+=(uint16_t)(scsp_now-scsp_last);
        scsp_last=scsp_now;
        if(ok&&!sent&&frame-start>=60){
            wf_audio2_result_t result=wf_audio2_command(&audio,
#ifdef AUDIO2_REPLAY
                audio2_replay[0].word
#else
                AUDIO2_COMMAND
#endif
            );
            if(result==WF_AUDIO2_ACCEPTED){sent=true;start=frame;scsp_elapsed=0;audio2_diag[4]=frame;audio2_diag[21]=scsp_now;}
            else if(result!=WF_AUDIO2_BUSY){ok=false;audio2_diag[1]=3;}
#ifdef AUDIO2_REPLAY
            if(sent){audio2_replay_observed[0][0]=audio2_replay[0].word;
                audio2_replay_observed[0][1]=scsp_elapsed;audio2_replay_observed[0][2]=frame;
                audio2_replay_observed[0][3]=result;replay_index=1;}
#endif
        }
#ifdef AUDIO2_REPLAY
        while(ok&&sent&&replay_index<AUDIO2_REPLAY_COUNT&&
              scsp_elapsed>=audio2_replay[replay_index].sample){
            wf_audio2_result_t result=wf_audio2_command(&audio,audio2_replay[replay_index].word);
            if(result==WF_AUDIO2_BUSY)break;
            if(result!=WF_AUDIO2_ACCEPTED){ok=false;audio2_diag[1]=3;break;}
            audio2_replay_observed[replay_index][0]=audio2_replay[replay_index].word;
            audio2_replay_observed[replay_index][1]=scsp_elapsed;
            audio2_replay_observed[replay_index][2]=frame;
            audio2_replay_observed[replay_index][3]=result;
            ++replay_index;
        }
        audio2_diag[22]=replay_index;audio2_diag[23]=AUDIO2_REPLAY_COUNT;
#endif
        if(sent&&!finished&&scsp_elapsed>=AUDIO2_SECONDS*44100u){
            finished=true;audio2_diag[1]=4;
        }
        audio2_diag[2]=frame;audio2_diag[3]=music->error;
        audio2_diag[5]=music->state;audio2_diag[6]=music->events;
        audio2_diag[7]=*(volatile uint16_t *)0x25a0b03a;
        audio2_diag[8]=*(volatile uint16_t *)0x25a0b028;
        audio2_diag[9]=*(volatile uint32_t *)(uintptr_t)(0x25a00000u+AUDIO2_SFX_ENGINE_ADDR+32u);
        audio2_diag[10]=music->last_command;
        audio2_diag[11]=*(volatile uint16_t *)0x25a0b010;
        audio2_diag[12]=(uint32_t)(uintptr_t)&audio2_diag[0];
        audio2_diag[13]=*(volatile uint16_t *)0x25a0c046;
        audio2_diag[14]=read32(&wf_music_fm_discarded_events);
        audio2_diag[15]=read32(&wf_music_fm_discarded_batches);
        audio2_diag[16]=read32(&wf_music_fm_rejected_events);
        audio2_diag[17]=read32(&wf_music_ring_rejected_events);
        audio2_diag[18]=scsp_elapsed;audio2_diag[19]=audio.status.state;
        audio2_diag[20]=audio.status.error;
        if((frame%30)==0){
            dbgio_printf("\x1b[H\x1b[2JAUDIO 2 DIAGNOSTIC\nCOMMAND %02X  FRAME %lu\nBOOT %s  STATE %u\nWORKER ERROR %lu\nFM EVENTS LOST %lu  BATCHES %lu\nLATE %lu  PCM UNDERRUNS %lu\nAPI STATE %lu  ERROR %lu\n\nHARDWARE PENDING\nNATIVE FIDELITY UNDER AUDIT\n%s\n",AUDIO2_COMMAND,(unsigned long)frame,ok?"OK":"FAIL",music->state,(unsigned long)music->error,(unsigned long)audio2_diag[14],(unsigned long)audio2_diag[15],(unsigned long)audio2_diag[8],(unsigned long)audio2_diag[9],(unsigned long)audio2_diag[19],(unsigned long)audio2_diag[20],finished?"CAPTURE WINDOW COMPLETE":"");
            dbgio_flush();
        }
        vdp2_sync();vdp2_sync_wait();
    }
}
