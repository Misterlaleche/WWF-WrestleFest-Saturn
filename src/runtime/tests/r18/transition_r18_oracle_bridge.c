#include "wf_r18_transition.h"
#include "wf_r17_dependencies.h"
#include "wf_gameplay_bindings.h"
#include "wf_r18_ring.h"
#include <stdio.h>
#include <string.h>
static const char *directory;
static uint32_t b(const uint8_t*p,unsigned n){uint32_t v=0;while(n--)v=(v<<8)|*p++;return v;}
static void w(uint8_t*p,uint32_t v,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8*(n-i-1)));}
static int size(const char*n,uint32_t*out){char p[2048];snprintf(p,sizeof p,"%s/%s",directory,n);FILE*f=fopen(p,"rb");if(!f)return -1;fseek(f,0,SEEK_END);*out=(uint32_t)ftell(f);fclose(f);return 0;}
static int file(const char*n,uint32_t at,void*d,size_t count){char p[2048];snprintf(p,sizeof p,"%s/%s",directory,n);FILE*f=fopen(p,"rb");if(!f)return -1;fseek(f,(long)at,SEEK_SET);int ok=fread(d,1,count,f)==count;fclose(f);return ok?0:-1;}
static wf_status_t read_rom(void*u,uint32_t at,void*d,size_t n){if(at>0x80000u||n>0x80000u-at)return WF_BLOCKED;memcpy(d,(uint8_t*)u+at,n);return WF_OK;}
static wf_status_t audio(wf_runtime_t*r,uint16_t cmd,void*u){(void)r;(void)u;return cmd==0x80||cmd==0x20?WF_OK:WF_BAD_STATE;}
int transition_r18_oracle(uint8_t*mem,uint32_t*regs,unsigned op,const char*assets){
 wf_runtime_t r={0};wf_gameplay_binding_context_t bindings={0};wf_r18_video_t video={0};wf_r18_world_ctx_t world={0};wf_r18_display_t display={0};uint8_t pack[0x1f000],priority=0;
 wf_r18_transition_context_t c={.video=&video,.world=&world,.display=&display,.camera=&bindings.camera,.render_queue=&bindings.render_queue,.scene_blob=pack,.scene_capacity=sizeof pack,.file_size=size,.file_read=file,.priority_140010=&priority};
 directory=assets;c.sprite_control_140008=(uint16_t)b(mem+0x140008,2);for(unsigned l=0;l<WF_R17_RENDER_LISTS;l++){bindings.render_queue.count[l]=1;bindings.render_queue.list[l][0]=&r.actors[l];}for(unsigned i=0;i<4;i++)display.scroll[i]=(uint16_t)b(mem+0x100000+i*2,2);uint8_t*g=mem+0x1c0000;memcpy(r.m68k_regs.d,regs,32);r.m68k_regs.valid=1;
 video.read_rom=read_rom;video.read_user=mem;world.read_rom=read_rom;world.read_user=mem;
 memcpy(video.fg0,mem+0xc0000,sizeof video.fg0);memcpy(video.background,mem+0x80000,sizeof video.background);memcpy(video.palettes,mem+0x180000,sizeof video.palettes);memcpy(video.fg_palette,mem+0x188000,sizeof video.fg_palette);memcpy(video.bg_palette,mem+0x186000,sizeof video.bg_palette);
 memcpy(r.actors,g+0x5b0,sizeof r.actors);memcpy(r.fixed_object0.b,g+0xf1c,0x10c);memcpy(r.fixed_object1.b,g+0x1028,0x10c);memcpy(r.fixed_four_1134.b,g+0x1134,0x30);memcpy(r.referee_aux.b,g+0x1164,0x30);memcpy(r.fixed_four_1194.b,g+0x1194,0x30);memcpy(r.fixed_four_11c4.b,g+0x11c4,0x30);memcpy(r.referee.b,g+0x11f4,0x64);
 memcpy(r.g.transfer_queue_1808,g+0x1808,16);memcpy(r.branchb_list_182a,g+0x182a,48);memcpy(r.branchb_prev_185a,g+0x185a,48);memcpy(r.branchb_objs_188a,g+0x188a,192);
 memcpy(world.animation_queue[0],g+0x194a,48);memcpy(world.animation_queue[1],g+0x197a,48);world.animation_count[0]=(uint16_t)b(g+0x19ac,2);world.animation_count[1]=(uint16_t)b(g+0x19aa,2);
 for(unsigned l=0;l<2;l++)for(unsigned i=0;i<16;i++){world.cache[l][i].source=b(g+(l?0x1756:0x16d6)+i*8,4);world.cache[l][i].destination=b(g+(l?0x1756:0x16d6)+i*8+4,4);}
#define G(at,field,n) r.g.field=b(g+at,n)
 G(0x7e,global_007e,2);
 G(0x7f,global_007f,1);
 G(0x76,gameplay_vblank_enable,1);
 G(0x7c,credited_route,2);
 G(0x161,mode_0161,1);
 G(0x162,progression_0162,2);
 G(0x1670,global_1670,4);
 G(0x1674,global_1674,4);
 G(0x1678,global_1678,1);
 G(0x1679,global_1679,1);
 G(0x15f4,global_15f4,2);
 G(0x15f8,global_15f8,2);
 G(0x16d2,global_16d2,2);
 G(0x16d4,global_16d4,2);
 G(0x16e6,global_16e6,1);
 G(0x17d6,global_17d6,2);
 G(0x17d8,global_17d8,2);
 G(0x17da,global_17da,2);
 G(0x17dc,global_17dc,2);
 G(0x17de,global_17de,2);
 G(0x17e0,global_17e0,2);
 G(0x17e2,global_17e2,2);
 G(0x17e4,global_17e4,2);
 G(0x17e6,global_17e6,2);
 G(0x17ea,global_17ea,2);
 G(0x17ee,global_17ee,2);
 G(0x17f2,global_17f2_word,2);
 G(0x17f6,global_17f6,2);
 G(0x17fa,global_17fa,2);
 G(0x17fe,global_17fe,2);
 G(0x1800,global_1800,2);
 G(0x1802,global_1802,2);
 G(0x1804,global_1804,2);
 G(0x1806,global_1806,2);
 G(0x1828,transfer_count_1828,2);
 G(0x15fc,global_15fc,2);
 G(0x6f,frame_handshake[1],1);
 G(0x15d4,global_15d4,1);
 G(0x15d5,global_15d5,1);
#undef G
 wf_r18_transition_bind(&c);bindings.transition_reset=wf_r18_transition_reset;bindings.transition_barrier=wf_r18_transition_barrier;bindings.r17_owner_user=&video;bindings.audio_command=audio;
 wf_status_t status=op==1?wf_r18_transition_reset(&r,0,&video):op==2?wf_r18_transition_barrier(&r,0,&video):wf_r17_f98c(&r,&bindings);
 if(status==WF_OK&&op==3){r.g.global_1678=0xc0;status=wf_r17_f98c(&r,&bindings);}
 if(status==WF_OK&&op!=1&&(bindings.camera.scene_index!=r.g.global_007e||bindings.camera.scroll0_x!=(int16_t)r.g.global_17e6||bindings.camera.scroll0_y!=(int16_t)r.g.global_17ee||bindings.camera.scroll1_x!=(int16_t)r.g.global_17ea||bindings.camera.scroll1_y!=(int16_t)r.g.global_17f2_word||bindings.camera.target_x!=(int16_t)r.g.global_1804||bindings.camera.target_y!=(int16_t)r.g.global_1806))return -0x26936;
 if(status==WF_OK&&op!=2)for(unsigned l=0;l<WF_R17_RENDER_LISTS;l++)if(bindings.render_queue.count[l]||bindings.render_queue.list[l][0])return -0x1fde;
 if(status==WF_OK&&op==1){uint8_t pixel;if(wf_r18_ring_pixel(0,0,0,&pixel)!=-1||video.ring_cell)return -0x1f6c;}
 memcpy(regs,r.m68k_regs.d,32);memcpy(g+0x5b0,r.actors,sizeof r.actors);memcpy(g+0xf1c,r.fixed_object0.b,0x10c);memcpy(g+0x1028,r.fixed_object1.b,0x10c);memcpy(g+0x1134,r.fixed_four_1134.b,0x30);memcpy(g+0x1164,r.referee_aux.b,0x30);memcpy(g+0x1194,r.fixed_four_1194.b,0x30);memcpy(g+0x11c4,r.fixed_four_11c4.b,0x30);memcpy(g+0x11f4,r.referee.b,0x64);
 memcpy(g+0x1808,r.g.transfer_queue_1808,16);memcpy(g+0x182a,r.branchb_list_182a,48);memcpy(g+0x185a,r.branchb_prev_185a,48);memcpy(g+0x188a,r.branchb_objs_188a,192);
 memcpy(g+0x194a,world.animation_queue[0],48);memcpy(g+0x197a,world.animation_queue[1],48);w(g+0x19ac,world.animation_count[0],2);w(g+0x19aa,world.animation_count[1],2);
 for(unsigned l=0;l<2;l++)for(unsigned i=0;i<16;i++){w(g+(l?0x1756:0x16d6)+i*8,world.cache[l][i].source,4);w(g+(l?0x1756:0x16d6)+i*8+4,world.cache[l][i].destination,4);}
 memcpy(mem+0xc0000,video.fg0,sizeof video.fg0);memcpy(mem+0x80000,video.background,sizeof video.background);memcpy(mem+0x180000,video.palettes,sizeof video.palettes);memcpy(mem+0x188000,video.fg_palette,sizeof video.fg_palette);memcpy(mem+0x186000,video.bg_palette,sizeof video.bg_palette);
 if(op!=1){w(mem+0x140010,priority,2);for(unsigned i=0;i<4;i++)w(mem+0x100000+i*2,display.scroll[i],2);}
 w(mem+0x140008,c.sprite_control_140008,2);
#define G(at,field,n) w(g+at,r.g.field,n)
 G(0x7e,global_007e,2);
 G(0x7f,global_007f,1);
 G(0x76,gameplay_vblank_enable,1);
 G(0x7c,credited_route,2);
 G(0x161,mode_0161,1);
 G(0x162,progression_0162,2);
 G(0x1670,global_1670,4);
 G(0x1674,global_1674,4);
 G(0x1678,global_1678,1);
 G(0x1679,global_1679,1);
 G(0x15f4,global_15f4,2);
 G(0x15f8,global_15f8,2);
 G(0x16d2,global_16d2,2);
 G(0x16d4,global_16d4,2);
 G(0x16e6,global_16e6,1);
 G(0x17d6,global_17d6,2);
 G(0x17d8,global_17d8,2);
 G(0x17da,global_17da,2);
 G(0x17dc,global_17dc,2);
 G(0x17de,global_17de,2);
 G(0x17e0,global_17e0,2);
 G(0x17e2,global_17e2,2);
 G(0x17e4,global_17e4,2);
 G(0x17e6,global_17e6,2);
 G(0x17ea,global_17ea,2);
 G(0x17ee,global_17ee,2);
 G(0x17f2,global_17f2_word,2);
 G(0x17f6,global_17f6,2);
 G(0x17fa,global_17fa,2);
 G(0x17fe,global_17fe,2);
 G(0x1800,global_1800,2);
 G(0x1802,global_1802,2);
 G(0x1804,global_1804,2);
 G(0x1806,global_1806,2);
 G(0x1828,transfer_count_1828,2);
 G(0x15fc,global_15fc,2);
 G(0x6f,frame_handshake[1],1);
 G(0x15d4,global_15d4,1);
 G(0x15d5,global_15d5,1);
#undef G
 return status?-(int)(c.blocked_pc?c.blocked_pc:status):0;
}
