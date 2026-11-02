/* Host diagnostic only: inspect command01/channel1 M1 without modifying it.
 * Uses the preserved original trace parser and externally supplied pinned OPM.
 * No ROM tables, sample data or renderer dependency is linked into Saturn.
 */
#define main original_render_main
#include "oracle_a53/tools/render_combined_a50.c"
#undef main

int main(int argc,char **argv)
{
    event_t *events=0;size_t count=0,index=0;uint64_t origin,end;
    opm_t chip;int32_t audio[2];
    unsigned phase=0,pg=0,atten=0,fb=0;int mod=0,old0=0,old1=0;
    if(argc!=2 || !load_events(argv[1],&events,&count,&origin,&end))return 2;
    if(end>origin+MASTER_HZ)end=origin+MASTER_HZ;
    OPM_Reset(&chip,opm_flags_none);
    puts("cycle,origin,phase20,phase10,modulation,attenuation,feedback,previous0,previous1,output");
    for(uint64_t cycle=0;cycle<=end;cycle++){
        while(index<count && events[index].cycle==cycle){
            event_t *e=&events[index++];
            if(!strcmp(e->kind,"YM_A"))OPM_Write(&chip,0,e->b);
            else if(!strcmp(e->kind,"YM_D"))OPM_Write(&chip,1,e->b);
        }
        if(cycle&1u)continue;
        unsigned step=chip.cycles;
        if(step==1){pg=chip.pg_phase[1];old0=chip.op_m1[1][0];old1=chip.op_m1[1][1];fb=chip.ch_fb[1];}
        if(step==6)atten=chip.eg_out[1];
        OPM_Clock(&chip,audio,0,0,0);
        if(step==1){phase=chip.op_phase_in;mod=chip.op_mod_in;}
        if(step==10 && cycle>=origin-357954){
            printf("%llu,%llu,%u,%u,%d,%u,%u,%d,%d,%d\n",
                (unsigned long long)cycle,(unsigned long long)origin,pg,phase,mod,atten,fb,old0,old1,chip.op_out[1]);
        }
    }
    free(events);return 0;
}
