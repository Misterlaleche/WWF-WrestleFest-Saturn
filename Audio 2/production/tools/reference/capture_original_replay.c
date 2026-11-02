/* Host-only full original Z80 replay. Never linked into Saturn firmware.
 * The supplied schedule contains measured, strictly increasing API admissions.
 * Refuse an overlapping unread latch/NMI instead of inventing input spacing.
 */
#define main historical_capture_main
#include "oracle_a53/tools/capture_full_audio_a50.c"
#undef main

typedef struct {
    wf_ctx_t original;
    int latch_pending;
    unsigned command_index;
    uint64_t origin,delivery_cycle;
    FILE *deliveries;
} replay_t;
typedef struct {uint64_t sample;unsigned word;} command_t;

static uint8_t replay_read(void *user,uint16_t address)
{
    replay_t *r=user;
    if(address==0xe000&&r->latch_pending){
        r->latch_pending=0;
        fprintf(r->deliveries,"%u,%llu,%llu\n",r->command_index,
                (unsigned long long)(r->delivery_cycle-r->origin),
                (unsigned long long)(now(&r->original)-r->origin));
    }
    return rb(&r->original,address);
}

int main(int argc,char **argv)
{
    if(argc!=6){fprintf(stderr,"usage: %s SOUND.ROM SCHEDULE.csv SECONDS TRACE.csv DELIVERIES.csv\n",argv[0]);return 2;}
    unsigned seconds=(unsigned)strtoul(argv[3],NULL,0);
    if(!seconds||seconds>1800||sizeof(unsigned long)<8)return 2;
    command_t commands[4096];unsigned count=0;char line[256];
    FILE *schedule=fopen(argv[2],"r");if(!schedule)return 3;
    while(fgets(line,sizeof line,schedule)){
        unsigned long long sample;unsigned word;
        if(sscanf(line,"%llu,%u",&sample,&word)!=2||count==4096||word>65535||
           !(word&255)||(word&255)>0x81||(count&&sample<=commands[count-1].sample))return 4;
        commands[count++]=(command_t){sample,word};
    }
    fclose(schedule);
    if(!count||commands[0].sample||commands[count-1].sample>=(uint64_t)seconds*44100u)return 4;
    replay_t *r=calloc(1,sizeof *r);if(!r||!load(argv[1],r->original.rom,sizeof r->original.rom))return 5;
    wf_ctx_t *c=&r->original;c->log=fopen(argv[4],"w");r->deliveries=fopen(argv[5],"w");
    if(!c->log||!r->deliveries)return 5;
    fprintf(c->log,"cycle,kind,a,b\n");fprintf(r->deliveries,"index,delivery_relative_cycle,latch_read_relative_cycle\n");
    z80_init(&c->cpu);c->cpu.read_byte=replay_read;c->cpu.write_byte=wb;
    c->cpu.port_in=pin;c->cpu.port_out=pout;c->cpu.userdata=r;
    while(now(c)<2*WF_CLK){z80_step(&c->cpu);update_timer(c);}
    r->origin=now(c);loge(c,"ORIGIN",0,0);
    uint64_t end=r->origin+(uint64_t)seconds*WF_CLK;unsigned index=0;
    while(now(c)<end){
        if(index<count){
            uint64_t target=r->origin+(commands[index].sample*WF_CLK+44100u-1u)/44100u;
            if(now(c)>=target){
                if(r->latch_pending||c->cpu.nmi_pending){
                    fprintf(stderr,"Unread latch/NMI at measured command%u; no invented spacing\n",index);return 6;
                }
                r->latch_pending=1;r->command_index=index;r->delivery_cycle=now(c);
                c->latch=(uint8_t)commands[index].word;
                loge(c,"CMD",index,commands[index].word);z80_gen_nmi(&c->cpu);index++;
            }
        }
        z80_step(&c->cpu);update_timer(c);
    }
    loge(c,"END",0,0);
    if(index!=count||r->latch_pending||c->cpu.nmi_pending)return 7;
    fclose(c->log);fclose(r->deliveries);free(r);
    fprintf(stderr,"PASS original replay: %u measured admissions; no overlapping unread latch/NMI\n",count);
    return 0;
}
