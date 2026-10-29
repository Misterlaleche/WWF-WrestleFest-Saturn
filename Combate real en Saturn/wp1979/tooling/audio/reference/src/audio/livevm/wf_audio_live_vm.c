#include "wf_audio_live_vm.h"
#include <string.h>

static const int oki_num[16]={32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0};
static uint64_t now(wf_audio_live_vm_t *v){ return (uint64_t)v->cpu.cyc; }
static void schedule_tb(wf_audio_live_vm_t *v){ uint64_t p=(uint64_t)(256u-v->ym_tb)*1024u; if(!p)p=1024u; v->next_tb=now(v)+p; }
static void update_timer(wf_audio_live_vm_t *v){
    if((v->ym_ctl&2u) && now(v)>=v->next_tb){
        uint64_t p=(uint64_t)(256u-v->ym_tb)*1024u; if(!p)p=1024u;
        while(now(v)>=v->next_tb)v->next_tb+=p;
        v->ym_status|=2u;
        if(v->ym_ctl&8u)z80_gen_int(&v->cpu,0xff);
    }
}
static void oki_write(wf_audio_live_vm_t *v,uint8_t data){
    unsigned i;
    if(v->pending>=0){
        unsigned mask=(data>>4)&15u,vol=data&15u,sid=(unsigned)v->pending;
        for(i=0;i<4;i++)if(mask&(1u<<i)){
            if(!v->voices[i].playing && sid<128u && v->bank0[sid].valid){
                v->voices[i].playing=1;v->voices[i].sample_id=(uint8_t)sid;v->voices[i].pos=0;
                v->voices[i].bytes=v->bank0[sid].bytes;v->voices[i].offset=v->bank0[sid].offset;
                v->voices[i].volume=(uint8_t)vol;v->voices[i].current=0;
            }
        }
        v->pending=-1;return;
    }
    if(data&0x80u){v->pending=(int)(data&0x7fu);return;}
    { unsigned mask=data>>3; for(i=0;i<4;i++)if(mask&(1u<<i)){v->voices[i].playing=0;v->voices[i].current=0;} }
}
static void oki_tick(wf_audio_live_vm_t *v){
    unsigned i;
    for(i=0;i<4;i++)if(v->voices[i].playing){
        wf_livevm_voice_t *q=&v->voices[i];
        if(q->pos>=q->bytes || (uint64_t)q->offset+q->pos>=v->audpcm_bytes){q->playing=0;q->current=0;continue;}
        q->current=(int8_t)v->audpcm[q->offset+q->pos++]; if(q->pos>=q->bytes)q->playing=0;
    }
}
static int32_t oki_mix(wf_audio_live_vm_t *v){ int32_t sum=0; unsigned i; for(i=0;i<4;i++)if(v->voices[i].playing){int s=(int)v->voices[i].current; sum+=(s*256*oki_num[v->voices[i].volume&15u])/32;} return sum; }
static int16_t clip16(int64_t x){ if(x>32767)return 32767; if(x<-32768)return -32768; return (int16_t)x; }

static void synth_cycles(wf_audio_live_vm_t *v,uint64_t target,uint8_t *dst,uint32_t want,uint32_t *made);
static void sync_now(wf_audio_live_vm_t *v){ synth_cycles(v,now(v),v->render_dst,v->render_want,&v->render_made); }

static uint8_t rb(void*u,uint16_t a){ wf_audio_live_vm_t*v=u; if(a<0xc000u)return a<v->sound_rom_bytes?v->sound_rom[a]:0xffu; if(a<0xc800u)return v->ram[a-0xc000u]; if(a==0xc800u||a==0xc801u)return v->ym_status; if(a==0xd800u)return 0; if(a==0xe000u)return v->latch; return 0xffu; }
static void wb(void*u,uint16_t a,uint8_t val){ wf_audio_live_vm_t*v=u;
    if(a>=0xc000u&&a<0xc800u){v->ram[a-0xc000u]=val;return;}
    if(a==0xc800u){sync_now(v);v->ym_addr=val; OPM_Write(&v->opm,0,val); return;}
    if(a==0xc801u){uint8_t r=v->ym_addr; sync_now(v);OPM_Write(&v->opm,1,val); if(r==0x12u)v->ym_tb=val; if(r==0x14u){uint8_t old=v->ym_ctl;v->ym_ctl=val;if(val&0x20u)v->ym_status&=(uint8_t)~2u;if((val&2u)&&!(old&2u))schedule_tb(v);}return;}
    if(a==0xd800u){sync_now(v);oki_write(v,val);return;}
    if(a==0xe800u){sync_now(v);v->bank=val&1u;if(v->bank!=0u)v->error=1u;return;}
}
static uint8_t pin(z80*z,uint8_t p){(void)z;(void)p;return 0xffu;} static void pout(z80*z,uint8_t p,uint8_t val){(void)z;(void)p;(void)val;}

static void synth_cycles(wf_audio_live_vm_t *v,uint64_t target,uint8_t *dst,uint32_t want,uint32_t *made){
    while(v->rendered_cycle<target && *made<want){
        OPM_Clock(&v->opm,v->y,0,0,0);
        v->oki_phase+=WF_LIVEVM_OKI_HZ; if(v->oki_phase>=WF_LIVEVM_MASTER_HZ){v->oki_phase-=WF_LIVEVM_MASTER_HZ;oki_tick(v);}
        if(v->rendered_cycle>=v->origin_cycle){
            v->out_phase+=WF_LIVEVM_OUT_HZ;
            if(v->out_phase>=WF_LIVEVM_MASTER_HZ){
                int32_t o=oki_mix(v); int64_t m=(int64_t)v->y[0]*10813+(int64_t)v->y[1]*10813+(int64_t)o*21627; int16_t s=clip16(m>>15);
                if(dst){dst[*made*2u]=(uint8_t)((uint16_t)s>>8); dst[*made*2u+1u]=(uint8_t)s;} (*made)++;v->samples_total++;v->out_phase-=WF_LIVEVM_MASTER_HZ;
            }
        }
        v->rendered_cycle++;
    }
}

bool wf_audio_live_vm_init(wf_audio_live_vm_t *v,const uint8_t *sound_rom,size_t sound_rom_bytes,const uint8_t *audpcm,size_t audpcm_bytes,const wf_livevm_asset_t bank0[128]){
    uint64_t boot_end;
    if(!v||!sound_rom||sound_rom_bytes<0xc000u||!audpcm||!audpcm_bytes||!bank0)return false;
    memset(v,0,sizeof(*v)); v->sound_rom=sound_rom;v->sound_rom_bytes=sound_rom_bytes;v->audpcm=audpcm;v->audpcm_bytes=audpcm_bytes;memcpy(v->bank0,bank0,sizeof(v->bank0));v->pending=-1;
    z80_init(&v->cpu);v->cpu.read_byte=rb;v->cpu.write_byte=wb;v->cpu.port_in=pin;v->cpu.port_out=pout;v->cpu.userdata=v;OPM_Reset(&v->opm,opm_flags_none);
    boot_end=2ull*WF_LIVEVM_MASTER_HZ;v->origin_cycle=UINT64_MAX;
    v->render_dst=0;v->render_want=UINT32_MAX;v->render_made=0;while(now(v)<boot_end && !v->error){uint64_t start=now(v);z80_step(&v->cpu);update_timer(v);sync_now(v);if(now(v)<=start){v->error=1;break;}}v->render_want=0;v->render_made=0;
    return !v->error;
}
bool wf_audio_live_vm_start(wf_audio_live_vm_t *v,uint8_t command){ if(!v||command<1u||command>0x12u||v->error)return false; v->latch=command;v->origin_cycle=now(v);v->out_phase=0;v->samples_total=0;v->started=1;z80_gen_nmi(&v->cpu);return true; }
bool wf_audio_live_vm_render_be(wf_audio_live_vm_t *v,uint8_t *dst,uint32_t samples){ if(!v||!dst||!samples||!v->started||v->error)return false; v->render_dst=dst;v->render_want=samples;v->render_made=0; while(v->render_made<samples&&!v->error){uint64_t start=now(v);z80_step(&v->cpu);update_timer(v);sync_now(v);if(now(v)<=start){v->error=1;break;}} v->render_dst=0;v->render_want=0;return v->render_made==samples&&!v->error; }
