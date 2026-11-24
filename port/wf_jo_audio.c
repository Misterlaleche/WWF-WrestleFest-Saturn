/* Jo 2026.3 adapter for Audio 2 WFSX v1.
 * Sources: Audio 2 6d6a570, F1 audit, original Z80 command table. This adapter
 * preserves sample identities and command ordering; it is not a claim of
 * physical arcade waveform or event-time equivalence. See audio-manifest.json.
 */
#include "wf_jo_audio.h"
#include "wf_jo_music_headers.h"
#include <string.h>
#ifdef WF_JO_AUDIO_HOST
#define JO_COMPILE_WITH_AUDIO_SUPPORT
#define JO_COMPILE_WITH_FS_SUPPORT
#include "jo/audio.h"
extern unsigned jo_get_ticks(void);
extern void *jo_malloc(unsigned int);
extern void jo_free(void *);
#else
#include <jo/jo.h>
#endif

#define COMMANDS 98u
#define SAMPLE_ROWS 116u
#define QUEUE_SIZE 64u
#define VOICES 3u
#define DYNAMIC_BYTES 72600u
#define LAUNCH_FRAMES 512u
#define AHEAD_FRAMES 1024u
#define TIMER_NUM 3579545u
#define TIMER_DEN (1000u * 64u * 253u)
#define COMMON_COUNT 18u
#define MUSIC_COUNT 18u
#define PREFIX_SMALL WF_JO_AUDIO_PREFIX_WORKSPACE_BYTES
#define PREFIX_LARGE 65536u
#define MUSIC_FADE_STEP_MS 50u
#define MUSIC_FADE_STEPS 100u
#define MUSIC_FADE_START_LEVEL 127u
#define MUSIC_FADE_DURATION_MS (MUSIC_FADE_STEP_MS * MUSIC_FADE_STEPS)

typedef struct {
    unsigned offset, frames, decoded;
    int signal, index;
} decoder_t;
typedef struct {
    jo_sound allocation, playback;
    decoder_t decoder;
    unsigned sample_row, ready_row, requested_ms, started_ms;
    uint8_t volume, filling, pending, playing;
} pcm_voice_t;
typedef struct {
    uint8_t active, command, event, priority;
    uint16_t remaining;
} sequence_t;
typedef struct { uint16_t raw; uint32_t ms; } command_t;
typedef struct { uint8_t id, valid; jo_audio_prepared_music prepared; } music_t;

volatile wf_jo_audio_diagnostic_t wf_jo_audio_diagnostic;
static const uint8_t common_ids[COMMON_COUNT]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,50,51};
static const uint8_t music_ids[MUSIC_COUNT]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18};
_Static_assert(sizeof wf_jo_music_headers/sizeof wf_jo_music_headers[0] == MUSIC_COUNT,
               "Music header count must match the prepared cue table");
/* SCSP TL rounds the arcade 16-level OKI gain to the closest available Jo
 * two-TL-step volume, then includes the board reference's ~0.66 OKI gain.
 * This quantization is disclosed; the PCM itself is not normalized. */
static const uint8_t volumes[16]={122,118,114,110,106,103,98,95,90,0,0,0,0,0,0,0};
static const int16_t steps[49]={16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552};
static const int8_t shifts[8]={-1,-1,-1,-1,2,4,6,8};
static int16_t delta_lut[49][16];
static uint8_t index_lut[49][8];
static const uint8_t *pack;
static unsigned sample_offset, command_offset, event_offset;
static uint8_t resident_map[SAMPLE_ROWS];
static jo_sound residents[COMMON_COUNT];
static pcm_voice_t pcm[VOICES];
static sequence_t sequence[VOICES];
static command_t queue[QUEUE_SIZE];
static unsigned queue_read, queue_write, bank, last_ms;
static uint64_t timer_phase;
static music_t music[MUSIC_COUNT];
static bool servicing;
static unsigned music_command, music_requested_ms, music_was_playing;
static unsigned music_fade_started_ms, music_fade_level;
static bool music_fade_active;
static void *prefix_workspace;
static uint16_t stage[256] __attribute__((aligned(4)));
static bool initialized;
static bool emergency_stopped;

static unsigned be16(const uint8_t *p) {return (unsigned)p[0]*256u+p[1];}
static uint32_t be32(const uint8_t *p) {return (uint32_t)be16(p)*65536u+be16(p+2);}
static void maximum(volatile uint32_t *p,unsigned n) {if(n>*p)*p=n;}
static uint32_t crc32(const uint8_t *p,size_t n)
{
    uint32_t c=~0u;
    while(n--){c^=*p++;for(unsigned j=0;j<8;++j)c=(c>>1)^(0xedb88320u & (0u-(c&1u)));}
    return ~c;
}

bool wf_jo_audio_pack_valid(const void *data,size_t n)
{
    const uint8_t *p=data;unsigned samples,commands,events,raw;
    if(!p||n<32u||n>0x6c000u||(n&1u)||be32(p)!=0x57465358u||
       be16(p+4)!=1||be16(p+6)!=SAMPLE_ROWS||be32(p+8)!=n)return false;
    samples=be32(p+12);commands=be32(p+16);events=be32(p+20);raw=be32(p+24);
    if(samples!=32||commands!=960||events!=1548||raw<events||raw>n||((raw-events)&3u))return false;
    if(crc32(p+32,n-32)!=be32(p+28))return false;
    for(unsigned i=0;i<SAMPLE_ROWS;++i){
        const uint8_t *e=p+samples+i*8u;unsigned at=be32(e),len=be16(e+4);
        if(i==0||i==74){if(at||len||be16(e+6))return false;continue;}
        if(!len||len>DYNAMIC_BYTES/4u||at<raw||at>n||len>n-at||!be16(e+6))return false;
    }
    for(unsigned i=0;i<COMMANDS;++i){
        const uint8_t*c=p+commands+i*6u;unsigned at=be16(c+4);
        if(c[1]>2||c[2]>9||c[3]>1||(at&3u)||at>raw-events||c[2]*4u>raw-events-at)return false;
        for(unsigned j=0;j<c[2];++j){
            const uint8_t *e=p+events+at+j*4u;
            if(!e[0]||e[0]>(c[1]==1?41u:c[1]==0?73u:19u)||e[2]>15||e[3]>3)return false;
        }
    }
    return true;
}

static void decoder_tables(void)
{
    for(int i=0;i<49;++i){
        for(int n=0;n<16;++n){
            int s=steps[i];int d=(s>>3)+((n&1)?s>>2:0)+((n&2)?s>>1:0)+((n&4)?s:0);
            delta_lut[i][n]=(int16_t)((n&8)?-d:d);
        }
        for(int n=0;n<8;++n){int next=i+shifts[n];if(next<0)next=0;if(next>48)next=48;index_lut[i][n]=(uint8_t)next;}
    }
}
static decoder_t decoder_start(unsigned row)
{
    const uint8_t *e=pack+sample_offset+row*8u;
    decoder_t d={be32(e),be16(e+4)*2u,0,0,0};return d;
}
static int16_t decode_one(decoder_t *d)
{
    unsigned byte=pack[d->offset+(d->decoded>>1)];
    unsigned nibble=(d->decoded&1u)?byte&15u:byte>>4;
    int s=d->signal+delta_lut[d->index][nibble];
    if(s>2047)s=2047;
    if(s< -2048)s= -2048;
    d->signal=s;d->index=index_lut[d->index][nibble&7u];++d->decoded;
    return (int16_t)(s*16);
}
static bool decode_to(decoder_t *d,const jo_sound *allocation,unsigned count)
{
    if(count>d->frames-d->decoded)count=d->frames-d->decoded;
    while(count){
        unsigned n=count>256u?256u:count;unsigned at=d->decoded*2u;
        for(unsigned i=0;i<n;++i){
            uint16_t v=(uint16_t)decode_one(d);
            /* The SCSP upload is always big-endian PCM. A typed aligned
             * sample store cannot alias the decoder's int state. */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            stage[i]=v;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            stage[i]=(uint16_t)((v<<8)|(v>>8));
#else
            uint8_t *bytes=(uint8_t *)(void *)stage;
            bytes[i*2u]=(uint8_t)(v>>8);bytes[i*2u+1u]=(uint8_t)v;
#endif
        }
        if(!jo_audio_write_resident(allocation,at,stage,n*2u))return false;
        count-=n;
    }
    return true;
}
static void stop_pcm(unsigned v)
{
    jo_audio_stop_channel((unsigned char)v);
    pcm[v].pending=pcm[v].playing=pcm[v].filling=0;
}
static const uint8_t *command_row(unsigned c) {return pack+command_offset+(c-0x20u)*6u;}

static void start_pcm(unsigned v,unsigned id,unsigned volume,unsigned now)
{
    unsigned row=bank*74u+id;
    pcm_voice_t *p=&pcm[v];
    /* The ROM's E800 bank register affects every active OKI voice. Predecoded
     * SCSP samples cannot emulate that address remapping; the counter records
     * exposures. New note lookup does use the current GLOBAL bank. */
    stop_pcm(v);
    if(id==0||id>(bank?41u:73u)){++wf_jo_audio_diagnostic.invalid_bank_starts;return;}
    p->volume=(uint8_t)volume;p->requested_ms=now;p->sample_row=row;
    unsigned resident=resident_map[row];
    if(resident){
        p->playback=residents[resident-1u];++wf_jo_audio_diagnostic.resident_hits;
    }else if(p->ready_row==row){
        p->playback=p->allocation;p->playback.data_length=be16(pack+sample_offset+row*8u+4u)*4u;
        ++wf_jo_audio_diagnostic.cache_hits;
    }else{
        if(!jo_audio_sync()){wf_jo_audio_diagnostic.fault=4;return;}
        p->ready_row=0;p->decoder=decoder_start(row);p->filling=p->pending=1;
        p->playback=p->allocation;p->playback.data_length=p->decoder.frames*2u;
        ++wf_jo_audio_diagnostic.cache_misses;
        return;
    }
    p->playback.volume=volumes[volume];p->playback.current_playing_channel=(uint8_t)v;
    jo_audio_play_sound_on_channel(&p->playback,(uint8_t)v);
    p->playing=1;p->started_ms=now;++wf_jo_audio_diagnostic.physical_starts;
}

static void fire(unsigned v,const uint8_t *c,unsigned now)
{
    sequence_t *q=&sequence[v];const uint8_t *e=pack+event_offset+be16(c+4)+q->event*4u;
    if((e[3]&3u)==1u)++wf_jo_audio_diagnostic.f1_skips;
    else start_pcm(v,e[0],e[2],now);
    q->remaining=e[1];
}
static void sfx_command(unsigned cmd,unsigned now)
{
    const uint8_t *c=command_row(cmd);unsigned v,best=0;
    if(!c[2])return;
    for(v=0;v<VOICES;++v)if(!sequence[v].active)break;
    if(v==VOICES){
        for(v=1;v<VOICES;++v)if(sequence[v].priority<sequence[best].priority)best=v;
        if(c[0]<sequence[best].priority){++wf_jo_audio_diagnostic.priority_rejects;return;}
        v=best;stop_pcm(v);
    }
    if(c[1]<2&&c[1]!=bank){
        ++wf_jo_audio_diagnostic.bank_changes;
        for(unsigned i=0;i<VOICES;++i)
            if(pcm[i].playing&&jo_audio_is_channel_playing((uint8_t)i))++wf_jo_audio_diagnostic.active_bank_changes;
        bank=c[1];
    }
    sequence[v]=(sequence_t){1,(uint8_t)cmd,0,c[0],0};
    wf_jo_audio_diagnostic.selected_voice=v;fire(v,c,now);
}
static void advance_to(unsigned now)
{
    unsigned elapsed=now-last_ms;last_ms=now;
    timer_phase+=(uint64_t)elapsed*TIMER_NUM;
    unsigned ticks=(unsigned)(timer_phase/TIMER_DEN);timer_phase%=TIMER_DEN;
    if(ticks>1u)wf_jo_audio_diagnostic.late_timer_ticks+=ticks-1u;
    while(ticks--){
        for(unsigned v=0;v<VOICES;++v){
            sequence_t*q=&sequence[v];if(!q->active)continue;
            if(q->remaining&&--q->remaining)continue;
            const uint8_t*c=command_row(q->command);
            if(++q->event>=c[2]){if(c[3]){stop_pcm(v);++wf_jo_audio_diagnostic.cleanup_stops;}q->active=0;}
            else fire(v,c,now);
        }
    }
}
static void music_fade_cancel(void)
{
    music_fade_active=false;music_fade_started_ms=0;music_fade_level=127u;
    wf_jo_audio_diagnostic.music_fade_volume=127u;
}

static void music_fade_service(unsigned now)
{
    if(!music_fade_active)return;
    unsigned elapsed=now-music_fade_started_ms;
    unsigned fade_steps=elapsed/MUSIC_FADE_STEP_MS;
    if(fade_steps>MUSIC_FADE_STEPS)fade_steps=MUSIC_FADE_STEPS;
    unsigned level=MUSIC_FADE_START_LEVEL-fade_steps;
    if(level!=music_fade_level){
        music_fade_level=level;wf_jo_audio_diagnostic.music_fade_volume=level;
        jo_audio_set_music_volume((uint8_t)level);
    }
    if(elapsed>=MUSIC_FADE_DURATION_MS){
        if(music_fade_level){music_fade_level=0;wf_jo_audio_diagnostic.music_fade_volume=0;
            jo_audio_set_music_volume(0);}
        jo_audio_stop_music();music_command=music_was_playing=0;
        music_fade_active=false;++wf_jo_audio_diagnostic.music_fade_completions;
    }
}

static void music_cmd(unsigned cmd,unsigned now)
{
    if(cmd==0){
        /* RESET's00 stops audible hardware. A fully primed, never-started
         * future cue is only a cache: keeping it changes no audible state
         * and avoids throwing away the final-load02 read before its event. */
        if(!music_command&&!jo_audio_is_music_playing()&&
           jo_audio_is_music_pending()&&jo_audio_is_music_ready()){
            ++wf_jo_audio_diagnostic.preserved_primes;return;
        }
        music_fade_cancel();jo_audio_stop_music();music_command=music_was_playing=0;return;
    }
    if(cmd==0x1f){
        /* The original sound program fades logarithmically for about five
         * seconds. Jo's 0..127 volume is an SCSP total-level scale. The
         * measured board envelope is approximated by 100 one-level steps at
         * 50 ms each (about 15 dB/second), followed by a final key-off. */
        if((music_command||jo_audio_is_music_playing())&&!music_fade_active){
            music_fade_active=true;music_fade_started_ms=now;music_fade_level=127u;
            wf_jo_audio_diagnostic.music_fade_volume=127u;
            ++wf_jo_audio_diagnostic.music_fades;
        }
        return;
    }
    for(unsigned i=0;i<MUSIC_COUNT;++i)if(music[i].id==cmd&&music[i].valid){
        music_fade_cancel();music_command=cmd;music_requested_ms=now;music_was_playing=0;
        wf_jo_audio_diagnostic.music_command=cmd;
        jo_audio_set_music_volume(127);
        unsigned began=jo_get_ticks();
        if(!jo_audio_play_prepared_music(&music[i].prepared,false))++wf_jo_audio_diagnostic.music_errors;
        maximum(&wf_jo_audio_diagnostic.music_command_max_ms,jo_get_ticks()-began);
        return;
    }
    if(cmd<32u)wf_jo_audio_diagnostic.music_missing_mask|=1u<<cmd;
    ++wf_jo_audio_diagnostic.music_errors;
}
static void command_apply(unsigned raw,unsigned now)
{
    unsigned cmd=raw&255u;
    ++wf_jo_audio_diagnostic.commands_applied;wf_jo_audio_diagnostic.last_command=cmd;
    if(cmd>=0x20&&cmd<=0x81)sfx_command(cmd,now);
    else if(cmd<=0x12||cmd==0x1f){
        if(!cmd)for(unsigned v=0;v<VOICES;++v){stop_pcm(v);sequence[v].active=0;}
        music_cmd(cmd,now);
    }
}

static bool service_pcm(unsigned now)
{
    unsigned budget=WF_JO_AUDIO_DECODE_BUDGET,used=0,began=jo_get_ticks(),launched=0;
    /* Stop before a one-shot could read samples not decoded yet. We keep the
     * rest of the game alive and disclose the underrun, never play garbage. */
    for(unsigned v=0;v<VOICES;++v){
        pcm_voice_t*p=&pcm[v];
        if(p->playing&&!jo_audio_is_channel_playing((uint8_t)v))p->playing=0;
        if(p->playing&&p->filling&&(now-p->started_ms)*8u+16u>=p->decoder.decoded){
            wf_jo_audio_diagnostic.pcm_underrun_sample=p->sample_row;
            wf_jo_audio_diagnostic.pcm_underrun_decoded=p->decoder.decoded;
            wf_jo_audio_diagnostic.pcm_underrun_elapsed_frames=(now-p->started_ms)*8u;
            stop_pcm(v);++wf_jo_audio_diagnostic.pcm_underruns;
        }
    }
    /* Round robin bounded chunks: simultaneous rare speech never performs a
     * full 36,300-frame decode on the gameplay SH-2. */
    bool progress=true;
    while(budget&&progress){
        progress=false;
        for(unsigned v=0;v<VOICES&&budget;++v){
            pcm_voice_t*p=&pcm[v];if(!p->filling||(launched&(1u<<v)))continue;
            unsigned played=p->pending?0u:(jo_get_ticks()-p->started_ms)*8u;
            unsigned target=played+(p->pending?LAUNCH_FRAMES:AHEAD_FRAMES);
            if(target>p->decoder.frames)target=p->decoder.frames;
            if(p->decoder.decoded>=target)continue;
            unsigned n=target-p->decoder.decoded;if(n>256u)n=256u;if(n>budget)n=budget;
            if(!decode_to(&p->decoder,&p->allocation,n)){wf_jo_audio_diagnostic.fault=5;return false;}
            used+=n;budget-=n;progress=true;
            if(p->decoder.decoded==p->decoder.frames){p->filling=0;p->ready_row=p->sample_row;}
            if(p->pending&&(p->decoder.decoded>=LAUNCH_FRAMES||!p->filling)){
                launched|=1u<<v;
                p->pending=0;p->playing=1;p->started_ms=jo_get_ticks();
                p->playback.volume=volumes[p->volume];p->playback.current_playing_channel=(uint8_t)v;
                jo_audio_play_sound_on_channel(&p->playback,(uint8_t)v);
                ++wf_jo_audio_diagnostic.physical_starts;
                maximum(&wf_jo_audio_diagnostic.launch_max_delay_ms,p->started_ms-p->requested_ms);
            }
        }
    }
    wf_jo_audio_diagnostic.decode_samples+=used;
    maximum(&wf_jo_audio_diagnostic.max_decode_samples_service,used);
    unsigned duration=jo_get_ticks()-began;
    wf_jo_audio_diagnostic.decode_total_ms+=duration;maximum(&wf_jo_audio_diagnostic.decode_max_ms,duration);
    return true;
}

void wf_jo_audio_emergency_stop(void)
{
    /* Silence first, independent of initialized/fault state and without
     * flushing queues, reading the disc or touching the VDP1 pipeline. */
    jo_audio_emergency_mute();
    if(emergency_stopped)return;
    emergency_stopped=true;
    initialized=false;
    wf_jo_audio_diagnostic.initialized=0;
    queue_read=queue_write=0;
    wf_jo_audio_diagnostic.queue_depth=0;
    timer_phase=0;
    music_command=music_was_playing=0;music_fade_cancel();
    for(unsigned v=0;v<VOICES;++v){
        sequence[v].active=0;
        pcm[v].pending=pcm[v].playing=pcm[v].filling=0;
    }
    /* Keep allocations, counters and the first audio fault for diagnosis. */
}

bool wf_jo_audio_init(const void *sfx_pack,size_t bytes)
{
    if(emergency_stopped)return false;
    initialized=false;memset((void*)&wf_jo_audio_diagnostic,0,sizeof wf_jo_audio_diagnostic);
    wf_jo_audio_diagnostic.magic=0x574a4131u;
    if(!wf_jo_audio_pack_valid(sfx_pack,bytes)){wf_jo_audio_diagnostic.fault=1;return false;}
    pack=sfx_pack;sample_offset=be32(pack+12);command_offset=be32(pack+16);event_offset=be32(pack+20);
    memset(residents,0,sizeof residents);memset(resident_map,0,sizeof resident_map);
    memset(pcm,0,sizeof pcm);memset(sequence,0,sizeof sequence);memset(music,0,sizeof music);
    queue_read=queue_write=bank=0;timer_phase=0;music_command=music_was_playing=0;
    music_fade_active=false;music_fade_started_ms=0;music_fade_level=127u;
    wf_jo_audio_diagnostic.music_fade_volume=127u;
    prefix_workspace=NULL;decoder_tables();
    if(!jo_audio_reserve_music()){wf_jo_audio_diagnostic.fault=2;return false;}
    for(unsigned i=0;i<COMMON_COUNT;++i){
        decoder_t d=decoder_start(common_ids[i]);jo_sound*s=&residents[i];
        s->mode=JoSoundMono16Bit;s->sample_rate=8000;s->volume=127;
        if(!jo_audio_reserve_resident(s,d.frames*2u)||!decode_to(&d,s,d.frames)){
            wf_jo_audio_diagnostic.fault=2;return false;
        }
        wf_jo_audio_diagnostic.resident_pcm_bytes+=(s->data_length+31u)&~31u;
        unsigned raw=be32(pack+sample_offset+common_ids[i]*8u);
        for(unsigned row=0;row<SAMPLE_ROWS;++row)
            if(be32(pack+sample_offset+row*8u)==raw)resident_map[row]=(uint8_t)(i+1u);
    }
    for(unsigned v=0;v<VOICES;++v){
        pcm[v].allocation.mode=JoSoundMono16Bit;pcm[v].allocation.sample_rate=8000;pcm[v].allocation.volume=127;
        if(!jo_audio_reserve_resident(&pcm[v].allocation,DYNAMIC_BYTES)){wf_jo_audio_diagnostic.fault=2;return false;}
        wf_jo_audio_diagnostic.dynamic_pcm_bytes+=(DYNAMIC_BYTES+31u)&~31u;
    }
    for(unsigned i=0;i<MUSIC_COUNT;++i){
        static const char hex[]="0123456789ABCDEF";
        char name[]="MU00.WAV";unsigned begun=jo_get_ticks();music[i].id=music_ids[i];
        /* Generated entries are strictly MU01..MU12 (hex). A changed cue
         * order must not silently bind a header to another file. */
        if(music[i].id!=i+1u){
            wf_jo_audio_diagnostic.music_missing_mask|=1u<<(i+1u);continue;
        }
        name[2]=hex[music[i].id>>4];name[3]=hex[music[i].id&15u];
        if(jo_audio_prepare_music_from_header(name,wf_jo_music_headers[i].header,
                sizeof wf_jo_music_headers[i].header,wf_jo_music_headers[i].file_size,&music[i].prepared)){
            music[i].valid=1;wf_jo_audio_diagnostic.music_prepared_mask|=1u<<music[i].id;
        }else wf_jo_audio_diagnostic.music_missing_mask|=1u<<music[i].id;
        maximum(&wf_jo_audio_diagnostic.music_prepare_max_ms,jo_get_ticks()-begun);
    }
    last_ms=jo_get_ticks();initialized=true;
    wf_jo_audio_diagnostic.pack_bytes=(uint32_t)bytes;wf_jo_audio_diagnostic.initialized=1;
    return true;
}

static bool prefix_overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
    return x<y+bn && y<x+an;
}

static bool prefix_available(void *storage,size_t bytes)
{
    if(!storage||((uintptr_t)storage&3u)||bytes>UINTPTR_MAX-(uintptr_t)storage ||
       prefix_overlap(storage,bytes,pack,wf_jo_audio_diagnostic.pack_bytes))return false;
    for(unsigned i=0;i<MUSIC_COUNT;++i)
        if(music[i].prepared.prefix && prefix_overlap(storage,bytes,
                music[i].prepared.prefix,music[i].prepared.prefix_bytes))return false;
    return true;
}

bool wf_jo_audio_set_prefix_workspace(void *storage,size_t bytes)
{
    if(!initialized||bytes<PREFIX_SMALL||bytes>UINTPTR_MAX-(uintptr_t)storage ||
       !prefix_available(storage,PREFIX_SMALL))return false;
    for(unsigned i=0;i<MUSIC_COUNT;++i)
        if(music[i].id==8u&&music[i].prepared.prefix)return false;
    prefix_workspace=storage;
    return true;
}

uint32_t wf_jo_audio_prefetch_music(void *storage,size_t bytes)
{
    const unsigned ids[2]={11u,8u};
    if(!initialized)return 0;
    uint32_t mask=wf_jo_audio_diagnostic.music_prefetch_mask;
    /* These synchronous loading reads must never compete with a staged or
     * playing stream. In particular, priming is the final loading operation. */
    if(jo_audio_is_music_playing()||jo_audio_is_music_pending())return mask;
    if(storage&&(((uintptr_t)storage&3u)||bytes>UINTPTR_MAX-(uintptr_t)storage))return mask;
    bool large=storage&&bytes>=PREFIX_LARGE+PREFIX_SMALL;
    /* Keep the 96 KiB layout for old leases. The larger cue08 reserve covers
     * the 300 ms CD handoff without changing its immediate key-on or PCM. */
    const unsigned sizes[2]={large?PREFIX_LARGE:PREFIX_SMALL,
        storage&&bytes>=WF_JO_AUDIO_CART_PREFIX_BYTES?
            WF_JO_AUDIO_CART_PREFIX_BYTES-PREFIX_LARGE:PREFIX_SMALL};
    void *targets[2]={NULL,NULL};
    music_t *cues[2]={NULL,NULL};
    if(storage&&bytes>=sizes[0])targets[0]=storage;
    if(large)targets[1]=(uint8_t*)storage+PREFIX_LARGE;
    else if(prefix_workspace)targets[1]=prefix_workspace;
    else if(storage&&bytes>=2u*PREFIX_SMALL)targets[1]=(uint8_t*)storage+PREFIX_SMALL;
    for(unsigned j=0;j<2;++j){
        for(unsigned i=0;i<MUSIC_COUNT;++i)
            if(music[i].id==ids[j]&&music[i].valid&&!music[i].prepared.prefix)cues[j]=&music[i];
        if(cues[j]&&targets[j]&&!prefix_available(targets[j],sizes[j]))return mask;
    }
    if(cues[0]&&cues[1]&&targets[0]&&targets[1]&&
       prefix_overlap(targets[0],sizes[0],targets[1],sizes[1]))return mask;
    for(unsigned j=0;j<2;++j){
        if(cues[j]){
            void *target=targets[j];bool owned=false;unsigned began=jo_get_ticks();
            if(!target){target=jo_malloc(sizes[j]);owned=true;}
            if(target){
                const char *name=ids[j]==11u?"MU0B.WAV":"MU08.WAV";
                if(jo_audio_cache_music_prefix(name,&cues[j]->prepared,target,sizes[j])){
                    wf_jo_audio_diagnostic.music_prefetch_mask|=1u<<ids[j];
                    wf_jo_audio_diagnostic.music_prefetch_bytes+=cues[j]->prepared.prefix_bytes;
                }else if(owned)jo_free(target);
                /* Both borrowed and Jo-owned successful caches live for the
                 * backend lifetime. A failed read is visible in the mask. */
            }
            maximum(&wf_jo_audio_diagnostic.music_prepare_max_ms,jo_get_ticks()-began);
        }
    }
    return wf_jo_audio_diagnostic.music_prefetch_mask;
}

bool wf_jo_audio_prime_music(uint8_t command)
{
    if(!initialized)return false;
    for(unsigned i=0;i<MUSIC_COUNT;++i)if(music[i].id==command&&music[i].valid){
        unsigned began=jo_get_ticks(),polls=0;
        if(!jo_audio_prime_prepared_music(&music[i].prepared))return false;
        while(!jo_audio_is_music_ready()&&jo_get_ticks()-began<2000u&&polls++<200000u)
            jo_audio_service_music();
        maximum(&wf_jo_audio_diagnostic.music_prepare_max_ms,jo_get_ticks()-began);
        last_ms=jo_get_ticks();
        if(jo_audio_is_music_ready())return true;
        ++wf_jo_audio_diagnostic.music_errors;jo_audio_stop_music();return false;
    }
    return false;
}

wf_status_t wf_r18_scsp_audio(wf_runtime_t *runtime,uint16_t raw,void *user)
{
    (void)runtime;(void)user;
    if(!initialized)return WF_BAD_STATE;
    if(queue_write-queue_read==QUEUE_SIZE){++wf_jo_audio_diagnostic.queue_overflows;return WF_BAD_STATE;}
    queue[queue_write&(QUEUE_SIZE-1u)]=(command_t){raw,jo_get_ticks()};++queue_write;
    ++wf_jo_audio_diagnostic.commands_received;wf_jo_audio_diagnostic.last_raw=raw;
    wf_jo_audio_diagnostic.queue_depth=queue_write-queue_read;
    maximum(&wf_jo_audio_diagnostic.queue_peak,queue_write-queue_read);
    return WF_OK;
}
bool wf_r18_scsp_service(void)
{
    if(!initialized||wf_jo_audio_diagnostic.fault)return false;
    if(servicing)return true;
    servicing=true;
    unsigned now=jo_get_ticks();maximum(&wf_jo_audio_diagnostic.service_max_gap_ms,now-last_ms);
    while(queue_read!=queue_write){
        command_t c=queue[queue_read&(QUEUE_SIZE-1u)];++queue_read;
        maximum(&wf_jo_audio_diagnostic.command_queue_max_ms,now-c.ms);
        advance_to(c.ms);command_apply(c.raw,c.ms);
    }
    wf_jo_audio_diagnostic.queue_depth=0;advance_to(now);
    if(!service_pcm(now)){servicing=false;return false;}
    unsigned music_began=jo_get_ticks();
    jo_audio_service_music();
    music_fade_service(now);
    maximum(&wf_jo_audio_diagnostic.music_service_max_ms,jo_get_ticks()-music_began);
    maximum(&wf_jo_audio_diagnostic.music_io_errors,jo_audio_get_music_io_errors());
    if(music_command){
        if(jo_audio_is_music_pending())wf_jo_audio_diagnostic.music_pending_ms=jo_get_ticks()-music_requested_ms;
        else if(jo_audio_is_music_playing()){
            if(!music_was_playing)maximum(&wf_jo_audio_diagnostic.music_start_max_delay_ms,
                                         jo_audio_get_music_start_ticks()-music_requested_ms);
            music_was_playing=1;wf_jo_audio_diagnostic.music_pending_ms=0;
        }else if(music_was_playing){
            if(music_command==5||music_command==8||music_command==9)++wf_jo_audio_diagnostic.music_natural_ends;
            else ++wf_jo_audio_diagnostic.music_exhaustions;
            music_command=music_was_playing=0;
        }
        maximum(&wf_jo_audio_diagnostic.music_underruns,jo_audio_get_music_underruns());
    }
    servicing=false;return initialized&&!emergency_stopped&&wf_jo_audio_diagnostic.fault==0;
}
bool wf_jo_audio_service_cooperate(void)
{
    if(!initialized||wf_jo_audio_diagnostic.fault)return false;
    if(servicing)return true;
    unsigned began=jo_get_ticks();
    if(queue_read==queue_write&&began-last_ms<4u)return true;
    bool result=wf_r18_scsp_service();
    ++wf_jo_audio_diagnostic.cooperative_services;
    maximum(&wf_jo_audio_diagnostic.cooperative_max_ms,jo_get_ticks()-began);
    return result;
}
bool wf_r18_scsp_boot(volatile uint32_t *clock,const void *elf,size_t bytes)
{(void)clock;(void)elf;(void)bytes;return initialized;}
bool wf_r18_scsp_smpc_owned(void) {return false;}
void wf_r18_scsp_vblank(void) { /* Jo already services its SCSP music in VBlank. */ }

#ifdef WF_JO_AUDIO_HOST
bool wf_jo_audio_test_decode_catalog(const void *data,size_t n,const void *expected,size_t bytes)
{
    const uint8_t *reference=expected;unsigned seen[116],count=0,at=0;
    if(!wf_jo_audio_pack_valid(data,n))return false;
    pack=data;sample_offset=be32(pack+12);decoder_tables();
    for(unsigned row=1;row<SAMPLE_ROWS;++row){
        if(row==74)continue;
        decoder_t d=decoder_start(row);unsigned i;
        for(i=0;i<count;++i)if(seen[i]==d.offset)break;
        if(i<count)continue;
        seen[count++]=d.offset;at=(at+31u)&~31u;
        if(at>bytes||d.frames*2u>bytes-at)return false;
        while(d.decoded<d.frames){
            uint16_t s=(uint16_t)decode_one(&d);
            if(reference[at++]!=(uint8_t)(s>>8)||reference[at++]!=(uint8_t)s)return false;
        }
    }
    return count==95u&&at==bytes;
}
#endif
