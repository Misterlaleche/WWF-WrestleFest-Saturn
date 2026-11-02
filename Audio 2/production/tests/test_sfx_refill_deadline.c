/* Refill scheduling uses the measured four-voice starvation state. Decoder
 * contents are checked separately from the choice of voice; no hardware clock
 * or sample-buffer capacity is enlarged by this fixture. */
#include "wf_scsp_sfx.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t pack[4096];
static uint16_t output[4][4097];
static unsigned writes[4];
static unsigned timed_quarters;
static bool timed_writes;

static bool write_sample(uint32_t address,uint16_t value,void *user)
{
    (void)user;
    assert(address>=WF_SFX_PCM_BASE);
    unsigned offset=address-WF_SFX_PCM_BASE,voice=offset/8194u;
    assert(voice<4 && !(offset&1u));
    output[voice][(offset%8194u)/2u]=value;++writes[voice];
    if(timed_writes)timed_quarters+=3;
    return true;
}
static bool control(unsigned voice,bool playing,unsigned volume,void *user)
{
    (void)voice;(void)playing;(void)volume;(void)user;return true;
}
static void fixture(wf_scsp_sfx_t *s,const unsigned ahead[4],unsigned cursor)
{
    memset(writes,0,sizeof writes);memset(output,0x5a,sizeof output);
    for(unsigned i=0;i<sizeof pack;i++)pack[i]=(uint8_t)(i*37u+(i>>1));
    wf_scsp_sfx_init(s,write_sample,NULL);
    assert(wf_scsp_sfx_configure_pcm(s,8194,1,control));
    s->bound=1;s->pack=pack;s->work_voice=(uint8_t)cursor;
    for(unsigned v=0;v<4;v++){
        wf_scsp_sfx_pcm_t *p=&s->pcm[v];
        p->playing=1;p->initial_fill=1;p->write_at=2048;
        p->played=2048-ahead[v];p->write_end=4096;p->total_frames=8192;
        p->decoder.offset=64;p->decoder.remaining=8192;
    }
}
static void verify_next_samples_budget(wf_scsp_sfx_t *s,unsigned selected,unsigned count,unsigned budget)
{
    wf_scsp_sfx_pcm_t before[4];memcpy(before,s->pcm,sizeof before);
    unsigned old_writes[4];memcpy(old_writes,writes,sizeof old_writes);
    wf_sfx_decoder_t expected=before[selected].decoder;
    int16_t samples[128];assert(count<=128);
    for(unsigned i=0;i<count;i++)samples[i]=wf_scsp_sfx_decode(&expected,pack);
    wf_scsp_sfx_work_budget(s,budget);
    for(unsigned v=0;v<4;v++){
        if(v!=selected){
            assert(!memcmp(&s->pcm[v],&before[v],sizeof before[v]));
            assert(writes[v]==old_writes[v]);continue;
        }
        const wf_scsp_sfx_pcm_t *p=&s->pcm[v];
        assert(p->write_at==before[v].write_at+count);
        for(unsigned i=0;i<count;i++)
            assert(output[v][(before[v].write_at+i)&4095u]==(uint16_t)samples[i]);
        assert(p->decoder.offset==expected.offset && p->decoder.remaining==expected.remaining);
        assert(p->decoder.signal==expected.signal && p->decoder.index==expected.index);
        assert(p->decoder.low==expected.low && writes[v]-old_writes[v]==count);
    }
}
static void verify_next_samples(wf_scsp_sfx_t *s,unsigned selected,unsigned count)
{
    verify_next_samples_budget(s,selected,count,128);
}
static void variable_budget_preserves_samples_and_decoder(void)
{
    wf_scsp_sfx_t s;const unsigned ahead[]={140,1062,43,222};
    const unsigned budgets[]={0,1,17,32,64,127,128,129,1000};
    for(unsigned i=0;i<sizeof budgets/sizeof budgets[0];i++){
        fixture(&s,ahead,1);
        verify_next_samples_budget(&s,2,budgets[i]>128?128:budgets[i],budgets[i]);
        assert(s.work_voice==(budgets[i]?3:1));
    }
}
static void closest_deadline_prevents_measured_starvation(void)
{
    wf_scsp_sfx_t s;
    const unsigned ahead[]={140,1062,43,222};
    fixture(&s,ahead,1);
    wf_scsp_sfx_work(&s);
    /* 200Fs consume36 PCM samples, leaving7 in the previously starved voice.
     * The existing8-sample guard must stay active and must not trip. */
    wf_scsp_sfx_advance(&s,200);
    assert(!s.underruns && s.pcm[2].playing &&
           "round-robin refilled1062 samples ahead while the43-sample voice starved");
    assert(writes[2]==128 && !writes[0] && !writes[1] && !writes[3]);

    fixture(&s,ahead,1);
    s.pcm[2].write_at=65560;s.pcm[2].played=65517;
    s.pcm[2].write_end=69536;s.pcm[2].total_frames=81920;
    verify_next_samples(&s,2,128);
}
static void equal_margins_keep_round_robin_order(void)
{
    wf_scsp_sfx_t s;const unsigned ahead[]={256,256,256,256};
    fixture(&s,ahead,2);
    for(unsigned i=0;i<8;i++)verify_next_samples(&s,(2+i)&3u,128);
    /* Equal distance to the original guard, despite differing raw headroom. */
    fixture(&s,ahead,1);
    s.pcm[0].played=s.pcm[0].write_at-100;s.pcm[0].initial_fill=1;
    s.pcm[1].played=s.pcm[1].write_at-156;s.pcm[1].initial_fill=0;
    verify_next_samples(&s,1,128);
}
static void finished_sample_padding_does_not_steal_refills(void)
{
    wf_scsp_sfx_t s;const unsigned ahead[]={12,1062,43,222};
    fixture(&s,ahead,0);
    wf_scsp_sfx_pcm_t *p=&s.pcm[0];
    p->total_frames=2040;p->decoder.remaining=0;
    /* The entire sound and its existing8-sample safety tail are written.
     * More silence before write_end4096 has no playback deadline. */
    verify_next_samples(&s,2,128);
    wf_scsp_sfx_advance(&s,23);
    assert(!s.underruns && !s.pcm[0].playing && s.pcm[2].playing);

    fixture(&s,ahead,0);p=&s.pcm[0];
    p->total_frames=2044;p->decoder.remaining=0;
    /* Only four padding samples remain necessary for the unchanged guard. */
    verify_next_samples(&s,0,4);
    verify_next_samples(&s,2,128);
    for(unsigned i=2048;i<2052;i++)assert(output[0][i]==0);
}
static uint32_t measured_clock(void *user)
{
    (void)user;return (timed_quarters+3u)/4u;
}
static void initial_sfx_decode_has_bounded_work_and_refill_runway(void)
{
    wf_scsp_sfx_t s;const unsigned ahead[]={140,1062,43,222};
    fixture(&s,ahead,1);s.samples=32;
    memset(pack+40,0,8);pack[43]=64;pack[44]=8; /*4096 decoded samples*/
    /* Native mailbox0x2C measured282Fs with256 initial samples. Charge a
     * conservative0.75Fs/sample and65Fs fixed setup/selection/control. */
    timed_quarters=65u*4u;timed_writes=true;s.now=measured_clock;
    assert(wf_scsp_sfx_oki_start(&s,0,0,1,2,false));
    timed_writes=false;
    assert(measured_clock(NULL)<=256 && "initial SFX decode blocks all transport service beyond256Fs");
    assert(s.pcm[0].write_at==128 && s.pcm[0].decoder.remaining==4096-128);
    wf_sfx_decoder_t expected={.offset=64,.remaining=4096};
    for(unsigned i=0;i<128;i++)assert(output[0][i]==(uint16_t)wf_scsp_sfx_decode(&expected,pack));
    assert(s.pcm[0].decoder.offset==expected.offset && s.pcm[0].decoder.low==expected.low);
    assert(s.pcm[0].decoder.signal==expected.signal && s.pcm[0].decoder.index==expected.index);
    assert(output[0][4096]==output[0][0]);
    /* Account for the complete measured launch, then continue the actual
     * urgent refill policy in a4-voice schedule containing short gaps. */
    wf_scsp_sfx_advance(&s,measured_clock(NULL)+1);
    const unsigned counts[]={32,64,128,128},costs[]={64,96,128,128};
    for(unsigned i=0;i<120;i++){
        wf_scsp_sfx_work_budget(&s,counts[i&3u]);
        wf_scsp_sfx_advance(&s,costs[i&3u]);
        assert(!s.underruns && "smaller SFX prefix lacks playback runway under bounded urgent refills");
    }
    assert(s.pcm[0].playing && s.pcm[0].played>2000);
    /* Music retains its existing256-sample launch and complete cache. */
    assert(wf_scsp_sfx_oki_start(&s,3,0,1,2,false));
    assert(s.pcm[3].write_at==256);
}

int main(int argc,char **argv)
{
    (void)argc;(void)argv;
    initial_sfx_decode_has_bounded_work_and_refill_runway();
    variable_budget_preserves_samples_and_decoder();
    closest_deadline_prevents_measured_starvation();
    equal_margins_keep_round_robin_order();
    finished_sample_padding_does_not_steal_refills();
    puts("PASS urgent refill, equal-margin round robin, exact decoder/sample continuation and bounded end padding");
    return 0;
}
