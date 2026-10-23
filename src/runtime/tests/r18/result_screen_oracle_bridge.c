#include <string.h>
#include "wf_r18_result_screen.h"

typedef struct result_probe {
    const uint8_t *rom;
    size_t rom_size;
    wf_r18_video_t *video;
    uint32_t *events;
    uint8_t *snapshots;
    unsigned count;
} result_probe_t;

static wf_status_t read_rom(void *user,uint32_t address,void *out,size_t length){
    result_probe_t *p=user;
    if(address>p->rom_size||length>p->rom_size-address)return WF_BLOCKED;
    memcpy(out,p->rom+address,length);
    return WF_OK;
}

static void observe(result_probe_t *p,wf_runtime_t *rt,unsigned kind,unsigned value){
    uint32_t *event=p->events+p->count*10u;
    event[0]=kind;event[1]=value;
    memcpy(event+2,rt->m68k_regs.d,sizeof rt->m68k_regs.d);
    memcpy(p->snapshots+p->count*sizeof p->video->fg0,p->video->fg0,sizeof p->video->fg0);
    p->count++;
}

static wf_status_t wait_frames(wf_runtime_t *rt,uint16_t frames,void *user){
    observe(user,rt,0u,frames);
    return WF_OK;
}

typedef struct native_probe {
    result_probe_t p;
    wf_runtime_t *runtime;
} native_probe_t;

static wf_status_t commit(void *user){
    native_probe_t *p=user;
    observe(&p->p,p->runtime,1u,(uint16_t)p->runtime->m68k_regs.d[0]);
    return WF_OK;
}

/* Only physical boundaries are observed. The real translated $2503C owns text. */
int wf_result_screen_oracle(const uint8_t *rom,size_t rom_size,uint8_t *actors,
                           uint32_t *regs,unsigned progression,uint8_t *fg0,
                           uint32_t *events,uint8_t *snapshots,unsigned *count){
    wf_runtime_t rt;
    wf_r18_video_t video;
    native_probe_t probe;
    wf_status_t status;
    memset(&rt,0,sizeof rt);memset(&video,0,sizeof video);memset(&probe,0,sizeof probe);
    memcpy(rt.actors,actors,sizeof rt.actors);memcpy(rt.m68k_regs.d,regs,sizeof rt.m68k_regs.d);
    rt.m68k_regs.valid=1;rt.g.progression_0162=(uint16_t)progression;
    memcpy(video.fg0,fg0,sizeof video.fg0);
    probe.p.rom=rom;probe.p.rom_size=rom_size;probe.p.video=&video;
    probe.p.events=events;probe.p.snapshots=snapshots;probe.runtime=&rt;
    video.read_rom=read_rom;video.read_user=&probe;
    status=wf_r18_result_in_ring(&rt,&video,wait_frames,commit,&probe);
    memcpy(actors,rt.actors,sizeof rt.actors);memcpy(regs,rt.m68k_regs.d,sizeof rt.m68k_regs.d);
    memcpy(fg0,video.fg0,sizeof video.fg0);*count=probe.p.count;
    return status;
}
