/* Real Saturn lifecycle stress, using only the public API. The98-command
 * diagnostic is not a gameplay producer or a waveform-fidelity comparison. */
#define main audio2_standard_harness_unused_main
#include "saturn_harness.c"
#undef main

/* Counts before cold recovery remain here after the driver resets itself.
 * 0:magic 1:phase 2:SFXaccepted 3:BUSY 4:quiescentstops 5:recoveries
 * 6:restartedcommand 7:diagnosticfailure 8:priorAPIerror 9:priorFMdiscard
 * 10:priorunderrun 11:priorlate 12:priorpriorityreject 13:prioraccepted
 * 14:postFMdiscard 15:postunderrun 16:postlate 17:currentAPIstate
 * 18:currentAPIerror 19:workerquiescent 20:lastvideo 21:postpriorityreject */
volatile uint32_t audio2_lifecycle[24] __attribute__((section(".uncached.audio2_lifecycle")));

static void remember(unsigned base)
{
    audio2_lifecycle[base]=read32(&wf_music_fm_discarded_events);
    audio2_lifecycle[base+1]=*(volatile uint32_t *)(uintptr_t)(0x25a00000u+AUDIO2_SFX_ENGINE_ADDR+32u);
    audio2_lifecycle[base+2]=*(volatile uint16_t *)0x25a0b028;
}
static void diagnostic_fail(unsigned reason)
{
    if(!audio2_lifecycle[7])audio2_lifecycle[7]=reason;
    audio2_diag[1]=3;
}

int main(void)
{
    audio2_diag[0]=0x57464132u;audio2_diag[1]=1;
    audio2_lifecycle[0]=0x4c494631u;
    bool ok=boot_audio();audio2_diag[1]=ok?2:3;
    if(!ok)diagnostic_fail(1);
    unsigned phase=0,next=0x20;
    uint32_t boundary=video_frames;
    for(;;){
        wf_audio2_service(&audio);
        uint32_t frame=video_frames;
        audio2_lifecycle[17]=audio.status.state;
        audio2_lifecycle[18]=audio.status.error;
        audio2_lifecycle[19]=wf_audio2_yaul_worker_quiescent(NULL);
        audio2_lifecycle[20]=frame;
        audio2_lifecycle[22]=music->last_command;
        audio2_lifecycle[23]=music->state;
        if(ok&&audio.status.state==WF_AUDIO2_FAULT){
            audio2_lifecycle[8]=audio.status.error;
            remember(9);diagnostic_fail(2);ok=false;
        }
        if(ok&&phase==0&&frame-boundary>=60u){
            wf_audio2_result_t r=wf_audio2_command(&audio,0x01);
            if(r==WF_AUDIO2_ACCEPTED)phase=1;
            else if(r!=WF_AUDIO2_BUSY){diagnostic_fail(3);ok=false;}
        }
        if(ok&&phase==1){
            while(next<=0x81u){
                wf_audio2_result_t r=wf_audio2_command(&audio,(uint16_t)next);
                if(r==WF_AUDIO2_BUSY){++audio2_lifecycle[3];break;}
                if(r!=WF_AUDIO2_ACCEPTED){diagnostic_fail(4);ok=false;break;}
                ++next;++audio2_lifecycle[2];
            }
            if(next==0x82u){phase=2;boundary=frame;}
        }
        if(ok&&phase==2&&frame-boundary>=120u){
            audio2_lifecycle[8]=audio.status.error;remember(9);
            audio2_lifecycle[12]=audio.status.priority_rejections;
            audio2_lifecycle[13]=audio.status.accepted_commands;
            wf_audio2_stop(&audio);phase=3;boundary=frame;
        }
        if(ok&&(phase==3||phase==6)&&audio.status.state==WF_AUDIO2_STOPPED){
            if(!wf_audio2_yaul_worker_quiescent(NULL)){diagnostic_fail(5);ok=false;}
            else{
                ++audio2_lifecycle[4];
                if(phase==3){
                    if(!wf_audio2_recover(&audio)){diagnostic_fail(6);ok=false;}
                    else{++audio2_lifecycle[5];phase=4;boundary=video_frames;}
                }else{phase=7;audio2_diag[1]=4;}
            }
        }
        if(ok&&phase==4&&audio.status.state==WF_AUDIO2_RUNNING){
            wf_audio2_result_t r=wf_audio2_command(&audio,0x06);
            if(r==WF_AUDIO2_ACCEPTED){audio2_lifecycle[6]=0x06;phase=5;boundary=frame;}
            else if(r!=WF_AUDIO2_BUSY){diagnostic_fail(7);ok=false;}
        }
        if(ok&&phase==5&&frame-boundary>=180u){
            remember(14);audio2_lifecycle[21]=audio.status.priority_rejections;
            wf_audio2_stop(&audio);phase=6;boundary=frame;
        }
        /* recover() performs a cold boot and can advance VBlank. Use a
         * fresh clock here; the pre-boot local frame would underflow. */
        if(ok&&(phase==3||phase==4||phase==6)&&video_frames-boundary>600u){
            diagnostic_fail(8);ok=false;
        }
        audio2_lifecycle[1]=phase;
        audio2_diag[2]=frame;audio2_diag[19]=audio.status.state;audio2_diag[20]=audio.status.error;
        if((frame%30u)==0){
            dbgio_printf("\x1b[H\x1b[2JAUDIO2 LIFECYCLE TEST\nPHASE %u  SFX %lu/98\nBUSY %lu  STOPS %lu\nRECOVERIES %lu  ERROR %lu\nHARDWARE PENDING\n",phase,
                (unsigned long)audio2_lifecycle[2],(unsigned long)audio2_lifecycle[3],
                (unsigned long)audio2_lifecycle[4],(unsigned long)audio2_lifecycle[5],
                (unsigned long)audio2_lifecycle[7]);dbgio_flush();
        }
        vdp2_sync();vdp2_sync_wait();
    }
}
