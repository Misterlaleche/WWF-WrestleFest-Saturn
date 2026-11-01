/* Catches lost same-frame producers, wrong raw words, BUSY consumption,
 * queue overflow, and invented audio for unresolved commands. Uses the real
 * public Audio2 command/service implementation with an in-memory mailbox. */
#include "wf_audio2_game_queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t ticks;
static uint32_t now(void *u){(void)u;return ticks;}
static void barrier(void *u){(void)u;}
int main(void)
{
    wf_audio2_t a={0};wf_native_mail_t m={0};wf_music_module_shared_t s={0};
    wf_audio2_game_queue_t q={0};
    a.status.state=WF_AUDIO2_RUNNING;a.config.mail=&m;a.config.music=&s;
    a.config.platform.now_us=now;a.config.platform.barrier=barrier;
    a.host.timeout_us=100000u;m.state=WF_NATIVE_READY;s.state=WF_MUSIC_RUNNING;
    if(!wf_audio2_game_queue_push(&q,0x312fu)){
        puts("RED: existing game audio callback could not retain its original word");return 1;
    }
    assert(wf_audio2_game_queue_push(&q,0x0120u));
    assert(q.used==2&&q.submitted==2&&!m.request);
    assert(wf_audio2_game_queue_service(&q,&a));
    assert(m.request==1&&m.argument==0x312fu&&q.used==1);
    assert(wf_audio2_game_queue_service(&q,&a));
    assert(m.request==1&&q.used==1&&q.completed==1);
    m.ack=m.request;m.result=WF_NATIVE_OK;
    assert(wf_audio2_game_queue_service(&q,&a));
    assert(m.request==2&&m.argument==0x0120u&&!q.used&&q.completed==2);
    m.ack=m.request;m.result=WF_NATIVE_OK;
    assert(wf_audio2_game_queue_push(&q,0x0013u));
    assert(wf_audio2_game_queue_push(&q,0x0004u));
    assert(wf_audio2_game_queue_service(&q,&a));
    assert(q.unsupported==1&&q.last_unimplemented==0x0013u);
    assert(m.request==2&&s.producer==1&&s.commands[0]==4&&!q.used);
    for(unsigned i=0;i<64;i++)assert(wf_audio2_game_queue_push(&q,0x0020u));
    assert(!wf_audio2_game_queue_push(&q,0x0021u)&&q.used==64&&q.overflows==1);
    assert(!wf_audio2_game_queue_push(&q,0x0082u)&&q.invalid==1);
    puts("PASS exact original words, ordered BUSY retry, fail-closed unknown command, bounded 64 entries");
    return 0;
}
