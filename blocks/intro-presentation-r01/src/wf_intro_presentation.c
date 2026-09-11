#include "wf_intro_presentation.h"
#include <string.h>
const wf_intro_asset_t wf_intro_assets[WF_INTRO_SCENE_COUNT]={
    {0U,13001U,13004U,0xFB40AE48U,0x964A7820U},
    {13516U,14653U,28172U,0x2041D3CEU,0x0A115ABBU},
    {28684U,5208U,33892U,0x50F5EF03U,0x41E58924U}
};
static wf_intro_event_t ev(wf_intro_event_type_t t,wf_intro_scene_t s,unsigned f){
    wf_intro_event_t e={.type=t,.scene=s,.fade_level=(uint8_t)f}; return e;
}
wf_intro_event_t wf_intro_init(wf_intro_state_t *s,bool fbi){
    if(s==NULL) return ev(WF_INTRO_EVENT_FAULT,WF_INTRO_SCENE_TITLE_HANDOFF,0);
    memset(s,0,sizeof(*s)); s->fbi_enabled=fbi; s->scene=fbi?WF_INTRO_SCENE_FBI:WF_INTRO_SCENE_LEGAL;
    s->phase=WF_INTRO_PHASE_HOLD; s->fade_level=WF_ARCADE_BOOT_FADE_STEPS-1U;
    return ev(WF_INTRO_EVENT_PUBLISH_SCENE,s->scene,0);
}
wf_intro_event_t wf_intro_vblank(wf_intro_state_t *s){
    if(s==NULL) return ev(WF_INTRO_EVENT_FAULT,WF_INTRO_SCENE_TITLE_HANDOFF,0);
    if(s->phase==WF_INTRO_PHASE_HANDOFF) return ev(WF_INTRO_EVENT_NONE,WF_INTRO_SCENE_TITLE_HANDOFF,0);
    if(s->phase==WF_INTRO_PHASE_FAULT) return ev(WF_INTRO_EVENT_FAULT,s->scene,0);
    if((unsigned)s->scene>=WF_INTRO_SCENE_COUNT ||
       (s->phase!=WF_INTRO_PHASE_HOLD && s->phase!=WF_INTRO_PHASE_FADE) ||
       s->hold_vblanks>WF_ARCADE_BOOT_WAIT_VBLANKS ||
       s->fade_level>=WF_ARCADE_BOOT_FADE_STEPS ||
       s->fade_frames>=WF_ARCADE_BOOT_FADE_FRAMES_PER_STEP){
        s->phase=WF_INTRO_PHASE_FAULT; return ev(WF_INTRO_EVENT_FAULT,s->scene,0);
    }
    if(s->phase==WF_INTRO_PHASE_HOLD){
        s->hold_vblanks++;
        if(s->hold_vblanks==WF_ARCADE_BOOT_WAIT_VBLANKS){
            s->phase=WF_INTRO_PHASE_FADE; s->fade_level=WF_ARCADE_BOOT_FADE_STEPS-1U; s->fade_frames=0;
            return ev(WF_INTRO_EVENT_APPLY_FADE,s->scene,s->fade_level);
        }
        return ev(WF_INTRO_EVENT_NONE,s->scene,0);
    }
    s->fade_frames++;
    if(s->fade_frames<WF_ARCADE_BOOT_FADE_FRAMES_PER_STEP) return ev(WF_INTRO_EVENT_NONE,s->scene,s->fade_level);
    s->fade_frames=0;
    if(s->fade_level>0U){ s->fade_level--; return ev(WF_INTRO_EVENT_APPLY_FADE,s->scene,s->fade_level); }
    if(s->scene==WF_INTRO_SCENE_TECHNOS){
        s->scene=WF_INTRO_SCENE_TITLE_HANDOFF; s->phase=WF_INTRO_PHASE_HANDOFF;
        return ev(WF_INTRO_EVENT_TITLE_HANDOFF,s->scene,0);
    }
    s->scene=(wf_intro_scene_t)((unsigned)s->scene+1U); s->phase=WF_INTRO_PHASE_HOLD;
    s->hold_vblanks=0; s->fade_level=WF_ARCADE_BOOT_FADE_STEPS-1U;
    return ev(WF_INTRO_EVENT_PUBLISH_SCENE,s->scene,0);
}
bool wf_intro_unpack(const uint8_t *src,size_t size,uint8_t *dst,size_t cap){
    size_t in=0,out=0; if(src==NULL||dst==NULL) return false;
    while(in<size){
        unsigned tag=src[in++]; size_t count=(size_t)(tag&127U)+1U;
        if(count>cap-out) return false;
        if((tag&128U)!=0U){ if(in==size) return false; memset(dst+out,src[in++],count); }
        else { if(count>size-in) return false; memcpy(dst+out,src+in,count); in+=count; }
        out+=count;
    }
    return out==cap;
}
uint32_t wf_intro_fnv1a(const uint8_t *data,size_t size){
    uint32_t h=2166136261U; while(size--!=0U) h=(h^*data++)*16777619U; return h;
}
static uint16_t ex4(unsigned v){ v&=15U; return (uint16_t)((v<<1)|(v>>3)); }
uint16_t wf_intro_fade_rgb1555(uint16_t c,unsigned level){
    if(level>=WF_ARCADE_BOOT_FADE_STEPS) return 0x8000U;
    unsigned r4=(c&31U)>>1,g4=((c>>5)&31U)>>1,b4=((c>>10)&31U)>>1;
    uint16_t r5=ex4(wf_arcade_fade_4bit[r4][level]),g5=ex4(wf_arcade_fade_4bit[g4][level]),b5=ex4(wf_arcade_fade_4bit[b4][level]);
    return (uint16_t)(0x8000U|(b5<<10)|(g5<<5)|r5);
}
void wf_intro_build_fade_palette(const uint8_t src[WF_INTRO_PALETTE_BYTES],unsigned level,uint8_t out[WF_INTRO_PALETTE_BYTES]){
    for(unsigned i=0;i<256U;i++){ uint16_t s=(uint16_t)(((uint16_t)src[i*2U]<<8)|src[i*2U+1U]); uint16_t f=wf_intro_fade_rgb1555(s,level); out[i*2U]=(uint8_t)(f>>8); out[i*2U+1U]=(uint8_t)f; }
}
