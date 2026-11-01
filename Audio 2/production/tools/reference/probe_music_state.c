/* Host-only original score observation. No production sequence/synth code.
 * The original full-Z80 oracle context is included unchanged. Snapshots require
 * both PC003A and an acknowledged TimerB, avoiding pre-service idle snapshots.
 */
#define main historical_capture_main
#include "oracle_a53/tools/capture_full_audio_a50.c"
#undef main

typedef struct {
    wf_ctx_t original; /* first, so the unchanged callbacks retain their context */
    uint8_t registers[256],keys[8];
    uint64_t origin,tick,ym_events,oki_events,keyons,last_audio_cycle;
    uint64_t first_global_end_cycle,fade_sent_cycle,fade_clear_cycle;
    uint32_t global_ends,channel_ends[10],back_jumps[10];
    int pending_tick,capturing;
} observed_t;

static void observed_write(void *user,uint16_t address,uint8_t value)
{
    observed_t *o=user;wf_ctx_t *c=&o->original;
    if(address==0xc006&&value==0&&o->fade_sent_cycle&&!o->fade_clear_cycle)
        o->fade_clear_cycle=now(c)-o->origin;
    if(address==0xc801){
        uint8_t reg=c->ym_addr;o->registers[reg]=value;
        if(reg==8){o->keys[value&7]=(value>>3)&15;if(value&0x78)o->keyons++;}
        if(o->capturing&&reg!=0x14){o->ym_events++;o->last_audio_cycle=now(c)-o->origin;}
    }else if(address==0xd800&&o->capturing){o->oki_events++;o->last_audio_cycle=now(c)-o->origin;}
    wb(c,address,value);
}

static void observe_step(observed_t *o)
{
    wf_ctx_t *c=&o->original;
    if(o->capturing){
        unsigned channel=c->ram[0x65];
        if(c->cpu.pc==0x0e80&&rb(c,c->cpu.ix)==0xfb){
            if(!o->global_ends)o->first_global_end_cycle=now(c)-o->origin;
            o->global_ends++;
        }
        if(c->cpu.pc==0x0d2d&&rb(c,c->cpu.ix)==0xf0&&channel<10)o->channel_ends[channel]++;
        if(c->cpu.pc==0x0e17&&rb(c,c->cpu.ix)==0xfd&&channel<10){
            unsigned target=rb(c,c->cpu.ix+1)|((unsigned)rb(c,c->cpu.ix+2)<<8);
            if(target<=c->cpu.ix)o->back_jumps[channel]++;
        }
    }
    z80_step(&c->cpu);
    uint64_t before=c->next_tb;
    int due=(c->ym_ctl&2u)&&now(c)>=before;
    update_timer(c);
    if(due){o->tick+=(c->next_tb-before)/((256u-c->ym_tb)*1024u);o->pending_tick=1;}
}

static int bytes(FILE *f,const void *p,size_t n){return fwrite(p,1,n,f)==n;}
static int u32(FILE *f,uint32_t v){uint8_t p[4];for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));return bytes(f,p,4);}
static int u64(FILE *f,uint64_t v){uint8_t p[8];for(unsigned i=0;i<8;i++)p[i]=(uint8_t)(v>>(8*i));return bytes(f,p,8);}
static int snapshot(observed_t *o,FILE *f)
{
    wf_ctx_t *c=&o->original;z80 cpu=c->cpu;
    cpu.cyc=0;cpu.read_byte=NULL;cpu.write_byte=NULL;cpu.port_in=NULL;cpu.port_out=NULL;cpu.userdata=NULL;
    uint8_t io[6]={c->ym_addr,c->ym_status,c->ym_tb,c->ym_ctl,c->latch,c->bank};
    if(!u64(f,now(c)-o->origin)||!u64(f,o->tick)||!u64(f,o->ym_events)||
       !u64(f,o->oki_events)||!u64(f,o->keyons)||!u64(f,o->last_audio_cycle)||!u32(f,o->global_ends))return 0;
    for(unsigned i=0;i<10;i++)if(!u32(f,o->channel_ends[i]))return 0;
    for(unsigned i=0;i<10;i++)if(!u32(f,o->back_jumps[i]))return 0;
    if(!u64(f,o->first_global_end_cycle)||!u64(f,o->fade_sent_cycle)||!u64(f,o->fade_clear_cycle))return 0;
    if(!bytes(f,&cpu,sizeof cpu)||!bytes(f,io,sizeof io)||
       !u64(f,(c->ym_ctl&2)?c->next_tb-now(c):0)||!bytes(f,c->ram,sizeof c->ram)||
       !bytes(f,o->registers,sizeof o->registers)||!bytes(f,o->keys,sizeof o->keys))return 0;
    return 1;
}

#ifndef WF_MUSIC_STATE_PROBE_NO_MAIN
int main(int argc,char **argv)
{
    if(argc!=6){fprintf(stderr,"usage: %s SOUND.ROM COMMAND SECONDS TAIL_SECONDS OUTPUT.bin\n",argv[0]);return 2;}
    unsigned command=(unsigned)strtoul(argv[2],NULL,0),seconds=(unsigned)strtoul(argv[3],NULL,0),tail=(unsigned)strtoul(argv[4],NULL,0);
    if(command<1||command>18||!seconds||seconds>1800||tail>60||sizeof(unsigned long)<8)return 2;
    observed_t *o=calloc(1,sizeof *o);if(!o||!load(argv[1],o->original.rom,sizeof o->original.rom))return 3;
    wf_ctx_t *c=&o->original;c->log=fopen("/dev/null","w");if(!c->log)return 4;
    FILE *out=fopen(argv[5],"wb");if(!out)return 4;
    z80_init(&c->cpu);c->cpu.read_byte=rb;c->cpu.write_byte=observed_write;
    c->cpu.port_in=pin;c->cpu.port_out=pout;c->cpu.userdata=o;
    while(now(c)<2*WF_CLK)observe_step(o);
    o->origin=now(c);o->tick=0;o->pending_tick=0;o->capturing=1;o->keyons=0;
    c->latch=(uint8_t)command;z80_gen_nmi(&c->cpu);
    uint64_t fade=o->origin+(uint64_t)seconds*WF_CLK,end=fade+(uint64_t)tail*WF_CLK;
    if(!bytes(out,"WFMS0002",8)||!u32(out,(uint32_t)sizeof(z80))||!u32(out,command)||
       !u64(out,o->origin)||!u64(out,(uint64_t)seconds*WF_CLK)||!u64(out,(uint64_t)tail*WF_CLK))return 5;
    int faded=0;unsigned next_status=60;
    while(now(c)<end){
        if(tail&&!faded&&now(c)>=fade){c->latch=0x1f;z80_gen_nmi(&c->cpu);faded=1;o->fade_sent_cycle=now(c)-o->origin;}
        observe_step(o);
        if(o->pending_tick&&c->cpu.pc==0x003a&&!(c->ym_status&2u)){
            if(!snapshot(o,out))return 5;
            o->pending_tick=0;
        }
        if(now(c)-o->origin>=(uint64_t)next_status*WF_CLK){
            fprintf(stderr,"command%02X original%us\n",command,next_status);next_status+=60;
        }
    }
    if(!snapshot(o,out)||fclose(out)||fclose(c->log))return 5;
    fprintf(stderr,"command%02X snapshots completed, active=%u, ends=%u, fade=%d\n",command,c->ram[6],o->global_ends,faded);
    free(o);return 0;
}
#endif
