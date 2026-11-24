/* The real save adapter + real sprite-cache owner. Only backup I/O is fake.
 * It exercises the documented STN-52 maximum work footprint, not a claim that
 * the small internal-RAM save on R01 necessarily reaches that footprint. */
#include "wf_jo_save.h"
#include "wf_jo_assets.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FULL 565248u
#define REQUIRED (16384u+8312u)
static unsigned opens,closes,reads,writes;
static int opened;
static uint8_t *arena;
static uint8_t ranks[132];
static int open_io(void *u,void *p,size_t bytes) {
 (void)u;assert(!opened&&bytes==WF_JO_SAVE_WORKSPACE_BYTES);
 assert(wf_jo_assets_sprite_cache_active_bytes()==FULL-bytes);
 assert(p==arena+32u+FULL-bytes);++opens;opened=1;
 /* This reaches 120 bytes beyond the old work area. The enclosing arena
  * deliberately has guard storage there, so old-source red is defined C. */
 memset(p,0x6d,REQUIRED);return 1;
}
static void close_io(void*u){(void)u;assert(opened);opened=0;++closes;}
static int find_io(void*u,const char*n,size_t*b){(void)u;(void)n;(void)b;assert(opened);return 0;}
static int read_io(void*u,const char*n,void*p,size_t b){(void)u;(void)n;(void)p;(void)b;++reads;return 0;}
static int write_io(void*u,const char*n,const void*p,size_t b){(void)u;(void)n;(void)p;(void)b;++writes;return 0;}
static int verify_io(void*u,const char*n,const void*p,size_t b){(void)u;(void)n;(void)p;(void)b;return 0;}
static const wf_jo_save_backend_t io={open_io,close_io,find_io,read_io,write_io,verify_io};
static uint8_t *read_file(const char *name,size_t *n) {
 FILE*f=fopen(name,"rb");assert(f&&!fseek(f,0,SEEK_END));*n=(size_t)ftell(f);rewind(f);
 uint8_t*p=malloc(*n);assert(p&&fread(p,1,*n,f)==*n);fclose(f);return p;
}
int main(int ac,char**av) {
 assert(ac==4);size_t rn,pn,sn;uint8_t*rom=read_file(av[1],&rn),*pack=read_file(av[2],&pn),*raw=read_file(av[3],&sn);
 unsigned block=(unsigned)pack[8]<<24|(unsigned)pack[9]<<16|(unsigned)pack[10]<<8|pack[11];
 assert(sn==8388608u&&(block==4096u||block==8192u));
 unsigned tiles_per_block=block/128u;
 arena=malloc(FULL+256u);assert(arena);memset(arena,0xa5,FULL+256u);
 uint8_t*cache=arena+32u;assert(!wf_jo_assets_bind(rom,rn,pack,pn,cache,FULL));
 const uint8_t*p;for(unsigned i=0;i<FULL/block;i++)assert(!wf_jo_assets_tile_read(i*tiles_per_block,&p));
 uint8_t *before=malloc(FULL);assert(before);memcpy(before,cache,FULL);
 void*lease=NULL;assert(!wf_jo_assets_borrow_sprite_cache_tail(WF_JO_SAVE_WORKSPACE_BYTES,&lease));
 wf_jo_save_t save;wf_jo_save_init(&save,&io,NULL);
 for(unsigned i=0;i<sizeof ranks;i++)ranks[i]=(uint8_t)i;
 /* Complete too-short range and misalignment reject before open. */
 for(size_t n=0;n<WF_JO_SAVE_WORKSPACE_BYTES;n++)
  assert(wf_jo_save_load(&save,ranks,lease,n)==WF_JO_SAVE_UNAVAILABLE);
 assert(wf_jo_save_load(&save,ranks,(uint8_t*)lease+1u,WF_JO_SAVE_WORKSPACE_BYTES)==WF_JO_SAVE_UNAVAILABLE&&!opens);
 assert(wf_jo_save_load(&save,ranks,lease,WF_JO_SAVE_WORKSPACE_BYTES)==WF_JO_SAVE_DEFAULTS);
 assert(opens==1&&closes==1&&!opened&&!reads&&!writes);
 for(unsigned i=0;i<sizeof ranks;i++)assert(ranks[i]==(uint8_t)i);
 assert(!memcmp(cache,before,FULL-WF_JO_SAVE_WORKSPACE_BYTES));
 for(unsigned i=0;i<32u;i++)assert(arena[i]==0xa5);
 for(unsigned i=0;i<224u;i++)if(arena[32u+FULL+i]!=0xa5){
  fprintf(stderr,"FAIL STN-52: BIOS work footprint corrupts following RAM at +%u\n",i);return 2;
 }
 assert(!wf_jo_assets_return_sprite_cache_tail(lease));
 assert(wf_jo_assets_sprite_cache_active_bytes()==FULL);
 wf_jo_asset_stats_t first,last;wf_jo_assets_get_stats(&first);
 for(unsigned i=0;i<FULL/block;i++){
  assert(!wf_jo_assets_tile_read(i*tiles_per_block,&p));assert(!memcmp(p,raw+i*block,128u));
 }
 wf_jo_assets_get_stats(&last);
 assert(last.block_misses-first.block_misses==WF_JO_SAVE_WORKSPACE_BYTES/block);
 assert(!wf_jo_save_needs_store(&save,ranks));
 assert(wf_jo_save_store(&save,ranks,NULL,0)==WF_JO_SAVE_DEFAULTS&&!writes&&opens==1);
 free(before);free(arena);free(raw);free(pack);free(rom);
 printf("PASS STN-52: block=%u lease=%u, full work footprint, adjacent RAM guards, all short sizes, read-only load, exact cache invalidation/restoration\n",block,WF_JO_SAVE_WORKSPACE_BYTES);
}
