#include "wp01_audio_queue.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t log_values[32],log_count;
static void record(void *context,uint16_t offset,uint16_t value){
    assert(context==log_values);assert(log_count<32);
    log_values[log_count++]=((uint32_t)offset<<16)|value;
}
static void be16(uint8_t *p,uint32_t n){p[0]=n>>8;p[1]=n;}
static void be32(uint8_t *p,uint32_t n){be16(p,n>>16);be16(p+2,n);}
static void event(uint8_t *p,uint32_t tick,uint16_t reg,uint16_t value){be32(p,tick);be16(p+4,reg);be16(p+6,value);}
int main(void){
    uint8_t p[60]={0};wp01_audio_queue_t q;
    be32(p,0x5750314e);be32(p+4,1);be32(p+8,1);be32(p+12,3);
    be32(p+16,90000);be32(p+20,100);be32(p+24,120093750);be32(p+28,143);
    be16(p+32,0x16);be16(p+34,0);
    event(p+36,10,0x10,0x1234);event(p+44,10,0,0x1800);event(p+52,70000,0,0x1000);
    assert(wp01_audio_queue_bind(&q,p,sizeof(p),record,log_values));
    wp01_audio_queue_restart(&q);assert(log_count==1&&log_values[0]==0x00160000);
    assert(wp01_audio_queue_advance(&q,9)==0&&log_count==1);
    assert(wp01_audio_queue_advance(&q,10)==2&&log_count==3);
    assert(log_values[1]==0x00101234&&log_values[2]==0x00001800);
    assert(wp01_audio_queue_advance(&q,65535)==0);
    assert(wp01_audio_queue_advance(&q,70037)==1&&log_values[3]==0x00001000);
    assert(q.max_lateness==37&&q.max_burst==2);
    assert(wp01_audio_queue_advance(&q,90000)==0);
    log_count=0;wp01_audio_queue_restart(&q);
    assert(q.next==0&&q.elapsed==0&&q.max_lateness==0&&log_count==1);
    assert(wp01_audio_queue_advance(&q,10)==2);
    assert(!wp01_audio_queue_bind(&q,p,sizeof(p)-1,record,log_values));
    be16(p+48,0x401);assert(!wp01_audio_queue_bind(&q,p,sizeof(p),record,log_values));
    be16(p+48,0);be32(p+52,8);assert(!wp01_audio_queue_bind(&q,p,sizeof(p),record,log_values));
    puts("PASS: native register ordering, due times, repeat, long timeline, latency and bounds");
}
