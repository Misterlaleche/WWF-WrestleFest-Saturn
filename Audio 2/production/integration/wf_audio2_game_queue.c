#include "wf_audio2_game_queue.h"
bool wf_audio2_game_queue_push(wf_audio2_game_queue_t *q,uint16_t raw)
{
    if(!q||q->fault)return false;
    if((raw&255u)>0x81u){q->invalid++;return false;}
    if(q->used==64u){q->overflows++;return false;}
    q->words[q->tail]=raw;q->tail=(q->tail+1u)&63u;
    q->used++;q->submitted++;return true;
}
bool wf_audio2_game_queue_service(wf_audio2_game_queue_t *q,wf_audio2_t *a)
{
    if(!q||!a||q->fault)return false;
    wf_audio2_service(a);
    if(a->status.state==WF_AUDIO2_FAULT){q->fault=true;return false;}
    for(unsigned budget=0;budget<4u&&q->used;budget++){
        wf_audio2_result_t result=wf_audio2_command(a,q->words[q->head]);
        if(result==WF_AUDIO2_BUSY||result==WF_AUDIO2_NOT_READY)break;
        if(result==WF_AUDIO2_UNSUPPORTED){
            q->unsupported++;q->last_unimplemented=q->words[q->head];
        }else if(result!=WF_AUDIO2_ACCEPTED){q->fault=true;return false;}
        q->head=(q->head+1u)&63u;q->used--;q->completed++;
    }
    return true;
}
