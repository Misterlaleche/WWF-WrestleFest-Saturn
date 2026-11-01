#include "wf_native_queue.h"
#include <string.h>
bool wf_native_queue_init(wf_native_queue_t *q,wf_native_host_t *h)
{
    if(!q||!h||!h->mail||!h->ready||h->poisoned||
       h->mail->abi!=WF_NATIVE_ABI||!h->ops.enter_owner||
       !h->ops.leave_owner||!h->ops.barrier)return false;
    memset(q,0,sizeof *q);q->host=h;return true;
}
bool wf_native_queue_push(wf_native_queue_t *q,uint16_t raw)
{
    if(!q||!q->host||q->fault||q->host->poisoned)return false;
    if((raw&255u)>0x81u){q->invalid++;return false;}
    if(q->used==64u){q->overflows++;return false;}
    q->words[q->tail]=raw;q->tail=(q->tail+1u)%64u;q->used++;q->submitted++;
    return true;
}
bool wf_native_queue_service(wf_native_queue_t *q,uint32_t now)
{
    wf_native_host_t *h;
    bool ok=true;
    if(!q||!q->host||q->fault)return false;
    h=q->host;
    if(h->poisoned||!h->ready)return false;
    if(!h->ops.enter_owner(h->ops.user))return true;
    if(h->mail->state!=WF_NATIVE_READY||h->mail->abi!=WF_NATIVE_ABI)goto fault;
    for(unsigned budget=0;budget<4u;budget++){
        if(q->pending){
            if(h->mail->ack!=q->sequence){
                if((uint32_t)(now-q->sent_at)>=100000u)goto fault;
                break;
            }
            h->ops.barrier(h->ops.user);
            if(h->mail->result==WF_NATIVE_UNIMPLEMENTED){
                q->unimplemented++;q->last_unimplemented=q->words[q->head];
            }else if(h->mail->result==WF_NATIVE_PRIORITY_REJECTED){
                q->priority_rejected++;
            }else if(h->mail->result!=WF_NATIVE_OK)goto fault;
            q->pending=false;q->head=(q->head+1u)%64u;q->used--;q->completed++;
        }
        if(!q->used)break;
        if(h->mail->request!=h->mail->ack)goto fault;
        q->sequence=(uint16_t)(h->mail->request+1u);
        h->mail->command=WF_NATIVE_ARCADE_WORD;
        h->mail->argument=q->words[q->head];
        h->ops.barrier(h->ops.user);
        h->mail->request=q->sequence;
        h->ops.barrier(h->ops.user);
        q->sent_at=now;q->pending=true;
    }
    goto done;
fault:
    q->fault=true;h->poisoned=true;ok=false;
done:
    h->ops.leave_owner(h->ops.user);return ok;
}
