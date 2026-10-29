/* Diagnostic build only: decorate the real parser/transport without replacing
 * any gameplay, decoded record, command or texture. Production builds use the
 * original translation unit directly. Timing regions overlap intentionally. */
#include <yaul.h>
#include <string.h>
#include "wf_r18_present.h"
extern void wf_r18_profile_stage(unsigned,uint16_t,uint32_t);
static wf_status_t measured_meta(wf_r18_metasprite_ctx_t*,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);
static void *measured_clear(void*,int,size_t);
static const wf_saturn_render_ops_t *measured_ops(void);
static wf_status_t measured_tile(wf_saturn_yaul_backend_t*,uint16_t,uint16_t,wf_status_t(*)(uint16_t,const uint8_t**));
#define wf_r18_metasprite_emit measured_meta
#define memset measured_clear
#define wf_saturn_yaul_render_ops measured_ops
#define wf_saturn_yaul_upload_rom_tile measured_tile
#include "../../src/wf_r18_present.c"
#undef wf_r18_metasprite_emit
#undef memset
#undef wf_saturn_yaul_render_ops
#undef wf_saturn_yaul_upload_rom_tile
static void mark(unsigned i,uint16_t t,uint32_t n){wf_r18_profile_stage(i,t,n);}
static wf_status_t measured_read(void*u,uint32_t a,void*d,size_t n){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_r18_rom_read(u,a,d,n);mark(16,t,(uint32_t)n);return s;}
static wf_status_t measured_meta(wf_r18_metasprite_ctx_t*c,uint16_t s,uint16_t p,uint16_t x,uint16_t y,uint16_t partner){
 uint16_t t=cpu_frt_count_get();wf_r18_rom_read_fn previous=c->read_rom;c->read_rom=measured_read;
 wf_status_t status=wf_r18_metasprite_emit(c,s,p,x,y,partner);c->read_rom=previous;mark(9,t,1);return status;
}
static void *measured_clear(void*d,int v,size_t n){uint16_t t=cpu_frt_count_get();void*p=memset(d,v,n);mark(10,t,(uint32_t)n);return p;}
static wf_status_t measured_tile(wf_saturn_yaul_backend_t*b,uint16_t slot,uint16_t id,wf_status_t(*read)(uint16_t,const uint8_t**)){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_saturn_yaul_upload_rom_tile(b,slot,id,read);mark(12,t,1);return s;}
static wf_status_t measured_begin(void*c,unsigned nt,unsigned np,unsigned nc){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_saturn_yaul_render_ops()->begin_frame(c,nt,np,nc);mark(11,t,1);return s;}
static wf_status_t measured_pal(void*c,uint8_t b,const uint8_t*p,size_t n){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_saturn_yaul_render_ops()->upload_palette_xbgr444_16(c,b,p,n);mark(13,t,1);return s;}
static wf_status_t measured_cmd(void*c,const wf_vdp1_sprite_cmd_t*p){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_saturn_yaul_render_ops()->emit_sprite_16x16(c,p);mark(14,t,1);return s;}
static wf_status_t measured_end(void*c){uint16_t t=cpu_frt_count_get();wf_status_t s=wf_saturn_yaul_render_ops()->end_frame(c);mark(15,t,1);return s;}
static const wf_saturn_render_ops_t *measured_ops(void){
 static wf_saturn_render_ops_t ops;ops=*wf_saturn_yaul_render_ops();ops.begin_frame=measured_begin;ops.upload_palette_xbgr444_16=measured_pal;ops.emit_sprite_16x16=measured_cmd;ops.end_frame=measured_end;return &ops;
}
