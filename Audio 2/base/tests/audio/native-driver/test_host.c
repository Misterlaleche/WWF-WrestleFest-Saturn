#include "wf_native_wp24.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    wf_native_mail_t mail;
    unsigned locks, unlocks, off, on, writes, fences, render_calls,dsp_disables;
    uint32_t clock;
    bool locked, respond, boot_respond, muted;
    uint8_t ram[0x80000];
} fixture_t;
static fixture_t f;
static wf24_music_stream_t stream;

static bool enable(bool on,void *u)
{
    fixture_t *t=u;
    assert(t->locked);
    if(on) {
        t->on++;
        assert(t->off==t->on);
        if(t->boot_respond) {
            t->mail.magic_hi=0x5746; t->mail.magic_lo=0x4e31;
            t->mail.abi=WF_NATIVE_ABI; t->mail.state=2;
        }
    } else t->off++;
    return true;
}
static void write16(uint32_t a,uint16_t v,void *u)
{
    fixture_t *t=u;
    t->writes++;
    assert(!(a&1));
    if(a==0x100400) {
        assert(v==0x0200||v==0x020f);
        if(v==0x020f)assert(t->mail.state==WF_NATIVE_READY);
        t->muted=v==0x0200;return;
    }
    if(a>=0x100804&&a<=0x100bfc&&((a-0x100804)%8)==0){assert(!v&&t->muted);t->dsp_disables++;return;}
    if(t->on<t->off)assert(t->muted);
    assert(a<0x80000);
    assert(!(a>=0x14000 && a<0x60000));
    t->ram[a]=(uint8_t)(v>>8); t->ram[a+1]=(uint8_t)v;
    if(a>=0xc040 && a<0xc060)
        ((volatile uint16_t *)&t->mail)[(a-0xc040)/2]=v;
}
static uint32_t now(void *u) { return ((fixture_t *)u)->clock; }
static void wait_us(uint32_t us,void *u) { ((fixture_t *)u)->clock+=us; }
static bool lock(void *u)
{
    fixture_t *t=u;
    if(t->locked) return false;
    t->locked=true; t->locks++;
    return true;
}
static void unlock(void *u)
{
    fixture_t *t=u;
    assert(t->locked); t->locked=false; t->unlocks++;
}
static void fence(void *u) { ((fixture_t *)u)->fences++; }
static void idle(void *u)
{
    fixture_t *t=u;
    t->clock+=1000;
    if(t->respond && t->mail.request!=t->mail.ack) {
        assert(t->locked);
        t->mail.value=5;
        t->mail.result=t->mail.command>6 ? 1 : 0;
        t->mail.ack=t->mail.request;
    }
}
static bool render(void *dst,uint32_t n,void *u)
{
    fixture_t *t=u;
    memset(dst,0x12,n); t->render_calls++;
    return true;
}
int main(void)
{
    wf_native_host_t h;
    wf_native_wp24_t bridge;
    wf24_stream_ops_t source={0},ops;
    wf_native_segment_t bad[3];
    wf_native_image_t image={bad,3};
    uint16_t value,seq;
    unsigned writes,ca;
    wf_native_platform_t p={enable,write16,now,wait_us,idle,lock,unlock,fence,&f};
    memset(&f,0,sizeof(f)); f.respond=true; f.boot_respond=true;
    assert(wf_native_host_init(&h,&f.mail,&p));
    assert(wf_native_image_valid(&wf_native_driver_image));
    memcpy(bad,wf_native_driver_image.segments,sizeof(bad));
    bad[1].address=0x14000;
    assert(!wf_native_host_boot(&h,&image)); assert(f.off==0);
    bad[1]=bad[0]; assert(!wf_native_image_valid(&image));
    bad[1]=wf_native_driver_image.segments[1];
    bad[1].memory_bytes=0xffffffffu; assert(!wf_native_image_valid(&image));
    assert(wf_native_host_boot(&h,&wf_native_driver_image));
    assert(f.off==1 && f.on==1 && h.ready && !h.poisoned);
    assert(f.dsp_disables==128);
    assert(f.clock>=40);
    assert(!memcmp(f.ram,wf_native_driver_image.segments[0].data_be,1024));
    assert(wf_native_host_command(&h,1,0,&value) && value==5);
    f.mail.request=f.mail.ack=0xffff;
    assert(wf_native_host_command(&h,2,0,NULL)); assert(f.mail.ack==0);
    assert(!wf_native_host_command(&h,0xdead,0,NULL)); assert(!h.poisoned);
    f.locked=true; seq=f.mail.request;
    assert(!wf_native_host_command(&h,1,0,NULL)); assert(f.mail.request==seq);
    f.locked=false;
    source.render_live_be=render; source.user=&f;
    assert(wf_native_wp24_bind(&bridge,&h,&source,&ops));
    assert(wf24_stream_init(&stream,&ops));
    assert(wf24_stream_start_live(&stream,1));
    assert(f.render_calls==2);
    assert(f.ram[0x60000]==0x12 && f.ram[0x6ffff]==0x12);
    assert(ops.slot31_read_ca_serialized(&ca,ops.user) && ca==5);
    assert(wf24_stream_stop(&stream));
    writes=f.writes;
    assert(!ops.copy_half_be(2,"bad",3,ops.user)); assert(f.writes==writes);
    /* Deadline crosses uint32 wrap; no resubmission after unknown outcome. */
    f.clock=0xfffffff0u; f.respond=false; h.timeout_us=2500;
    assert(!wf_native_host_command(&h,1,0,NULL)); assert(h.poisoned);
    seq=f.mail.request;
    f.respond=true; f.mail.ack=seq;
    assert(!wf_native_host_command(&h,2,0,NULL)); assert(f.mail.request==seq);
    assert(wf_native_host_boot(&h,&wf_native_driver_image)); assert(!h.poisoned);
    wf_native_host_pcm_tick(&h); assert(f.ram[0xc00d]==1);
    f.boot_respond=false;
    assert(!wf_native_host_boot(&h,&wf_native_driver_image));
    assert(!h.ready && h.poisoned && !f.locked && f.muted);
    assert(f.locks==f.unlocks);
    puts("native host: boot, validation, owner, deadlines and unchanged WP24 stream PASS");
    return 0;
}
