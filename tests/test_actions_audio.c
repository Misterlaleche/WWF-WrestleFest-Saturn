/* Production native action callbacks, with only the audio device captured. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r17_actions_r11.h"

static wf_runtime_t runtime;
static wf_r17_actions_r11_bridge_t bridge;
static wf_r17_render_ctx_t render;
static wf_r17_render_queue_t queue;
static unsigned command_count,failures;
static uint16_t commands[16];
static int tracing;
static unsigned fixture[5],patch_count;
static uint32_t patches[24][3];

static void patch(unsigned slot,unsigned offset,unsigned width,uint32_t value)
{
    assert(slot<WF_ACTOR_SLOTS&&patch_count<24);
    patches[patch_count][0]=0x1c05b0u+slot*0x10cu+offset;
    patches[patch_count][1]=width;patches[patch_count++][2]=value;
    if(width==1)wf_set_u8(&runtime.actors[slot],offset,(uint8_t)value);
    else if(width==2)wf_set_be16(&runtime.actors[slot],offset,(uint16_t)value);
    else {assert(width==4);wf_set_be32(&runtime.actors[slot],offset,value);}
}

static void trace_fixture(unsigned credited,unsigned mode,unsigned dip)
{
    printf("TRACE patch");
    for(unsigned i=0;i<5;i++)printf(" %u",fixture[i]);
    printf(" %u %u %u %u",credited,mode,dip,patch_count);
    for(unsigned i=0;i<patch_count;i++)
        printf(" %u %u %u",patches[i][0],patches[i][1],patches[i][2]);
    for(unsigned i=0;i<command_count;i++)printf(" %u",commands[i]);
    putchar('\n');
}


static wf_status_t capture(wf_runtime_t *rt,uint16_t command,void *user)
{
    (void)user;assert(command_count<16);commands[command_count++]=command;
    /* An output callback is not allowed to leak device registers into RNG. */
    for(unsigned i=0;i<8;i++)rt->m68k_regs.d[i]=0xa0a00000u+i;
    return WF_OK;
}
static wf_status_t video(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u)
{(void)r;(void)w;(void)n;(void)u;return WF_OK;}
static wf_status_t resource(wf_runtime_t*r,uint16_t w,void*u)
{(void)r;(void)w;(void)u;return WF_OK;}

static wf_actor_raw_t *setup(unsigned selector,unsigned frame,unsigned timer,unsigned flags,unsigned contact)
{
    wf_runtime_init(&runtime,NULL,0);memset(&bridge,0,sizeof bridge);
    memset(&render,0,sizeof render);memset(&queue,0,sizeof queue);
    command_count=0;memset(commands,0,sizeof commands);patch_count=0;
    fixture[0]=selector;fixture[1]=frame;fixture[2]=timer;fixture[3]=flags;fixture[4]=contact;
    runtime.g.credited_route=1;runtime.g.mode_0161=1;
    runtime.g.rng_state=1085;runtime.g.rng_state_valid=1;
    runtime.g.rng_register_context_valid=1;
    for(unsigned i=0;i<8;i++)runtime.m68k_regs.d[i]=0x12340000u+i;
    runtime.m68k_regs.valid=1;
    wf_runtime_bind_audio(&runtime,capture,NULL);
    bridge.fixed_object0=&runtime.fixed_object0;bridge.fixed_object1=&runtime.fixed_object1;
    render.queue=&queue;render.visibility_2188=wf_r17_visibility_2188_exact;
    bridge.render=&render;bridge.video_batch=video;
    bridge.resource_2b58=resource;bridge.resource_2aea=resource;
    for(unsigned i=0;i<WF_ACTOR_SLOTS;i++){
        wf_actor_raw_t *a=&runtime.actors[i];
        wf_set_u8(a,WF_A_ACTIVE,0x80u);
        wf_set_be16(a,WF_A_STATE,0);
        wf_set_be16(a,WF_A_X,0x280u);wf_set_be16(a,WF_A_Y,0x160u);wf_set_be16(a,WF_A_Z,0x140u);
        wf_set_be16(a,WF_A_STAMINA,50);wf_set_be16(a,WF_A_STAMINA_MAX,100);
        wf_set_be32(a,WF_A_LINK,0x1c05b0u+((i+1u)%WF_ACTOR_SLOTS)*0x10cu);
        wf_set_be32(a,WF_A_TEAM_LINK,0x1c05b0u+((i+2u)%WF_ACTOR_SLOTS)*0x10cu);
        wf_set_be32(a,WF_A_TARGET,0x1c05b0u+((i+1u)%WF_ACTOR_SLOTS)*0x10cu);
        wf_set_be32(a,WF_A_RECIP_LINK,0x1c06bcu);
    }
    wf_actor_raw_t *a=&runtime.actors[0];
    wf_set_be16(a,WF_A_STATE,5);wf_set_be16(a,WF_A_LATCHED,0x8005u);
    wf_set_be16(a,WF_A_SELECTOR,(uint16_t)selector);
    wf_set_be16(a,WF_A_ANIM_INDEX,(uint16_t)frame);
    wf_set_be16(a,WF_A_ANIM_COUNTER,(uint16_t)timer);
    wf_set_u8(a,WF_A_FLAGS33,(uint8_t)flags);
    wf_set_be16(a,WF_A_CONTACT,(uint16_t)contact);
    return a;
}

static void expect(unsigned selector,unsigned frame,unsigned timer,unsigned flags,
                   unsigned contact,unsigned count,uint16_t first,uint16_t second)
{
    setup(selector,frame,timer,flags,contact);
    wf_status_t status=wf_r17_dispatch_1c03e_r11(&runtime,0,&bridge);
    if(status!=WF_OK||command_count!=count||(count&&commands[0]!=first)||
       (count>1&&commands[1]!=second)){
        fprintf(stderr,"FAIL action %02X frame %u timer %u flags %02X contact %02X: status %u, commands %u [%04X,%04X], wanted %u [%04X,%04X]\n",
            selector,frame,timer,flags,contact,status,command_count,commands[0],commands[1],count,first,second);
        ++failures;
    }
    if(status==WF_OK)assert(runtime.m68k_regs.d[0]==0x12340000u);
    assert(runtime.audio_status==WF_OK);
    if(tracing)trace_fixture(1,1,0);
}

/* Hash all typed gameplay state, including every actor, fixed object, RNG and
 * live register, plus dispatcher status/trace. The output callback/pointers are
 * deliberately outside this prefix. Frozen and repaired actions must agree. */
static uint64_t hash_bytes(uint64_t h,const void *data,size_t length)
{
    const unsigned char *p=data;
    for(size_t i=0;i<length;i++){h^=p[i];h*=UINT64_C(1099511628211);}
    return h;
}
static unsigned matrix_cases,matrix_blocked;
static void matrix_case(int snapshot)
{
    unsigned credited=runtime.g.credited_route,mode=runtime.g.mode_0161,dip=runtime.g.config_0066;
    wf_status_t status=wf_r17_dispatch_1c03e_r11(&runtime,0,&bridge);
    matrix_cases++;
    if(snapshot){
        uint64_t h=hash_bytes(UINT64_C(14695981039346656037),&runtime,offsetof(wf_runtime_t,bindings));
        h=hash_bytes(h,&status,sizeof status);h=hash_bytes(h,&bridge.trace[0],sizeof bridge.trace[0]);
        printf("STATE %u %016llx\n",matrix_cases,(unsigned long long)h);
    }else if(status==WF_OK){
        assert(runtime.audio_status==WF_OK);
        assert(runtime.m68k_regs.d[0]==0x12340000u);
        if(tracing)trace_fixture(credited,mode,dip);
    }else {assert(status==WF_BLOCKED);matrix_blocked++;}
}

static void action_matrix(int snapshot)
{
    unsigned selectors[0x8f];
    for(unsigned i=0;i<sizeof selectors/sizeof selectors[0];i++)selectors[i]=i;
    static const unsigned frames[]={0,1,2,3,4,5,6,7,8,0xfe,0x100,0x104};
    static const unsigned timers[]={0,1,0x100};
    for(unsigned si=0;si<sizeof selectors/sizeof selectors[0];si++)
    for(unsigned fi=0;fi<sizeof frames/sizeof frames[0];fi++)
    for(unsigned ti=0;ti<sizeof timers/sizeof timers[0];ti++)
    for(unsigned variant=0;variant<4;variant++){
        unsigned selector=selectors[si];
        setup(selector,frames[fi],timers[ti],ti==1?4:0,variant==0?0:0x10);
        if(variant==2){
            patch(1,0x20,2,4);patch(1,0x64,2,(fi&1u)?9:8);
            patch(1,0x26,4,0x1c05b0u);
        }else if(variant==3){patch(1,0x20,2,5);patch(1,0x60,2,0x61);}
        if(selector==0x0e||selector==0x35)patch(0,0x44,2,3);
        matrix_case(snapshot);
    }
    /* Shared running/edge phases, both descriptor entries and each timing edge. */
    for(unsigned selector=0x0e;selector<=0x35;selector+=0x27)
    for(unsigned phase=0;phase<3;phase++)
    for(unsigned frame=0;frame<6;frame++)
    for(unsigned timer=0;timer<2;timer++)
    for(unsigned edge=0;edge<2;edge++){
        setup(selector,frame,timer,0,edge?0x0102:0);
        patch(0,0x44,2,phase);matrix_case(snapshot);
    }
    /* The latch/selector/role gates include negative cases and high-byte timers. */
    for(unsigned si=0;si<sizeof selectors/sizeof selectors[0];si++)
    for(unsigned variant=0;variant<4;variant++){
        unsigned selector=selectors[si];setup(selector,1,0,0,0x10);
        patch(0,0x1c,2,variant&1?5:0x8005);
        patch(0,0x60,2,selector|((variant&2)?0x8000:0));
        patch(1,0x26,4,0x1c05b0u);matrix_case(snapshot);
    }
    /* raw0A's common impact command joins six distinct interrupted holds. */
    static const unsigned interrupted[]={0x8048,0x800e,0x801a,0x8009,0x8061,0x8049};
    for(unsigned i=0;i<sizeof interrupted/sizeof interrupted[0];i++){
        setup(0x0a,0,0,0,0);patch(1,0x20,2,5);patch(1,0x60,2,interrupted[i]);
        matrix_case(snapshot);
    }
    /* raw1B/raw0D share three target routes; raw3B has separate pursuit/hold phases. */
    for(unsigned selector=0x0d;selector<=0x1b;selector+=0x0e)
    for(unsigned route=0;route<3;route++){
        setup(selector,3,1,0,0x10);patch(0,0x44,2,route);
        if(route==1){patch(1,0x20,2,5);patch(1,0x60,2,0x53);}
        matrix_case(snapshot);
    }
    for(unsigned phase=1;phase<=2;phase++)
    for(unsigned latched=0;latched<2;latched++)
    for(unsigned frame=0;frame<2;frame++){
        setup(0x203b,frame?0xfe:0,0,0,0);patch(0,0x44,2,phase);
        patch(0,0x1c,2,latched?0x8005:5);patch(1,0x60,2,0x808d);
        matrix_case(snapshot);
    }
    /* TAG result confirmation emits both original $111C8 commands. */
    static const unsigned result_selectors[]={0x09,0x1c,0x22,0x36,0x3b};
    for(unsigned i=0;i<sizeof result_selectors/sizeof result_selectors[0];i++){
        unsigned selector=result_selectors[i];
        setup(selector|0x0800u|(selector==0x3b?0x2000u:0),0xfe,0,1,0);
        runtime.g.mode_0161=0;patch(1,0x33,1,1);patch(1,0x66,2,0);
        if(selector==0x3b)patch(0,0x44,2,2);
        matrix_case(snapshot);
    }
    /* Shared $10FC6 outside contact: 28 then32, followed by the callback's cue. */
    static const unsigned edge_selectors[]={0x01,0x04,0x8d,0x40,0x75};
    for(unsigned i=0;i<sizeof edge_selectors/sizeof edge_selectors[0];i++){
        setup(edge_selectors[i],1,0,0,0x0310);matrix_case(snapshot);
    }
    /* Attract DIP and credited gameplay use the production $2052 gate. */
    for(unsigned credited=0;credited<2;credited++)for(unsigned dip=0;dip<2;dip++){
        setup(0x55,1,0,0,0);runtime.g.credited_route=(uint16_t)credited;
        runtime.g.config_0066=(uint16_t)(dip?0x20:0);matrix_case(snapshot);
    }
    if(!snapshot)printf("PASS native action matrix: %u fixtures, %u existing blocked dependencies\n",matrix_cases,matrix_blocked);
}

int main(int argc,char **argv)
{
    if(argc>1&&strcmp(argv[1],"--state")==0){action_matrix(1);return 0;}
    tracing=argc>1&&strcmp(argv[1],"--traces")==0;
    /* ROM $165A4..$165B6: linked grapple cue, only frame1/timer0. */
    expect(0x29,1,0,0,0,1,0x312a,0);
    expect(0x29,1,1,0,0,0,0,0);
    expect(0x29,0,0,0,0,0,0,0);
    /* Throw/hold cues and frame-edge negatives are literal ROM commands. */
    expect(0x24,2,0,0,0,1,0x3132,0);
    expect(0x24,2,1,0,0,0,0,0);
    expect(0x55,1,0,0,0,1,0x312a,0);
    expect(0x55,1,1,0,0,0,0,0);
    expect(0x56,0,0,0,0,1,0x3129,0);
    expect(0x56,0,1,0,0,0,0,0);
    expect(0x5a,0,0,0,0,1,0x312b,0);
    expect(0x5b,1,0,0,0,1,0x3129,0);
    expect(0x8b,0,0,0,0,1,0x3130,0);
    expect(0x8b,0,1,0,0,0,0,0);
    /* $1110E landing choice: ringside flag selects 2D, ring selects29. */
    expect(0x01,3,1,0,0x10,1,0x3129,0);
    expect(0x01,3,1,4,0x10,1,0x312d,0);
    expect(0x01,3,1,0,0,0,0,0);
    expect(0x26,3,1,0,0x10,1,0x3129,0);
    expect(0x26,4,0,0,0,1,0x3132,0);
    expect(0x40,2,1,0,0x10,1,0x3129,0);
    expect(0x40,2,1,4,0x10,1,0x312d,0);
    expect(0x40,2,1,0,0,0,0,0);
    /* Two simultaneous original commands keep their original order. */
    expect(0x02,1,0,0,0,2,0x312b,0x3132);
    expect(0x28,2,1,0,0x10,2,0x312b,0x3132);
    /* $1112E footsteps occur only at frame1/frame4 timer0. */
    expect(0x05,1,0,0,0,1,0x3133,0);
    expect(0x05,4,0,0,0,1,0x3133,0);
    expect(0x05,1,1,0,0,0,0,0);
    expect(0x05,2,0,0,0,0,0,0);
    /* Additional original landing, grapple and fall callback sites. */
    expect(0x04,2,1,0,0x10,1,0x3129,0);
    expect(0x19,4,1,0,0x10,1,0x3129,0);
    expect(0x1a,4,0,0,0,1,0x3129,0);
    expect(0x1a,4,1,0,0,0,0,0);
    expect(0x2e,5,1,0,0x10,1,0x3129,0);
    expect(0x30,3,0,0,0,1,0x312b,0);
    expect(0x30,3,1,0,0,0,0,0);
    expect(0x30,4,0,0,0,1,0x3132,0);
    expect(0x34,5,1,0,0x10,1,0x3129,0);
    expect(0x43,5,1,0,0x10,1,0x3129,0);
    expect(0x43,0xfe,1,0,0,1,0x3132,0);
    expect(0x8d,2,1,0,0x10,1,0x3129,0);
    expect(0x68,1,0,0,0x10,0,0,0);
    expect(0x68,4,0,0,0x10,0,0,0);
    if(failures)return 1;
    puts("PASS original native action sound gates and preserved caller D0");
    action_matrix(0);
    return 0;
}
