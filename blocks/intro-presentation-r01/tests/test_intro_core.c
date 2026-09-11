#include "wf_intro_presentation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void route(bool fbi){
    wf_intro_state_t s;wf_intro_event_t e=wf_intro_init(&s,fbi);
    assert(e.type==WF_INTRO_EVENT_PUBLISH_SCENE);
    assert(e.scene==(fbi?WF_INTRO_SCENE_FBI:WF_INTRO_SCENE_LEGAL));
    unsigned pubs=1,fades=0,vb=0,id=0,black=0;
    while(s.phase!=WF_INTRO_PHASE_HANDOFF){
        e=wf_intro_vblank(&s);vb++;
        if(e.type==WF_INTRO_EVENT_PUBLISH_SCENE)pubs++;
        else if(e.type==WF_INTRO_EVENT_APPLY_FADE){fades++;id+=e.fade_level==7U;black+=e.fade_level==0U;}
    }
    assert(e.type==WF_INTRO_EVENT_TITLE_HANDOFF);
    assert(pubs==(fbi?3U:2U));assert(fades==(fbi?24U:16U));
    assert(id==(fbi?3U:2U));assert(black==(fbi?3U:2U));assert(vb==(fbi?456U:304U));
}
static void fade(void){
    for(unsigned c=0;c<16U;c++){uint16_t c5=(uint16_t)((c<<1)|(c>>3));uint16_t w=(uint16_t)(0x8000U|(c5<<10)|(c5<<5)|c5);assert(wf_intro_fade_rgb1555(w,7)==w);assert(wf_intro_fade_rgb1555(w,0)==0x8000U);}
}
static void fault(void){
    wf_intro_state_t s;wf_intro_event_t e=wf_intro_init(&s,true);assert(e.type==WF_INTRO_EVENT_PUBLISH_SCENE);
    s.scene=WF_INTRO_SCENE_TITLE_HANDOFF;e=wf_intro_vblank(&s);assert(e.type==WF_INTRO_EVENT_FAULT);assert(s.phase==WF_INTRO_PHASE_FAULT);
    assert(wf_intro_vblank(&s).type==WF_INTRO_EVENT_FAULT);assert(wf_intro_vblank(NULL).type==WF_INTRO_EVENT_FAULT);
}
static void unpack(void){
    const uint8_t enc[]={2,10,20,30,0x82,40,0,50},exp[]={10,20,30,40,40,40,50};uint8_t out[7];
    assert(wf_intro_unpack(enc,sizeof(enc),out,sizeof(out)));assert(memcmp(out,exp,sizeof(out))==0);
    assert(!wf_intro_unpack(enc,sizeof(enc),out,sizeof(out)-1U));assert(!wf_intro_unpack(enc,sizeof(enc)-1U,out,sizeof(out)));
}
int main(void){route(true);route(false);fade();fault();unpack();puts("PASS: intro route, FBI gate, 128+24 timing, exact fade endpoints, fail-closed, RLE bounds");return 0;}
