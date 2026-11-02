/* Host-only observer over the unchanged coupled full-board context.
 * analyze_full_board_music_structure.py derives capture_board_context.h as
 * the exact prefix of capture_board_replay.c before its main declaration.
 * TimerA+B/busy/IRQ, ROM-addressed OKI and the original CPU stay in that
 * context. No observer read invokes a board MMIO callback or advances time.
 */
#include "capture_board_context.h"

typedef struct {
    board_t board; /* first: unchanged callbacks can keep their board_t cast */
    uint8_t registers[256],keys[8];
    uint64_t timer_b_services,checkpoints,ym_events,oki_events,keyons,last_audio_cycle;
    uint64_t first_global_end_cycle,fade_sent_cycle,fade_clear_cycle;
    uint64_t first_back_jump_cycle[10];
    uint16_t first_back_jump_from[10],first_back_jump_target[10];
    uint32_t global_ends,channel_ends[10],back_jumps[10];
    unsigned pending_checkpoint;
    FILE *snapshots;
} full_observed_t;

static int full_bytes(FILE *f,const void *p,size_t n){return fwrite(p,1,n,f)==n;}
static int full_u32(FILE *f,uint32_t v)
{uint8_t p[4];for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));return full_bytes(f,p,4);}
static int full_u64(FILE *f,uint64_t v)
{uint8_t p[8];for(unsigned i=0;i<8;i++)p[i]=(uint8_t)(v>>(8*i));return full_bytes(f,p,8);}

static int full_snapshot(full_observed_t *o,unsigned canonical)
{
    board_t *b=&o->board;wf_ctx_t *c=&b->original;
    if(!o->snapshots)return 1;
    z80 cpu=c->cpu;reference_oki_t oki=b->oki;
    cpu.cyc=0;cpu.read_byte=NULL;cpu.write_byte=NULL;
    cpu.port_in=NULL;cpu.port_out=NULL;cpu.userdata=NULL;
    oki.rom=NULL;oki.bank_changes_while_active=0;oki.ignored_busy_starts=0;oki.invalid_starts=0;
    uint8_t io[8]={c->ym_addr,c->latch,c->bank,OPM_Read(&b->opm,1),
        OPM_ReadIRQ(&b->opm),reference_oki_status(&b->oki),(uint8_t)b->latch_pending,(uint8_t)b->profile};
    FILE *f=o->snapshots;
    if(!full_u32(f,canonical)||!full_u64(f,now(c)-b->origin)||!full_u64(f,o->timer_b_services)||
       !full_u64(f,o->ym_events)||!full_u64(f,o->oki_events)||!full_u64(f,o->keyons)||
       !full_u64(f,o->last_audio_cycle)||!full_u32(f,o->global_ends))return 0;
    for(unsigned i=0;i<10;i++)if(!full_u32(f,o->channel_ends[i]))return 0;
    for(unsigned i=0;i<10;i++)if(!full_u32(f,o->back_jumps[i]))return 0;
    if(!full_u64(f,o->first_global_end_cycle)||!full_u64(f,o->fade_sent_cycle)||
       !full_u64(f,o->fade_clear_cycle)||!full_bytes(f,&cpu,sizeof cpu)||
       !full_bytes(f,io,sizeof io)||!full_u64(f,b->clock&1)||!full_u64(f,b->out_phase)||
       !full_u64(f,b->oki_phase)||!full_bytes(f,c->ram,sizeof c->ram)||
       !full_bytes(f,o->registers,sizeof o->registers)||!full_bytes(f,o->keys,sizeof o->keys)||
       !full_bytes(f,&b->opm,sizeof b->opm)||!full_bytes(f,&oki,sizeof oki)||
       !full_bytes(f,b->output,sizeof b->output))return 0;
    return 1;
}

static void full_observed_write(void *user,uint16_t address,uint8_t value)
{
    full_observed_t *o=user;board_t *b=&o->board;wf_ctx_t *c=&b->original;
    uint8_t reg=c->ym_addr;
    board_write(b,address,value);
    if(address==0xc801){
        o->registers[reg]=value;
        if(reg==8){o->keys[value&7]=(value>>3)&15;if(b->capturing&&(value&0x78))o->keyons++;}
        if(b->capturing&&reg!=0x14){o->ym_events++;o->last_audio_cycle=now(c)-b->origin;}
        /* OPM_Write queues the reset; this read observes the live flag before
         * the next chip clock consumes it. A later PC003A requires it clear. */
        if(b->capturing&&reg==0x14&&(value&0x20)&&(OPM_Read(&b->opm,1)&2)){
            o->timer_b_services++;o->pending_checkpoint=1;
        }
    }else if(b->capturing&&address==0xd800){o->oki_events++;o->last_audio_cycle=now(c)-b->origin;}
    if(b->capturing&&address==0xc006&&!value&&o->fade_sent_cycle&&!o->fade_clear_cycle)
        o->fade_clear_cycle=now(c)-b->origin;
}

static void full_observe_before(full_observed_t *o)
{
    board_t *b=&o->board;wf_ctx_t *c=&b->original;
    if(!b->capturing||c->cpu.ix>=0xc000)return;
    unsigned channel=c->ram[0x65],token=c->rom[c->cpu.ix];
    if(c->cpu.pc==0x0e80&&token==0xfb){
        if(!o->global_ends)o->first_global_end_cycle=now(c)-b->origin;
        o->global_ends++;
    }
    if(c->cpu.pc==0x0d2d&&token==0xf0&&channel<10)o->channel_ends[channel]++;
    if(c->cpu.pc==0x0e17&&token==0xfd&&channel<10&&c->cpu.ix<0xbffe){
        unsigned target=c->rom[c->cpu.ix+1]|((unsigned)c->rom[c->cpu.ix+2]<<8);
        if(target<=c->cpu.ix){
            if(!o->back_jumps[channel]){
                o->first_back_jump_cycle[channel]=now(c)-b->origin;
                o->first_back_jump_from[channel]=c->cpu.ix;
                o->first_back_jump_target[channel]=(uint16_t)target;
            }
            o->back_jumps[channel]++;
        }
    }
}

static void full_observe_step(full_observed_t *o)
{
    board_t *b=&o->board;wf_ctx_t *c=&b->original;
    full_observe_before(o);board_step(b);
    if(o->pending_checkpoint&&c->cpu.pc==0x003a&&!(OPM_Read(&b->opm,1)&2)&&!c->cpu.nmi_pending){
        if(!full_snapshot(o,1)){fprintf(stderr,"State snapshot write failure\n");exit(10);}
        o->pending_checkpoint=0;o->checkpoints++;
    }
}

#ifndef WF_FULL_BOARD_MUSIC_STATE_NO_MAIN
int main(int argc,char **argv)
{
    if(argc!=11){fprintf(stderr,"usage: %s SOUND.ROM SAMPLES.ROM SCHEDULE.csv SECONDS TRACE.csv PCM16BE REPORT.json DELIVERIES.csv full STATES.bin\n",argv[0]);return 2;}
    unsigned seconds=(unsigned)strtoul(argv[4],NULL,0);
    if(!seconds||seconds>1800||sizeof(unsigned long)<8||strcmp(argv[9],"full"))return 2;
    if(!board_outputs_are_new(argv))return 8;
    for(unsigned i=5;i<=8;i++)if(!strcmp(argv[10],argv[i]))return 8;
    struct stat status;
    if(!stat(argv[10],&status)||errno!=ENOENT){fprintf(stderr,"Refusing unavailable snapshot path\n");return 8;}
    board_command_t commands[4096];unsigned count=0;char line[256];
    FILE *schedule=fopen(argv[3],"r");if(!schedule)return 3;
    uint64_t fade_sample=UINT64_MAX;
    while(fgets(line,sizeof line,schedule)){
        unsigned long long sample;unsigned word;
        if(sscanf(line,"%llu,%u",&sample,&word)!=2||count==4096||word>65535||!(word&255)||
           (word&255)>0x81||(count&&sample<=commands[count-1].sample))return 4;
        if((word&255)==0x1f&&fade_sample==UINT64_MAX)fade_sample=sample;
        commands[count++]=(board_command_t){sample,word};
    }
    fclose(schedule);
    if(!count||commands[0].sample||commands[0].word<1||commands[0].word>18||
       commands[count-1].sample>=(uint64_t)seconds*44100)return 4;
    full_observed_t *o=calloc(1,sizeof *o);uint8_t *samples=malloc(0x80000);
    if(!o||!samples||!load(argv[1],o->board.original.rom,sizeof o->board.original.rom)||!load(argv[2],samples,0x80000))return 5;
    board_t *b=&o->board;wf_ctx_t *c=&b->original;
    b->profile=PROFILE_FULL;c->log=fopen(argv[5],"wx");b->pcm=fopen(argv[6],"wbx");
    FILE *report=fopen(argv[7],"wx");b->deliveries=fopen(argv[8],"wx");o->snapshots=fopen(argv[10],"wbx");
    if(!c->log||!b->pcm||!report||!b->deliveries||!o->snapshots)return 5;
    fprintf(c->log,"cycle,kind,a,b\n");fprintf(b->deliveries,"index,delivery_relative_cycle,latch_read_relative_cycle\n");
    OPM_Reset(&b->opm,opm_flags_none);reference_oki_reset(&b->oki,samples,0x80000);
    z80_init(&c->cpu);c->cpu.read_byte=board_read;c->cpu.write_byte=full_observed_write;
    c->cpu.port_in=pin;c->cpu.port_out=pout;c->cpu.userdata=o;
    while(now(c)<2*WF_CLK)full_observe_step(o);
    b->origin=now(c);b->capturing=1;loge(c,"ORIGIN",0,0);
    if(!full_bytes(o->snapshots,"WFMB0001",8)||!full_u32(o->snapshots,(uint32_t)sizeof(z80))||
       !full_u32(o->snapshots,(uint32_t)sizeof(opm_t))||!full_u32(o->snapshots,(uint32_t)sizeof(reference_oki_t))||
       !full_u32(o->snapshots,commands[0].word)||!full_u64(o->snapshots,b->origin)||
       !full_u64(o->snapshots,fade_sample)||!full_u64(o->snapshots,(uint64_t)seconds*WF_CLK))return 10;
    uint64_t end=b->origin+(uint64_t)seconds*WF_CLK;unsigned index=0;
    while(now(c)<end){
        if(index<count){
            uint64_t target=b->origin+(commands[index].sample*WF_CLK+44100-1)/44100;
            if(now(c)>=target){
                if(b->latch_pending||c->cpu.nmi_pending){fprintf(stderr,"Pending latch/NMI at measured command%u\n",index);return 6;}
                b->latch_pending=1;b->command_index=index;b->latch_delivery=now(c);
                c->latch=(uint8_t)commands[index].word;loge(c,"CMD",index,commands[index].word);
                if(c->latch==0x1f&&!o->fade_sent_cycle)o->fade_sent_cycle=now(c)-b->origin;
                z80_gen_nmi(&c->cpu);index++;
            }
        }
        full_observe_step(o);
    }
    loge(c,"END",0,0);
    if(!full_snapshot(o,0))return 10;
    int valid=index==count&&!b->latch_pending&&!c->cpu.nmi_pending&&!b->oki.fault;
    fprintf(report,"{\"profile\":\"full\",\"complete\":%s,\"commands\":%u,\"origin_cycle\":%llu,\"end_cycle\":%llu,\"frames\":%llu,\"peak\":%llu,\"clipped_frames\":%llu,\"ym_status_reads\":%llu,\"ym_busy_reads\":%llu,\"oki_status_reads\":%llu,\"oki_busy_reads\":%llu,\"instruction_checkpoints_with_timer_a_flag\":%llu,\"instruction_checkpoints_with_timer_b_flag\":%llu,\"bank_changes_while_pcm_active\":%llu,\"ignored_busy_starts\":%llu,\"invalid_sample_starts\":%llu,\"timer_b_services\":%llu,\"state_checkpoints\":%llu,\"backward_branch_first\":[",
        valid?"true":"false",count,(unsigned long long)b->origin,(unsigned long long)now(c),
        (unsigned long long)b->frames,(unsigned long long)b->peak,(unsigned long long)b->clipped,
        (unsigned long long)b->ym_reads,(unsigned long long)b->busy_ym_reads,
        (unsigned long long)b->oki_reads,(unsigned long long)b->busy_oki_reads,
        (unsigned long long)b->irq_a,(unsigned long long)b->irq_b,
        (unsigned long long)b->oki.bank_changes_while_active,(unsigned long long)b->oki.ignored_busy_starts,
        (unsigned long long)b->oki.invalid_starts,(unsigned long long)o->timer_b_services,(unsigned long long)o->checkpoints);
    for(unsigned i=0;i<10;i++)fprintf(report,"%s{\"channel\":%u,\"count\":%u,\"cycle\":%llu,\"from\":%u,\"target\":%u}",
        i?",":"",i,o->back_jumps[i],(unsigned long long)o->first_back_jump_cycle[i],
        o->first_back_jump_from[i],o->first_back_jump_target[i]);
    fprintf(report,"]}\n");
    int close_failed=fclose(c->log)!=0;
    close_failed|=fclose(b->pcm)!=0;close_failed|=fclose(report)!=0;close_failed|=fclose(b->deliveries)!=0;
    close_failed|=fclose(o->snapshots)!=0;
    if(close_failed){fprintf(stderr,"Observer output close failure\n");free(samples);free(o);return 10;}
    free(samples);free(o);return valid?0:7;
}
#endif
