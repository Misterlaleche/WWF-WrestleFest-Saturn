/* Real adapter, fake Jo hardware/CD boundary. ROM-derived expected PCM is
 * provided independently by the Python oracle; no Saturn MMIO in host tests. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define JO_COMPILE_WITH_AUDIO_SUPPORT
#define JO_COMPILE_WITH_FS_SUPPORT
#include "jo/audio.h"
#include "wf_jo_audio.h"
#include "audio_header_stub.h"

static uint8_t ram[524288];
static unsigned top, now, starts, stops, start_sample_bytes, music_start;
static unsigned channel_until[14];
static bool music, pending;
static unsigned heap_allocations;
static bool heap_fail;
static unsigned prefix_cache_calls,prefix_fail_id;
static void *cached_storage[19];
static unsigned cached_bytes[19];
static bool service_reenter;
static unsigned music_services;
static unsigned music_volume, music_volume_calls;
/* Weak only so the pre-implementation test fails on the missing entry point
 * at runtime, before evaluating its behavioral requirements. */
extern bool wf_jo_audio_service_cooperate(void) __attribute__((weak));
extern void wf_audio_test_bulk_writes(void);
extern void wf_audio_test_ring_bind(void);
extern void wf_audio_test_ring_convert(unsigned);
unsigned jo_get_ticks(void) { return now; }
void *jo_malloc(unsigned int n) {++heap_allocations;return heap_fail?NULL:malloc(n);}
void jo_free(void *p) {free(p);}
bool jo_audio_reserve_resident(jo_sound *s, unsigned bytes)
{
    unsigned start=(top-((bytes+3u)&~3u))&~31u;
    if(bytes>top || start<0x1c00u+0x8000u)return false;
    top=start;s->resident=start+1u;s->data=NULL;s->data_length=bytes;return true;
}
bool jo_audio_write_resident(const jo_sound *s,unsigned at,const void *data,unsigned bytes)
{
    assert(s && s->resident && at<=s->data_length && bytes<=s->data_length-at);
    assert(s->resident-1u+at+bytes<=sizeof ram);
    memcpy(ram+s->resident-1u+at,data,bytes);return true;
}
bool jo_audio_sync(void) { return true; }
void jo_audio_stop_channel(unsigned char ch) {channel_until[ch]=0;++stops;}
bool jo_audio_is_channel_playing(unsigned char ch) {return channel_until[ch]>now;}
void jo_audio_play_sound_on_channel(jo_sound *s,unsigned char ch)
{
    assert(ch<3 && s->resident && s->sample_rate==8000 && s->mode==JoSoundMono16Bit);
    assert(s->resident-1u+s->data_length<=sizeof ram);
    channel_until[ch]=now+(s->data_length/2u+7u)/8u;
    ++starts;start_sample_bytes=s->data_length;
}
bool jo_audio_prepare_music(const char *name,jo_audio_prepared_music *out)
{(void)name;memset(out,0,sizeof *out);out->fad=1;out->rate=44100;return true;}
bool jo_audio_prepare_music_from_header(const char *name,const unsigned char *header,
                                      unsigned bytes,unsigned size,jo_audio_prepared_music *out)
{wf_test_check_music_header(name,header,bytes,size);return jo_audio_prepare_music(name,out);}
bool jo_audio_cache_music_prefix(const char *n,jo_audio_prepared_music *p,void *s,unsigned c)
{
    unsigned id=n[3]=='B'?11u:8u;
    ++prefix_cache_calls;
    if(id==prefix_fail_id)return false;
    cached_storage[id]=s;cached_bytes[id]=c;
    memset(s,0x5a,c);p->prefix=s;p->prefix_bytes=c;return true;
}
bool jo_audio_reserve_music(void) {top=(top-65544u)&~31u;return true;}
bool jo_audio_stage_music(const jo_audio_prepared_music *m)
{(void)m;pending=true;return true;}
bool jo_audio_prime_prepared_music(const jo_audio_prepared_music *m)
{return jo_audio_stage_music(m);}
bool jo_audio_is_music_ready(void) {return pending;}
bool jo_audio_play_prepared_music(const jo_audio_prepared_music *m,bool loop)
{(void)m;assert(!loop);music=true;pending=false;music_start=now;return true;}
void jo_audio_service_music(void) {
    ++music_services;
    if(service_reenter){
        unsigned before=music_services;service_reenter=false;
        assert(wf_jo_audio_service_cooperate());
        assert(music_services==before);
    }
}
bool jo_audio_is_music_pending(void) {return pending;}
bool jo_audio_is_music_playing(void) {return music;}
void jo_audio_stop_music(void) {music=false;pending=false;}
void jo_audio_set_music_volume(unsigned char volume)
{
    assert(volume<=127);music_volume=volume;++music_volume_calls;
}
unsigned jo_audio_get_music_underruns(void) {return 0;}
unsigned jo_audio_get_music_start_ticks(void) {return music_start;}
unsigned jo_audio_get_music_io_errors(void) {return 0;}

static uint8_t *readfile(const char *p,size_t *n)
{
    FILE*f=fopen(p,"rb");uint8_t *b;assert(f);assert(!fseek(f,0,SEEK_END));
    *n=(size_t)ftell(f);rewind(f);b=malloc(*n);assert(b);assert(fread(b,1,*n,f)==*n);fclose(f);return b;
}
static void reset(const void *pack,size_t n)
{
    memset(ram,0xa5,sizeof ram);memset(channel_until,0,sizeof channel_until);
    top=sizeof ram;now=0;starts=stops=0;music=pending=false;
    heap_allocations=music_services=0;music_volume=0;music_volume_calls=0;
    service_reenter=heap_fail=false;
    prefix_cache_calls=prefix_fail_id=0;
    memset(cached_storage,0,sizeof cached_storage);memset(cached_bytes,0,sizeof cached_bytes);
    assert(wf_jo_audio_init(pack,n));
    assert(top>=0x1c00u+0x8000u);
}
static void run(unsigned ms)
{for(unsigned i=0;i<ms;++i){++now;assert(wf_r18_scsp_service());}}
static void frames(unsigned count)
{for(unsigned i=0;i<count;++i){now+=i%3u==0?16u:17u;assert(wf_r18_scsp_service());}}
static void send(unsigned raw)
{assert(wf_r18_scsp_audio(NULL,(uint16_t)raw,NULL)==WF_OK);assert(wf_r18_scsp_service());}

int main(int argc,char **argv)
{
    size_t n,pn;uint8_t *pack,*expected;assert(argc==3);
    pack=readfile(argv[1],&n);expected=readfile(argv[2],&pn);
    /* Initial scene preparation precedes audio setup on Saturn. */
    assert(!wf_jo_audio_diagnostic.initialized);
    wf_audio_test_ring_bind();wf_audio_test_bulk_writes();assert(!music_services);
    reset(pack,n);
    /* The first common effect is resident; no frame-time ADPCM decode. */
    send(0x3121);assert(starts==1);assert(start_sample_bytes==1802u*2u);
    assert(wf_jo_audio_diagnostic.resident_hits==1);
    assert(wf_jo_audio_diagnostic.decode_samples==0);
    /* Original F1 remains latched after PCM ends. 38 lasts 285 Timer-A ticks;
     * its second event (flags=1) must not play the sample again. */
    reset(pack,n);send(0x3138);
    assert(wf_jo_audio_diagnostic.decode_samples==512); /* Launch work cannot monopolize its frame. */
    run(1600);assert(starts==1);
    assert(wf_jo_audio_diagnostic.f1_skips==1);
    send(0x3138);assert(starts==2);assert(wf_jo_audio_diagnostic.cache_hits==1);
    /* No artificial physical channel shortcut changes three-voice priority. */
    reset(pack,n);send(0x3138);send(0x3138);send(0x3138);send(0x3121);
    assert(starts==3);assert(wf_jo_audio_diagnostic.priority_rejects==1);
    /* Full queues reject before publishing a command; later service is ordered. */
    reset(pack,n);for(unsigned i=0;i<64;++i)assert(wf_r18_scsp_audio(NULL,0x3120,NULL)==WF_OK);
    assert(wf_r18_scsp_audio(NULL,0x3121,NULL)==WF_BAD_STATE);
    assert(wf_jo_audio_diagnostic.commands_received==64);assert(wf_r18_scsp_service());
    assert(wf_jo_audio_diagnostic.commands_applied==64);
    /* A corrupted pack cannot reach a sound-RAM write. */
    pack[n-2]^=1;assert(!wf_jo_audio_pack_valid(pack,n));pack[n-2]^=1;
    assert(!wf_jo_audio_pack_valid(pack,n-1));
    /* Decoder output is compared against all independently generated PCM. */
    assert(wf_jo_audio_test_decode_catalog(pack,n,expected,pn));
    /* Every command is played at the same service cadence as the real port.
     * The independent ROM audit requires exactly 100 starts, not 117. */
    unsigned catalog_starts=0,max_work=0;
    for(unsigned cmd=0x20;cmd<=0x81;++cmd){
        reset(pack,n);send(0x3100u|cmd);frames(720);
        assert(wf_jo_audio_diagnostic.pcm_underruns==0);
        assert(wf_jo_audio_diagnostic.invalid_bank_starts==0);
        assert(wf_jo_audio_diagnostic.max_decode_samples_service<=WF_JO_AUDIO_DECODE_BUDGET);
        catalog_starts+=starts;
        if(wf_jo_audio_diagnostic.max_decode_samples_service>max_work)max_work=wf_jo_audio_diagnostic.max_decode_samples_service;
    }
    assert(catalog_starts==100);
    /* Three long voices share the budget, preserving audio with 60 Hz service. */
    reset(pack,n);send(0x3132);send(0x3132);send(0x3132);frames(360);
    assert(starts==3 && wf_jo_audio_diagnostic.pcm_underruns==0);
    assert(wf_jo_audio_diagnostic.max_decode_samples_service<=WF_JO_AUDIO_DECODE_BUDGET);
    /* A missed refill is explicit and stops before unwritten PCM is reused. */
    reset(pack,n);send(0x3132);now+=500;assert(wf_r18_scsp_service());
    assert(wf_jo_audio_diagnostic.pcm_underruns==1);
    /* Caller-owned prefix08 storage preserves the small frontend heap. Both
     * regions retain guard words; invalid alignment/length is rejected. */
    {
        static uint32_t prefix08[8194],prefix0b[8194];
        reset(pack,n);memset(prefix08,0xa5,sizeof prefix08);memset(prefix0b,0xa5,sizeof prefix0b);
        assert(!wf_jo_audio_set_prefix_workspace(NULL,32768));
        assert(!wf_jo_audio_set_prefix_workspace((uint8_t*)(prefix08+1)+1,32768));
        assert(!wf_jo_audio_set_prefix_workspace(prefix08+1,32767));
        assert(wf_jo_audio_set_prefix_workspace(prefix08+1,32768));
        assert(wf_jo_audio_prefetch_music(prefix0b+1,32768)==0x900u);
        assert(heap_allocations==0);
        assert(prefix08[0]==0xa5a5a5a5u&&prefix08[8193]==0xa5a5a5a5u);
        assert(prefix0b[0]==0xa5a5a5a5u&&prefix0b[8193]==0xa5a5a5a5u);
        assert(prefix08[1]==0x5a5a5a5au&&prefix08[8192]==0x5a5a5a5au);
        assert(!wf_jo_audio_set_prefix_workspace(prefix08+1,32768));
    }
    /* Exact 96 KiB cart lease caches64 KiB0B +32 KiB08 without heap or
     * LWRAM. The optional old workspace is unused on the larger path. */
    {
        static uint32_t cart[24578],workspace[8194];
        for(unsigned provided=0;provided<2;++provided){
            reset(pack,n);memset(cart,0xa5,sizeof cart);memset(workspace,0xa5,sizeof workspace);
            if(provided)assert(wf_jo_audio_set_prefix_workspace(workspace+1,32768));
            assert(wf_jo_audio_prefetch_music(cart+1,98304)==0x900u);
            assert(cached_storage[11]==cart+1 && cached_bytes[11]==65536u);
            assert(cached_storage[8]==(uint8_t*)(cart+1)+65536u && cached_bytes[8]==32768u);
            assert(wf_jo_audio_diagnostic.music_prefetch_bytes==98304u && !heap_allocations);
            assert(cart[0]==0xa5a5a5a5u && cart[24577]==0xa5a5a5a5u);
            for(unsigned i=0;i<8194u;++i)assert(workspace[i]==0xa5a5a5a5u);
            assert(wf_jo_audio_prefetch_music(cart+1,98304)==0x900u && prefix_cache_calls==2u);
        }
        /* A one-byte-short lease keeps the old two32 KiB layout. */
        reset(pack,n);memset(cart,0xa5,sizeof cart);
        assert(wf_jo_audio_prefetch_music(cart+1,98303)==0x900u);
        assert(cached_bytes[11]==32768u && cached_storage[8]==cart+8193);
        assert(wf_jo_audio_diagnostic.music_prefetch_bytes==65536u && !heap_allocations);
        for(unsigned i=16385;i<24578;++i)assert(cart[i]==0xa5a5a5a5u);
        /* Failed reads can be retried without sliding one cue over another. */
        for(unsigned failed=0;failed<2;++failed){
            reset(pack,n);memset(cart,0xa5,sizeof cart);prefix_fail_id=failed?8u:11u;
            assert(wf_jo_audio_prefetch_music(cart+1,98304)==(failed?0x800u:0x100u));
            prefix_fail_id=0;
            if(failed)assert(wf_jo_audio_prefetch_music(cart+1,65536)==0x800u && prefix_cache_calls==2u);
            assert(wf_jo_audio_prefetch_music(cart+1,98304)==0x900u);
            assert(prefix_cache_calls==3u && !heap_allocations);
            assert(cached_storage[11]==cart+1 && cached_bytes[11]==65536u);
            assert(cached_storage[8]==cart+16385 && cached_bytes[8]==32768u);
            assert(wf_jo_audio_diagnostic.music_prefetch_bytes==98304u);
            assert(cart[0]==0xa5a5a5a5u && cart[24577]==0xa5a5a5a5u);
        }
        reset(pack,n);
        assert(!wf_jo_audio_prefetch_music((uint8_t*)(cart+1)+1,98304));
        assert(!wf_jo_audio_prefetch_music((void*)(UINTPTR_MAX-3u),98304));
        assert(!wf_jo_audio_prefetch_music(pack,n));
        assert(!prefix_cache_calls && !heap_allocations && wf_jo_audio_pack_valid(pack,n));
        assert(!wf_jo_audio_set_prefix_workspace(pack,32768));
        assert(!wf_jo_audio_set_prefix_workspace((void*)(UINTPTR_MAX-3u),32768));
        assert(wf_jo_audio_set_prefix_workspace(cart+4097,32768));
        assert(!wf_jo_audio_prefetch_music(cart+1,32768)); /* Overlapping borrowed leases. */
        assert(!prefix_cache_calls && !heap_allocations);
        reset(pack,n);assert(wf_jo_audio_prime_music(4));
        assert(!wf_jo_audio_prefetch_music(cart+1,98304) && !prefix_cache_calls);
        send(0x3104);
        assert(!wf_jo_audio_prefetch_music(cart+1,98304) && !prefix_cache_calls);
    }
    /* Preferred112 KiB lease extends only08 to48 KiB. A shorter lease keeps
     * the established96 KiB layout, including0B's64 KiB and fixed offsets. */
    {
        static uint32_t cart[28674],workspace[8194];
        for(unsigned provided=0;provided<2;++provided){
            reset(pack,n);memset(cart,0xa5,sizeof cart);memset(workspace,0xa5,sizeof workspace);
            if(provided)assert(wf_jo_audio_set_prefix_workspace(workspace+1,32768));
            assert(wf_jo_audio_prefetch_music(cart+1,114688)==0x900u);
            assert(cached_storage[11]==cart+1 && cached_bytes[11]==65536u);
            assert(cached_storage[8]==cart+16385 && cached_bytes[8]==49152u);
            assert(wf_jo_audio_diagnostic.music_prefetch_bytes==114688u && !heap_allocations);
            assert(cart[0]==0xa5a5a5a5u && cart[28673]==0xa5a5a5a5u);
            for(unsigned i=0;i<8194u;++i)assert(workspace[i]==0xa5a5a5a5u);
            assert(wf_jo_audio_prefetch_music(cart+1,114688)==0x900u && prefix_cache_calls==2u);
        }
        reset(pack,n);memset(cart,0xa5,sizeof cart);
        assert(wf_jo_audio_prefetch_music(cart+1,114687)==0x900u);
        assert(cached_bytes[11]==65536u && cached_bytes[8]==32768u);
        assert(cached_storage[8]==cart+16385 && !heap_allocations);
        assert(wf_jo_audio_diagnostic.music_prefetch_bytes==98304u);
        for(unsigned i=24577;i<28674;++i)assert(cart[i]==0xa5a5a5a5u);
        for(unsigned failed=0;failed<2;++failed){
            reset(pack,n);memset(cart,0xa5,sizeof cart);prefix_fail_id=failed?8u:11u;
            assert(wf_jo_audio_prefetch_music(cart+1,114688)==(failed?0x800u:0x100u));
            prefix_fail_id=0;
            if(failed)assert(wf_jo_audio_prefetch_music(cart+1,65536)==0x800u && prefix_cache_calls==2u);
            assert(wf_jo_audio_prefetch_music(cart+1,114688)==0x900u);
            assert(prefix_cache_calls==3u && !heap_allocations);
            assert(cached_storage[11]==cart+1 && cached_bytes[11]==65536u);
            assert(cached_storage[8]==cart+16385 && cached_bytes[8]==49152u);
            assert(wf_jo_audio_diagnostic.music_prefetch_bytes==114688u);
            assert(cart[0]==0xa5a5a5a5u && cart[28673]==0xa5a5a5a5u);
        }
        reset(pack,n);assert(wf_jo_audio_prime_music(4));
        assert(!wf_jo_audio_prefetch_music(cart+1,114688) && !prefix_cache_calls);
        send(0x3104);
        assert(!wf_jo_audio_prefetch_music(cart+1,114688) && !prefix_cache_calls);
        reset(pack,n);heap_fail=true;
        assert(!wf_jo_audio_prefetch_music(NULL,114688));
        assert(heap_allocations==2u && !prefix_cache_calls);
        assert(!wf_jo_audio_diagnostic.music_prefetch_bytes);
        heap_fail=false;
        assert(wf_jo_audio_prefetch_music(cart+1,114688)==0x900u);
        assert(heap_allocations==2u && prefix_cache_calls==2u);
    }
    reset(pack,n);assert(wf_jo_audio_prime_music(4));send(0x3104);assert(music && music_start==now);
    now+=1000;send(0x3104);assert(music_start==now); /* Original Z80 restarts this command. */
    /* Cabinet RESET emits00 after loading. Stopping audible voices must not
     * discard an inaudible cache of the future title cue. */
    reset(pack,n);assert(wf_jo_audio_prime_music(2));assert(pending&&!music);
    send(0x3100);assert(pending&&!music);
    assert(wf_jo_audio_diagnostic.preserved_primes==1);
    send(0x3102);assert(music&&!pending);
    send(0x3100);assert(!music&&!pending); /* Audible stop behavior is unchanged. */
    /* A renderer's cooperative boundary drains a newly emitted command
     * immediately, even within four milliseconds of the previous service.
     * Empty calls are throttled; a long decode run can provide bounded
     * service between completed blocks without recursing or waiting fields. */
    reset(pack,n);assert(wf_jo_audio_prime_music(4));
    assert(wf_jo_audio_service_cooperate);
    assert(wf_jo_audio_service_cooperate());assert(music_services==0);
    assert(wf_r18_scsp_audio(NULL,0x3104,NULL)==WF_OK);
    service_reenter=true;assert(wf_jo_audio_service_cooperate());
    assert(music&&music_start==0&&music_services==1);
    now=3;assert(wf_jo_audio_service_cooperate());assert(music_services==1);
    now=4;assert(wf_jo_audio_service_cooperate());assert(music_services==2);
    assert(wf_r18_scsp_audio(NULL,0x3132,NULL)==WF_OK);
    assert(wf_jo_audio_service_cooperate());
    for(unsigned block=0;block<25;++block){now+=12;assert(wf_jo_audio_service_cooperate());}
    assert(wf_jo_audio_diagnostic.pcm_underruns==0);
    assert(wf_jo_audio_diagnostic.max_decode_samples_service<=WF_JO_AUDIO_DECODE_BUDGET);
    /* Cabinet bulk writes and both ring stages must offer a service boundary
     * while preserving pixels and the arcade frame counter. Removing any of
     * these hooks leaves the queued cue unplayed at that boundary. */
    reset(pack,n);assert(wf_jo_audio_prime_music(4));
    assert(wf_r18_scsp_audio(NULL,0x3104,NULL)==WF_OK);
    wf_audio_test_bulk_writes();assert(music);
    reset(pack,n);assert(wf_jo_audio_prime_music(4));
    assert(wf_r18_scsp_audio(NULL,0x3104,NULL)==WF_OK);
    wf_audio_test_ring_bind();assert(music);
    reset(pack,n);assert(wf_jo_audio_prime_music(4));
    assert(wf_r18_scsp_audio(NULL,0x3104,NULL)==WF_OK);
    wf_audio_test_ring_convert(0);assert(music);
    reset(pack,n);assert(wf_jo_audio_prime_music(4));
    assert(wf_r18_scsp_audio(NULL,0x3104,NULL)==WF_OK);
    wf_audio_test_ring_convert(1);assert(music);
    /* The arcade 1F command is a roughly five-second logarithmic fade, not
     * an abrupt stop. Jo's 0..127 music volume maps to logarithmic SCSP TL,
     * so 100 one-level steps over five seconds reproduce the measured
     * approximately 15 dB/second decay before the final key-off. */
    send(0x3104);assert(music&&music_volume==127);
    send(0x311f);assert(music);assert(wf_jo_audio_diagnostic.unsupported_fades==0);
    assert(wf_jo_audio_diagnostic.music_fades==1);
    run(49);assert(music&&music_volume==127);
    run(1);assert(music&&music_volume==126);
    run(4900);assert(music&&music_volume==28);
    run(50);assert(!music&&music_volume==0);
    assert(wf_jo_audio_diagnostic.music_fade_completions==1);
    /* Starting another cue cancels an in-flight fade and restores full level. */
    send(0x3104);send(0x311f);run(100);assert(music_volume==125);
    send(0x3102);assert(music&&music_volume==127);
    run(120);assert(music&&music_volume==127);
    assert(wf_jo_audio_diagnostic.music_fades==2);
    assert(wf_jo_audio_diagnostic.music_fade_completions==1);
    assert(wf_jo_audio_diagnostic.max_decode_samples_service<=WF_JO_AUDIO_DECODE_BUDGET);
    free(pack);free(expected);
    printf("PASS: PCM identities; 98 commands at 60 Hz -> %u starts; maximum decode %u frames/service; F1, priority, FIFO, CRC, bounds, underrun and music routing\n",catalog_starts,max_work);
    return 0;
}
