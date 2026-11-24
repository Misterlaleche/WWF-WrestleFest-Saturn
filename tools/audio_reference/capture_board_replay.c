/* Coupled host diagnostic: original full Z80 + pinned Nuked-OPM at external/2
 * + original ROM-addressed MSM6295. No Saturn software FM synthesis.
 * CPU bus events retain the preserved Z80 core's instruction-level granularity.
 */
#define main historical_capture_main
#include "oracle_a53/tools/capture_full_audio_a50.c"
#undef main
#include "opm.h"
#include "reference_oki.h"
#include <errno.h>
#include <sys/stat.h>

/* Recorded commands may be replayed without the Python new-directory guard.
 * Refuse every existing output before opening any of them; exclusive fopen
 * modes below also protect against a path appearing after this preflight. */
static int board_outputs_are_new(char **argv)
{
    for(unsigned i=5;i<=8;i++){
        for(unsigned j=5;j<i;j++)if(!strcmp(argv[i],argv[j])){
            fprintf(stderr,"Duplicate output path: %s\n",argv[i]);return 0;
        }
        struct stat status;
        if(!stat(argv[i],&status)){
            fprintf(stderr,"Refusing existing output: %s\n",argv[i]);return 0;
        }
        if(errno!=ENOENT){fprintf(stderr,"Cannot establish new output: %s\n",argv[i]);return 0;}
    }
    return 1;
}

enum {PROFILE_FULL,PROFILE_MASK_A,PROFILE_A53_MMIO};
typedef struct {uint64_t sample;unsigned word;} board_command_t;
typedef struct {
    wf_ctx_t original;
    opm_t opm;reference_oki_t oki;
    uint64_t clock,origin,out_phase,oki_phase,frames,latch_delivery;
    unsigned output_rate;
    uint64_t irq_a,irq_b,oki_reads,busy_oki_reads,ym_reads,busy_ym_reads;
    uint64_t clipped,peak,last_oki_status;
    int32_t output[2];
    int capturing,profile,latch_pending;unsigned command_index;
    FILE *pcm,*deliveries;
    uint8_t pcm_buffer[65536];
    unsigned pcm_used;
} board_t;

static void board_pcm_flush(board_t *b)
{
    if(b->pcm_used){
        if(fwrite(b->pcm_buffer,1,b->pcm_used,b->pcm)!=b->pcm_used){fprintf(stderr,"PCM write failure\n");exit(10);}
        b->pcm_used=0;
    }
}

static void board_pcm_sample(board_t *b,int64_t mixed)
{
    if(b->pcm_used+2u>sizeof b->pcm_buffer)board_pcm_flush(b);
    b->pcm_buffer[b->pcm_used++]=(uint8_t)((uint16_t)mixed>>8);
    b->pcm_buffer[b->pcm_used++]=(uint8_t)mixed;
}

static void board_advance(board_t *b,uint64_t until)
{
    while(b->clock<until){
        if(!(b->clock&1))OPM_Clock(&b->opm,b->output,NULL,NULL,NULL);
        b->oki_phase+=8000;
        if(b->oki_phase>=WF_CLK){b->oki_phase-=WF_CLK;reference_oki_tick(&b->oki);}
        if(b->capturing){
            b->out_phase+=b->output_rate;
            if(b->out_phase>=WF_CLK){
                b->out_phase-=WF_CLK;
                int64_t mixed=((int64_t)b->output[0]*10813+(int64_t)b->output[1]*10813+
                    (int64_t)reference_oki_mix(&b->oki)*21627)>>15;
                if(mixed>32767){mixed=32767;b->clipped++;}if(mixed< -32768){mixed=-32768;b->clipped++;}
                uint64_t magnitude=(uint64_t)(mixed<0?-mixed:mixed);if(magnitude>b->peak)b->peak=magnitude;
                board_pcm_sample(b,mixed);
                b->frames++;
            }
        }
        b->clock++;
    }
}

static uint8_t board_read(void *user,uint16_t address)
{
    board_t *b=user;wf_ctx_t *c=&b->original;board_advance(b,now(c));
    if(address==0xe000&&b->latch_pending){
        b->latch_pending=0;
        fprintf(b->deliveries,"%u,%llu,%llu\n",b->command_index,
            (unsigned long long)(b->latch_delivery-b->origin),(unsigned long long)(now(c)-b->origin));
    }
    if(b->profile!=PROFILE_A53_MMIO){
        if(address==0xc800||address==0xc801){
            uint8_t value=OPM_Read(&b->opm,1);b->ym_reads++;if(value&0x80)b->busy_ym_reads++;
            if(b->profile==PROFILE_MASK_A)value&=(uint8_t)~1u;
            return value;
        }
        if(address==0xd800){
            uint8_t value=reference_oki_status(&b->oki);b->oki_reads++;if(value&15)b->busy_oki_reads++;
            return value;
        }
    }
    return rb(c,address);
}

static void board_write(void *user,uint16_t address,uint8_t value)
{
    board_t *b=user;wf_ctx_t *c=&b->original;board_advance(b,now(c));
    if(address==0xc800)OPM_Write(&b->opm,0,value);
    if(address==0xc801)OPM_Write(&b->opm,1,value);
    if(address==0xd800){
        int pending=b->oki.pending;unsigned busy=reference_oki_status(&b->oki)&15;
        reference_oki_write(&b->oki,value);
        if(pending>=0){
            for(unsigned v=0;v<4;v++)if((value&(0x10u<<v))&&!(busy&(1u<<v))&&b->oki.voices[v].playing)
                loge(c,"PLAY",(b->oki.bank<<8)|(v<<4)|(value&15),(unsigned)pending);
        }else if(!(value&0x80))loge(c,"STOP",value>>3,0);
    }
    if(address==0xe800)reference_oki_bank(&b->oki,value);
    wb(c,address,value);
}

static void board_step(board_t *b)
{
    wf_ctx_t *c=&b->original;
    /* Original C049+voice is a sequence suppression latch, independent of the
     * physical OKI busy bit. Observe the actual JP NZ,0523 before executing it. */
    if(b->capturing&&c->cpu.pc==0x047f&&!c->cpu.zf){
        unsigned voice=c->ram[0x3d];
        if(voice<3)loge(c,"F1SKIP",(voice<<8)|c->ram[0x49+voice],c->cpu.d);
    }
    if(b->profile!=PROFILE_A53_MMIO){
        unsigned status=OPM_Read(&b->opm,1)&3;
        unsigned irq=b->profile==PROFILE_MASK_A?(status&2):OPM_ReadIRQ(&b->opm);
        /* Preserve the physical IRQ level, including deassertion. NMI remains
         * a separate edge associated with the unread original sound latch. */
        c->cpu.int_pending=irq!=0;c->cpu.int_data=0xff;
        if(status&1)b->irq_a++;
        if(status&2)b->irq_b++;
    }
    z80_step(&c->cpu);board_advance(b,now(c));
    if(b->profile==PROFILE_A53_MMIO)update_timer(c);
}

int main(int argc,char **argv)
{
    if(argc!=10&&argc!=11){fprintf(stderr,"usage: %s SOUND.ROM SAMPLES.ROM SCHEDULE.csv SECONDS TRACE.csv PCM16BE REPORT.json DELIVERIES.csv PROFILE(full|mask-a|a53-mmio) [OUTPUT_RATE]\n",argv[0]);return 2;}
    unsigned seconds=(unsigned)strtoul(argv[4],NULL,0);
    unsigned output_rate=argc==11?(unsigned)strtoul(argv[10],NULL,0):44100u;
    if(!seconds||seconds>1800||sizeof(unsigned long)<8||
       output_rate<44100u||output_rate>352800u||output_rate%44100u)return 2;
    if(!board_outputs_are_new(argv))return 8;
    board_command_t commands[4096];unsigned count=0;char line[256];
    FILE *schedule=fopen(argv[3],"r");if(!schedule)return 3;
    while(fgets(line,sizeof line,schedule)){
        unsigned long long sample;unsigned word;
        if(sscanf(line,"%llu,%u",&sample,&word)!=2||count==4096||word>65535||!(word&255)||
           (word&255)>0x81||(count&&sample<=commands[count-1].sample))return 4;
        commands[count++]=(board_command_t){sample,word};
    }
    fclose(schedule);if(!count||commands[0].sample||commands[count-1].sample>=(uint64_t)seconds*44100)return 4;
    board_t *b=calloc(1,sizeof *b);uint8_t *samples=malloc(0x80000);
    if(!b||!samples||!load(argv[1],b->original.rom,sizeof b->original.rom)||!load(argv[2],samples,0x80000))return 5;
    b->output_rate=output_rate;
    if(!strcmp(argv[9],"full"))b->profile=PROFILE_FULL;
    else if(!strcmp(argv[9],"mask-a"))b->profile=PROFILE_MASK_A;
    else if(!strcmp(argv[9],"a53-mmio"))b->profile=PROFILE_A53_MMIO;
    else return 2;
    wf_ctx_t *c=&b->original;c->log=fopen(argv[5],"wx");b->pcm=fopen(argv[6],"wbx");
    FILE *report=fopen(argv[7],"wx");b->deliveries=fopen(argv[8],"wx");
    if(!c->log||!b->pcm||!report||!b->deliveries)return 5;
    fprintf(c->log,"cycle,kind,a,b\n");fprintf(b->deliveries,"index,delivery_relative_cycle,latch_read_relative_cycle\n");
    OPM_Reset(&b->opm,opm_flags_none);reference_oki_reset(&b->oki,samples,0x80000);
    z80_init(&c->cpu);c->cpu.read_byte=board_read;c->cpu.write_byte=board_write;
    c->cpu.port_in=pin;c->cpu.port_out=pout;c->cpu.userdata=b;
    while(now(c)<2*WF_CLK)board_step(b);
    b->origin=now(c);b->capturing=1;loge(c,"ORIGIN",0,0);
    uint64_t end=b->origin+(uint64_t)seconds*WF_CLK;unsigned index=0;
    while(now(c)<end){
        if(index<count){
            uint64_t target=b->origin+(commands[index].sample*WF_CLK+44100-1)/44100;
            if(now(c)>=target){
                if(b->latch_pending||c->cpu.nmi_pending){fprintf(stderr,"Pending latch/NMI at measured command%u\n",index);return 6;}
                b->latch_pending=1;b->command_index=index;b->latch_delivery=now(c);
                c->latch=(uint8_t)commands[index].word;loge(c,"CMD",index,commands[index].word);
                z80_gen_nmi(&c->cpu);index++;
            }
        }
        board_step(b);
    }
    loge(c,"END",0,0);
    int valid=index==count&&!b->latch_pending&&!c->cpu.nmi_pending&&!b->oki.fault;
    fprintf(report,"{\"profile\":\"%s\",\"complete\":%s,\"commands\":%u,\"output_rate\":%u,\"origin_cycle\":%llu,\"end_cycle\":%llu,\"frames\":%llu,\"peak\":%llu,\"clipped_frames\":%llu,\"ym_status_reads\":%llu,\"ym_busy_reads\":%llu,\"oki_status_reads\":%llu,\"oki_busy_reads\":%llu,\"instruction_checkpoints_with_timer_a_flag\":%llu,\"instruction_checkpoints_with_timer_b_flag\":%llu,\"bank_changes_while_pcm_active\":%llu,\"ignored_busy_starts\":%llu,\"invalid_sample_starts\":%llu}\n",
        argv[9],valid?"true":"false",count,b->output_rate,(unsigned long long)b->origin,(unsigned long long)now(c),
        (unsigned long long)b->frames,(unsigned long long)b->peak,(unsigned long long)b->clipped,
        (unsigned long long)b->ym_reads,(unsigned long long)b->busy_ym_reads,
        (unsigned long long)b->oki_reads,(unsigned long long)b->busy_oki_reads,
        (unsigned long long)b->irq_a,(unsigned long long)b->irq_b,
        (unsigned long long)b->oki.bank_changes_while_active,(unsigned long long)b->oki.ignored_busy_starts,
        (unsigned long long)b->oki.invalid_starts);
    board_pcm_flush(b);
    int close_failed=fclose(c->log)!=0;
    close_failed|=fclose(b->pcm)!=0;close_failed|=fclose(report)!=0;close_failed|=fclose(b->deliveries)!=0;
    if(close_failed){fprintf(stderr,"Reference output close failure\n");free(samples);free(b);return 10;}
    free(samples);free(b);return valid?0:7;
}

