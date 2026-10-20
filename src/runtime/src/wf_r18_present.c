#include "wf_r18_present.h"
#include "wf_renderer_contract.h"
#include <string.h>
static wf_r18_file_read_fn read_file;
static uint8_t *rom_data,*tile_data;static size_t rom_slots,tile_slots;
static uint32_t rom_tags[128];static uint16_t tile_tags[6144];static uint8_t tile_valid[6144];
static wf_r18_sprite_record_t records[512];static wf_vdp1_frame_plan_t plan;
static wf_actor_raw_t *composed[WF_ACTOR_SLOTS];static size_t composed_count;
static uint8_t dma_stage[128] __attribute__((aligned(32)));
static wf_saturn_yaul_backend_t backend;
static unsigned busy;
void wf_r18_present_init(wf_r18_file_read_fn fn,void *rc,size_t rn,void *tc,size_t tn){read_file=fn;rom_data=rc;tile_data=tc;rom_slots=rn/1024;tile_slots=tn/128;if(rom_slots>128)rom_slots=128;if(tile_slots>6144)tile_slots=6144;for(unsigned i=0;i<128;i++)rom_tags[i]=0xffffffffu;memset(tile_valid,0,sizeof tile_valid);wf_saturn_yaul_backend_init(&backend,dma_stage,sizeof dma_stage);}
wf_status_t wf_r18_rom_read(void*u,uint32_t a,void*dest,size_t n){uint8_t*out=dest;(void)u;if(!read_file||!rom_slots||a>0x80000u||n>0x80000u-a)return WF_BLOCKED;
 while(n){uint32_t page=a/1024;size_t slot=page%rom_slots,off=a%1024,take=1024-off;if(take>n)take=n;
 if(rom_tags[slot]!=page){if(read_file("ROM68K.BIN",page*1024,rom_data+slot*1024,1024))return WF_BLOCKED;rom_tags[slot]=page;}
 memcpy(out,rom_data+slot*1024+off,take);out+=take;a+=(uint32_t)take;n-=take;}
 return WF_OK;}
static wf_status_t get_tile(uint16_t tile,const uint8_t **out){size_t slot;if(!tile_slots)return WF_BLOCKED;slot=tile%tile_slots;if(!tile_valid[slot]||tile_tags[slot]!=tile){if(read_file("SPRITES.PAK",(uint32_t)tile*128,tile_data+slot*128,128))return WF_BLOCKED;tile_tags[slot]=tile;tile_valid[slot]=1;}*out=tile_data+slot*128;return WF_OK;}
static uint16_t be16(const uint8_t*p){return (uint16_t)((p[0]<<8)|p[1]);}
static wf_actor_raw_t *actor_link(wf_runtime_t*r,uint32_t address){for(unsigned i=0;i<WF_ACTOR_SLOTS;i++)if(address==wf_actor_rom_base(i))return &r->actors[i];return NULL;}
static int add_cmd(uint16_t tile,int x,int y,unsigned fx,unsigned fy,unsigned palette){unsigned slot;
 if(x>=320||x+16<=0||y>=240||y+16<=0)return 1;
 if(plan.cmd_count==WF_VDP1_COMMANDS_MAX)return 0;
 for(slot=0;slot<plan.tile_count;slot++)if(plan.tile_upload[slot].source_tile==tile)break;
 if(slot==plan.tile_count){if(slot==WF_VDP1_STREAM_TILES_MAX)return 0;plan.tile_upload[slot].source_tile=tile;plan.tile_upload[slot].stream_slot=(uint16_t)slot;plan.tile_count++;}
 wf_vdp1_sprite_cmd_t*c=&plan.cmd[plan.cmd_count++];memset(c,0,sizeof *c);c->x=(int16_t)x;c->y=(int16_t)y;c->stream_tile=(uint16_t)slot;c->cram_slot=(uint8_t)palette;c->flip_x=(uint8_t)fx;c->flip_y=(uint8_t)fy;return 1;}
wf_status_t wf_r18_present_batch(wf_runtime_t*r,wf_actor_raw_t*const*actors,size_t n,void*u){wf_r18_video_t*v=u;wf_r18_metasprite_ctx_t parser;wf_status_t status=WF_OK;const wf_saturn_render_ops_t*ops=wf_saturn_yaul_render_ops();
 if(!r||!v||(!actors&&n)||busy)return WF_BAD_STATE;busy=1;memset(&parser,0,sizeof parser);parser.read_rom=wf_r18_rom_read;parser.palette_map=v->palette_map;parser.records=records;parser.capacity=512;parser.d6=r->m68k_regs.d[6];parser.d7=r->m68k_regs.d[7];
 composed_count=0;
 for(size_t i=0;i<n;i++){wf_actor_raw_t*a=actors[i];wf_actor_raw_t*partner;if(wf_u8(a,0x21)==255||(wf_be16(a,4)&0x7fff)==0x7fff)continue;partner=actor_link(r,wf_be32(a,0x26));status=wf_r18_metasprite_emit(&parser,wf_be16(a,2),wf_be16(a,4),wf_be16(a,0x14),wf_be16(a,0x16),partner?wf_be16(partner,2):0xffff);if(status!=WF_OK)goto done;if(composed_count<WF_ACTOR_SLOTS)composed[composed_count++]=a;}
 memset(&plan,0,sizeof plan);
 for(size_t i=0;i<parser.count;i++){const uint8_t*b=records[i].bytes;unsigned attr=be16(b+2);if(!(attr&1))continue;unsigned chain=(attr>>5)&7,fx=(attr>>4)&1,fy=(attr>>3)&1,palette=b[9]&15;unsigned tile=wf_sprite_number_align((unsigned)b[5]|((unsigned)b[7]<<8),chain);int x=wf_sprite_x_decode((unsigned)b[11]|((attr&4)<<6)),y=wf_sprite_y_decode((unsigned)b[1]|((attr&2)<<7));
 for(unsigned j=0;j<=chain;j++){int yy=fy?y-(int)chain*16+(int)j*16:y-(int)j*16;if(!add_cmd((uint16_t)(tile+j),x,yy,fx,fy,palette)){status=WF_BLOCKED;goto done;}}}
 status=ops->begin_frame(&backend,plan.tile_count,16,plan.cmd_count);if(status!=WF_OK)goto done;
 for(unsigned i=0;i<plan.tile_count;i++){const uint8_t*p;status=get_tile(plan.tile_upload[i].source_tile,&p);if(status==WF_OK)status=ops->upload_tile_16x16_4bpp(&backend,(uint16_t)i,p,128);if(status!=WF_OK)goto abort;}
 for(unsigned i=0;i<16;i++){status=ops->upload_palette_xbgr444_16(&backend,(uint8_t)i,v->palettes+0x2000+i*128,32);if(status!=WF_OK)goto abort;}
 for(unsigned i=0;i<plan.cmd_count;i++){status=ops->emit_sprite_16x16(&backend,&plan.cmd[i]);if(status!=WF_OK)goto abort;}
 status=ops->end_frame(&backend);if(status!=WF_OK)goto done;
 /* $293E consumes a resolved link after rendering its composition. */
 for(size_t i=0;i<composed_count;i++)if((wf_u8(composed[i],0x26)&128)&&!wf_be16(composed[i],0xfe))wf_set_be32(composed[i],0x26,0);
 r->m68k_regs.d[6]=parser.d6;r->m68k_regs.d[7]=parser.d7;goto done;
abort:ops->abort_frame(&backend);
done:busy=0;return status;}
unsigned wf_r18_present_commands(void){return plan.cmd_count;}
