#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define DECL(P) \
int P##wf_r18_ring_bind(const void*,size_t); \
int P##wf_r18_ring_tile_write(unsigned,unsigned,uint16_t,uint16_t); \
void P##wf_r18_ring_tile_restore(unsigned,unsigned); \
int P##wf_r18_ring_cells_update(unsigned,int,int,volatile uint16_t*,size_t*); \
int P##wf_r18_ring_pixel(unsigned,int,int,uint8_t*); \
int P##wf_r18_ring_surface_update(unsigned,int,int,unsigned,volatile uint8_t*,size_t*);
DECL(old_)
DECL(new_)
static uint32_t random_state=0x1979cafeu;
static uint32_t next(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static unsigned be16(const uint8_t*p){return ((unsigned)p[0]<<8)|p[1];}
static unsigned be32(const uint8_t*p){return ((unsigned)p[0]<<24)|((unsigned)p[1]<<16)|((unsigned)p[2]<<8)|p[3];}
static void put16(uint8_t*p,unsigned x){p[0]=(uint8_t)(x>>8);p[1]=(uint8_t)x;}
static unsigned checks,lookups,raster_pixels,views,binds;
static void bind_pair(const uint8_t *p,size_t n,int expected){assert(old_wf_r18_ring_bind(p,n)==expected);assert(new_wf_r18_ring_bind(p,n)==expected);binds++;}
static void lookups_all(const uint8_t *p){
 unsigned count=be16(p+10),base=be32(p+16);const uint8_t*ids=p+base+count*128u;
 unsigned index=0;
 for(unsigned source=0;source<65536u;source++){
  while(index<count&&be16(ids+index*2u)<source)index++;
  int expected=index<count&&be16(ids+index*2u)==source?0:-1;
  unsigned layer=(source>>10)&1u,cell=source&1023u;
  uint16_t attr=(uint16_t)((source&15u)|((source&3u)<<6));
  int a=old_wf_r18_ring_tile_write(layer,cell,attr,(uint16_t)source);
  int b=new_wf_r18_ring_tile_write(layer,cell,attr,(uint16_t)source);
  assert(a==expected&&b==expected);lookups++;
 }
}
static void map_views(const uint8_t*p,unsigned rounds){
 uint16_t *a=calloc(2u*1024u,sizeof *a),*b=calloc(2u*1024u,sizeof *b);assert(a&&b);
 unsigned count=be16(p+10),base=be32(p+16);const uint8_t*ids=p+base+count*128u;
 for(unsigned view=0;view<rounds;view++){
  for(unsigned j=0;j<1u+(next()%128u);j++){
   unsigned layer=next()&1u,cell=next()&1023u;
   if((next()&7u)==0){old_wf_r18_ring_tile_restore(layer,cell);new_wf_r18_ring_tile_restore(layer,cell);}
   else{unsigned i=next()%count;uint16_t source=(uint16_t)be16(ids+i*2u),attr=(uint16_t)(next()&255u);assert(old_wf_r18_ring_tile_write(layer,cell,attr,source)==0);assert(new_wf_r18_ring_tile_write(layer,cell,attr,source)==0);}
  }
  int wx=(int)(next()%10240u)-5120,wy=(int)(next()%8192u)-4096;
  if(view&&view%4u==0){wx=37;wy=81;}
  for(unsigned layer=0;layer<2;layer++){
   size_t x=0,y=0;assert(!old_wf_r18_ring_cells_update(layer,wx,wy,a+layer*1024u,&x));assert(!new_wf_r18_ring_cells_update(layer,wx,wy,b+layer*1024u,&y));assert(x==y);assert(!memcmp(a,b,2u*1024u*sizeof *a));checks++;
   /* Independent scalar pixel path checks visible cells, palette and flips. */
   for(unsigned py=0;py<240u;py+=7u)for(unsigned px=0;px<320u;px+=5u){
    int xw=(wx+(int)px)%2560,yw=(wy+(int)py)%2048;if(xw<0)xw+=2560;if(yw<0)yw+=2048;
    unsigned cell=((unsigned)yw>>4&31u)*32u+((unsigned)xw>>4&31u),name=b[layer*1024u+cell];
    unsigned xx=(unsigned)xw&15u,yy=(unsigned)yw&15u;if(name&0x400u)xx=15u-xx;if(name&0x800u)yy=15u-yy;
    uint8_t packed=p[base+(name&1023u)*128u+yy*8u+xx/2u];unsigned nibble=xx&1u?packed&15u:packed>>4;
    uint8_t actual=(uint8_t)(((name>>12)<<4)|nibble),old_pixel=0,new_pixel=0;
    assert(!old_wf_r18_ring_pixel(layer,xw,yw,&old_pixel));assert(!new_wf_r18_ring_pixel(layer,xw,yw,&new_pixel));assert(actual==old_pixel&&actual==new_pixel);raster_pixels++;
   }
   /* Repeated coordinates and no dirty cells must retain identical writes. */
   assert(!old_wf_r18_ring_cells_update(layer,wx,wy,a+layer*1024u,&x));assert(!new_wf_r18_ring_cells_update(layer,wx,wy,b+layer*1024u,&y));assert(x==0&&y==0);assert(!memcmp(a,b,2u*1024u*sizeof *a));checks++;
  }
  views++;
 }
 free(a);free(b);
}
static void bitmap_views(const uint8_t*p,size_t n,unsigned rounds){
 bind_pair(p,n,0);
 uint8_t *a=calloc(2u*512u*256u,1),*b=calloc(2u*512u*256u,1);assert(a&&b);
 unsigned count=be16(p+10),base=be32(p+16);const uint8_t*ids=p+base+count*128u;
 for(unsigned view=0;view<rounds;view++){
  int wx=(int)(next()%5120u)-2560,wy=(int)(next()%4096u)-2048;
  for(unsigned j=0;j<32u;j++){unsigned layer=next()&1u,cell=next()&1023u,index=next()%count;uint16_t attr=(uint16_t)next();assert(!old_wf_r18_ring_tile_write(layer,cell,attr,(uint16_t)be16(ids+index*2u)));assert(!new_wf_r18_ring_tile_write(layer,cell,attr,(uint16_t)be16(ids+index*2u)));}
  for(unsigned layer=0;layer<2;layer++){
   size_t x=0,y=0;unsigned tr=view&1u;
   assert(!old_wf_r18_ring_surface_update(layer,wx,wy,tr,a+layer*512u*256u,&x));assert(!new_wf_r18_ring_surface_update(layer,wx,wy,tr,b+layer*512u*256u,&y));assert(x==y);assert(!memcmp(a,b,2u*512u*256u));checks++;
  }
 }
 free(a);free(b);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long length=ftell(f);assert(length>0);rewind(f);size_t n=(size_t)length;uint8_t*p=malloc(n),*q=malloc(n);assert(p&&q);assert(fread(p,1,n,f)==n);fclose(f);memcpy(q,p,n);
 unsigned count=be16(p+10),base=be32(p+16);assert(count>2u);uint8_t*ids=q+base+count*128u;
 bind_pair(p,n,0);lookups_all(p);map_views(p,240u);bitmap_views(p,n,24u);
 /* A second source has IDs spread across the full public uint16 domain. */
 for(unsigned i=0;i<count;i++)put16(ids+i*2u,(unsigned)((uint64_t)i*65535u/(count-1u)));
 bind_pair(q,n,0);lookups_all(q);map_views(q,120u);bitmap_views(q,n,12u);
 /* Rebinding the original source must discard the second translation. */
 bind_pair(p,n,0);lookups_all(p);map_views(p,60u);
 /* Duplicate/out-of-order IDs fail bind, with g invalid until a valid bind. */
 put16(ids+2u,be16(ids));bind_pair(q,n,-1);assert(old_wf_r18_ring_tile_write(0,0,0,0)==-1);assert(new_wf_r18_ring_tile_write(0,0,0,0)==-1);
 memcpy(q,p,n);ids=q+base+count*128u;put16(ids,65535u);bind_pair(q,n,-1);
 bind_pair(p,n,0);lookups_all(p);
 assert(old_wf_r18_ring_tile_write(2,0,0,0)==-1&&new_wf_r18_ring_tile_write(2,0,0,0)==-1);
 assert(old_wf_r18_ring_tile_write(0,1024,0,0)==-1&&new_wf_r18_ring_tile_write(0,1024,0,0)==-1);
 bind_pair(NULL,n,-1);bind_pair(p,n-1u,-1);bind_pair(p,n,0);
 printf("PASS binds=%u source_id_probes=%u map_or_bitmap_checks=%u views=%u independent_raster_pixels=%u\n",binds,lookups,checks,views,raster_pixels);
 free(q);free(p);return 0;
}
