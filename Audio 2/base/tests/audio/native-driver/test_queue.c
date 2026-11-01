#include "wf_native_queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static wf_native_mail_t mail;
static unsigned locks,unlocks,fences;
static bool held;
static bool lock(void *u){(void)u;if(held)return false;held=true;locks++;return true;}
static void unlock(void *u){(void)u;assert(held);held=false;unlocks++;}
static void barrier(void *u){(void)u;fences++;}
int main(void)
{
    wf_native_host_t h={0};
    wf_native_queue_t q;
    memset(&mail,0,sizeof mail);
    mail.magic_hi=0x5746;mail.magic_lo=0x4e31;mail.abi=WF_NATIVE_ABI;mail.state=WF_NATIVE_READY;
    h.mail=&mail;h.ready=true;h.ops.enter_owner=lock;h.ops.leave_owner=unlock;h.ops.barrier=barrier;
    assert(wf_native_queue_init(&q,&h));
    for(unsigned i=0;i<64;i++)assert(wf_native_queue_push(&q,(uint16_t)(0x3100u+i)));
    assert(!wf_native_queue_push(&q,0x3181));assert(q.overflows==1 && q.used==64);
    assert(wf_native_queue_service(&q,0));assert(mail.command==WF_NATIVE_ARCADE_WORD);
    assert(mail.argument==0x3100 && q.used==64 && q.pending);
    uint16_t seq=mail.request;
    assert(wf_native_queue_service(&q,10));assert(mail.request==seq);
    for(unsigned i=0;i<64;i++){
        assert(mail.argument==(uint16_t)(0x3100u+i));
        mail.result=i?WF_NATIVE_UNIMPLEMENTED:WF_NATIVE_OK;
        mail.ack=mail.request;
        assert(wf_native_queue_service(&q,20+i));
    }
    assert(!q.pending && !q.used && q.completed==64 && q.unimplemented==63);
    assert(q.last_unimplemented==0x313f);
    assert(wf_native_queue_push(&q,0x3121));assert(wf_native_queue_service(&q,100));
    mail.result=4;mail.ack=mail.request;
    assert(wf_native_queue_service(&q,110));assert(!q.fault&&!q.used);
    assert(!wf_native_queue_push(&q,0x3182));assert(q.invalid==1);
    assert(wf_native_queue_push(&q,0x3181));
    held=true;assert(wf_native_queue_service(&q,0xfffffff0u));assert(!q.pending);held=false;
    assert(wf_native_queue_service(&q,0xfffffff0u));assert(q.pending);
    assert(!wf_native_queue_service(&q,100001u));assert(h.poisoned && q.fault);
    assert(!wf_native_queue_push(&q,0));seq=mail.request;mail.ack=seq;
    assert(!wf_native_queue_service(&q,100002));assert(mail.request==seq);
    h.poisoned=false;mail.request=2;mail.ack=1;
    assert(wf_native_queue_init(&q,&h));
    assert(wf_native_queue_push(&q,0x3102));
    assert(!wf_native_queue_service(&q,0));
    assert(mail.request==2 && mail.ack==1 && q.fault && h.poisoned);
    assert(locks==unlocks);
    assert(fences>=128);
    puts("queue: raw words, FIFO, overflow, owner, unknown outcome and wrap PASS");
    return 0;
}
