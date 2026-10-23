/* Exercise the production encoder, its real staged texture bytes and its
 * submitted 32-byte VDP1 commands. The independent reference always draws
 * the complete 16x16 source tile with transparent pen0. A wrong source-row
 * offset, flip origin, palette, residency decision or omitted penF must
 * change the framebuffer; retaining empty outer rows fails the work check.
 * This host interpreter is not a Saturn timing or emulator validation. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaul.h>
#include "wf_saturn_yaul_backend.h"
#include "../../src/wf_saturn_yaul_backend.c"

unsigned mock_sync_calls, mock_sprite_priorities[8], mock_screen_priorities[3];
uint8_t mock_vram[0x80000];
uint32_t mock_cram[1024];
static uint8_t stage[32768] __aligned(32);
static uint8_t palettes[16][32];
static uint32_t colors[16][16], reference[320u*240u], actual[320u*240u];
static vdp1_cmdt_t submitted[WF_VDP1_COMMANDS_MAX+4u];
static unsigned submitted_count, submissions, transfers, rom_reads;
static unsigned frames;
static uint64_t hardware_rows, full_rows;

typedef struct reference_sprite {
    wf_vdp1_sprite_cmd_t cmd;
    const uint8_t *pixels;
} reference_sprite_t;

void scu_dma_transfer(unsigned channel,void *dst,const void *src,size_t bytes)
{
    assert(channel==0);
    assert((const uint8_t *)src>=stage && (const uint8_t *)src+bytes<=stage+sizeof stage);
    memcpy(dst,src,bytes);
    ++transfers;
}
void scu_dma_transfer_wait(unsigned channel){assert(channel==0);}
void vdp1_sync_cmdt_list_put(vdp1_cmdt_list_t *list,unsigned offset)
{
    assert(offset==0 && list->count<=WF_VDP1_COMMANDS_MAX+4u);
    submitted_count=list->count;
    memcpy(submitted,list->cmdts,list->count*sizeof(*submitted));
    ++submissions;
}

static unsigned pen_at(const uint8_t *tile,unsigned x,unsigned y)
{
    unsigned byte=tile[y*8u+x/2u];
    return x&1u ? byte&15u : byte>>4;
}
static void clear_framebuffers(void)
{
    for(unsigned i=0;i<320u*240u;++i)
        actual[i]=reference[i]=0x1000000u+i;
}
static void draw_reference(const reference_sprite_t *sprite)
{
    const wf_vdp1_sprite_cmd_t *c=&sprite->cmd;
    for(unsigned y=0;y<16u;++y)for(unsigned x=0;x<16u;++x){
        int dx=(int)c->x+(int)x,dy=(int)c->y+(int)y;
        unsigned pen=pen_at(sprite->pixels,c->flip_x?15u-x:x,c->flip_y?15u-y:y);
        if(pen && dx>=0 && dx<320 && dy>=0 && dy<240)
            reference[(unsigned)dy*320u+(unsigned)dx]=colors[c->cram_slot][pen];
    }
}
static unsigned draw_commands(unsigned *sprites)
{
    unsigned rows=0;
    *sprites=0;
    assert(submitted_count>=3u);
    assert(submitted[0].cmd_ctrl==9u && submitted[0].cmd_xc==319 && submitted[0].cmd_yc==239);
    assert(submitted[1].cmd_ctrl==10u && submitted[1].cmd_xa==0 && submitted[1].cmd_ya==0);
    assert(submitted[submitted_count-1u].cmd_ctrl==0x8000u);
    for(unsigned i=2u;i+1u<submitted_count;++i){
        const vdp1_cmdt_t *c=&submitted[i];
        unsigned width=((c->cmd_size>>8)&63u)*8u,height=c->cmd_size&255u;
        size_t address=(size_t)c->cmd_srca*8u;
        assert((c->cmd_ctrl&~0x30u)==0 && c->cmd_link==0);
        assert(c->cmd_pmod==0x80u); /* transparent pen0, penF is not END */
        assert(width==16u && height>=1u && height<=16u);
        assert(address>=WF_SATURN_VDP1_DYNAMIC_OFFSET);
        assert(address+height*8u<=WF_SATURN_VDP1_DYNAMIC_END);
        assert((c->cmd_colr&15u)==0 && c->cmd_colr>=0x100u && c->cmd_colr<0x200u);
        assert(c->cmd_xb==0 && c->cmd_yb==0 && c->cmd_xc==0 && c->cmd_yc==0);
        assert(c->cmd_xd==0 && c->cmd_yd==0 && c->cmd_grda==0 && c->reserved==0);
        ++*sprites;rows+=height;
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){
            int dx=(int)c->cmd_xa+(int)x,dy=(int)c->cmd_ya+(int)y;
            unsigned sy=c->cmd_ctrl&0x20u?height-1u-y:y;
            unsigned sx=c->cmd_ctrl&0x10u?width-1u-x:x;
            unsigned byte=mock_vram[address+sy*(width/2u)+sx/2u];
            unsigned pen=sx&1u?byte&15u:byte>>4;
            if(pen && dx>=0 && dx<320 && dy>=0 && dy<240)
                actual[(unsigned)dy*320u+(unsigned)dx]=mock_cram[c->cmd_colr+pen];
        }
    }
    return rows;
}
static void begin(wf_saturn_yaul_backend_t *b,unsigned tiles,unsigned cmds)
{
    memset(g_cmdts,0x5a,sizeof g_cmdts);
    assert(wf_saturn_yaul_render_ops()->begin_frame(b,tiles,0,cmds)==WF_OK);
}
static void finish(wf_saturn_yaul_backend_t *b,const reference_sprite_t *sprites,
                   unsigned count,unsigned wanted_rows,unsigned wanted_sprites)
{
    unsigned hardware_sprites,previous=submissions;
    assert(b->emitted_cmds==count); /* Logical requests include all-pen0 tiles. */
    assert(wf_saturn_yaul_render_ops()->end_frame(b)==WF_OK);
    assert(submissions==previous+1u);
    clear_framebuffers();
    for(unsigned i=0;i<count;++i)draw_reference(&sprites[i]);
    unsigned rows=draw_commands(&hardware_sprites);
    if(memcmp(actual,reference,sizeof actual)){
        for(unsigned i=0;i<320u*240u;++i)if(actual[i]!=reference[i]){
            fprintf(stderr,"frame %u differs at %u,%u: actual=%08x reference=%08x\n",
                    frames,i%320u,i/320u,actual[i],reference[i]);
            break;
        }
        abort();
    }
    if(rows!=wanted_rows || hardware_sprites!=wanted_sprites){
        fprintf(stderr,"frame %u work: got %u rows/%u sprites, wanted %u/%u\n",
                frames,rows,hardware_sprites,wanted_rows,wanted_sprites);
        abort();
    }
    /* An old sprite/END beyond the submitted list is neither cleared nor read. */
    if(submitted_count<WF_VDP1_COMMANDS_MAX+4u)
        for(unsigned i=0;i<sizeof g_cmdts[0];++i)
            assert(((const uint8_t *)&g_cmdts[submitted_count])[i]==0x5a);
    hardware_rows+=rows;full_rows+=(uint64_t)count*16u;++frames;
}
static void upload(wf_saturn_yaul_backend_t *b,unsigned slot,const uint8_t *tile)
{
    assert(wf_saturn_yaul_render_ops()->upload_tile_16x16_4bpp(b,(uint16_t)slot,tile,128u)==WF_OK);
}
static void emit(wf_saturn_yaul_backend_t *b,const reference_sprite_t *sprite)
{
    assert(wf_saturn_yaul_render_ops()->emit_sprite_16x16(b,&sprite->cmd)==WF_OK);
}
static void make_pattern(uint8_t *tile,unsigned top,unsigned height,unsigned seed)
{
    memset(tile,0,128u);
    for(unsigned y=top;y<top+height;++y)for(unsigned x=0;x<16u;++x){
        /* Gaps inside the crop, both nibbles, penF, and asymmetric flips. */
        unsigned pen=(x*3u+y*5u+seed)%16u;
        if((y-top)%3u==1u && y!=top+height-1u)pen=0;
        if(x==0u && (y==top || y==top+height-1u))pen=15u;
        tile[y*8u+x/2u]|=(uint8_t)(x&1u?pen:pen<<4);
    }
}
static void setup_palettes(wf_saturn_yaul_backend_t *b)
{
    const wf_saturn_render_ops_t *ops=wf_saturn_yaul_render_ops();
    wf_saturn_cram_reset();
    assert(ops->begin_frame(b,0,16u,0)==WF_OK);
    for(unsigned bank=0;bank<16u;++bank){
        for(unsigned pen=0;pen<16u;++pen){
            unsigned r=(pen+bank)&15u,g=(pen*3u+bank*5u)&15u,blue=(pen*7u+bank*11u)&15u;
            unsigned color=r|(g<<4)|(blue<<8);
            palettes[bank][pen*2u]=(uint8_t)(color>>8);
            palettes[bank][pen*2u+1u]=(uint8_t)color;
            colors[bank][pen]=(r*17u)|((g*17u)<<8)|((blue*17u)<<16);
        }
        assert(ops->upload_palette_xbgr444_16(b,(uint8_t)bank,palettes[bank],32u)==WF_OK);
    }
    assert(ops->end_frame(b)==WF_OK);
}
static void exhaustive_patterns(wf_saturn_yaul_backend_t *b)
{
    static const int16_t positions[][2]={
        {101,79},{-15,-15},{-1,0},{0,-1},{319,239},{312,232},{320,240},{-16,-16}
    };
    uint8_t tile[128];
    for(unsigned top=0;top<16u;++top)for(unsigned height=1u;height<=16u-top;++height){
        make_pattern(tile,top,height,top+height);
        for(unsigned flip=0;flip<4u;++flip)for(unsigned p=0;p<sizeof positions/sizeof positions[0];++p){
            reference_sprite_t ref={.cmd={.x=positions[p][0],.y=positions[p][1],
                .stream_tile=0,.cram_slot=(uint8_t)((top+height+p)&15u),
                .flip_x=(uint8_t)(flip&1u),.flip_y=(uint8_t)(flip>>1)},.pixels=tile};
            begin(b,1u,1u);upload(b,0,tile);emit(b,&ref);finish(b,&ref,1u,height,1u);
        }
    }
    /* All transparent tiles consume a logical request, never a hardware sprite. */
    memset(tile,0,sizeof tile);
    for(unsigned flip=0;flip<4u;++flip){
        reference_sprite_t ref={.cmd={.x=8,.y=8,.flip_x=(uint8_t)(flip&1u),
            .flip_y=(uint8_t)(flip>>1),.cram_slot=15},.pixels=tile};
        begin(b,1,1);upload(b,0,tile);emit(b,&ref);finish(b,&ref,1,0,0);
    }
}
static void order_and_high_slots(wf_saturn_yaul_backend_t *b)
{
    uint8_t first[128],second[128];
    make_pattern(first,3,9,7);make_pattern(second,7,1,14);
    reference_sprite_t refs[2]={
        {.cmd={.x=20,.y=20,.stream_tile=254,.cram_slot=1,.flip_y=1},.pixels=first},
        {.cmd={.x=20,.y=20,.stream_tile=255,.cram_slot=14,.flip_x=1},.pixels=second}
    };
    begin(b,256,2);
    for(unsigned slot=0;slot<256u;++slot)upload(b,slot,slot==255u?second:first);
    emit(b,&refs[0]);emit(b,&refs[1]);finish(b,refs,2,10,2);
    /* VRAM has512 legal slots but the upload/cache arrays have256. An
     * unstaged command must keep all16 rows and must not index those arrays. */
    for(unsigned slot=256u;slot<512u;++slot){
        reference_sprite_t ref={.cmd={.x=4,.y=4,.stream_tile=(uint16_t)slot,
            .cram_slot=(uint8_t)(slot&15u),.flip_y=(uint8_t)(slot&1u)},.pixels=second};
        memcpy(mock_vram+WF_SATURN_VDP1_DYNAMIC_OFFSET+slot*128u,second,128u);
        begin(b,0,1);emit(b,&ref);finish(b,&ref,1,16,1);
    }
}
static const uint8_t *rom_pixels;
static wf_status_t read_rom(uint16_t source,const uint8_t **out)
{
    assert(source==123u || source==124u);++rom_reads;*out=rom_pixels;return WF_OK;
}
static void cached_mutations_and_aborts(wf_saturn_yaul_backend_t *b)
{
    const wf_saturn_render_ops_t *ops=wf_saturn_yaul_render_ops();
    uint8_t old[128],changed[128],source[128];
    make_pattern(old,2,4,8);make_pattern(changed,11,2,3);memcpy(source,old,128u);
    reference_sprite_t ref={.cmd={.x=319,.y=233,.flip_y=1,.cram_slot=15},.pixels=old};
    rom_pixels=source;unsigned reads=rom_reads;
    begin(b,1,1);assert(wf_saturn_yaul_upload_rom_tile(b,0,123u,read_rom)==WF_OK);
    emit(b,&ref);finish(b,&ref,1,4,1);assert(rom_reads==reads+1u);
    /* The immutable-source hit must use resident VRAM/cache metadata even
     * when the callback's backing buffer has since changed. */
    memcpy(source,changed,128u);unsigned dma=transfers;
    begin(b,1,1);assert(wf_saturn_yaul_upload_rom_tile(b,0,123u,read_rom)==WF_OK);
    emit(b,&ref);finish(b,&ref,1,4,1);assert(rom_reads==reads+1u && transfers==dma);
    /* Changed untyped data recomputes bounds and invalidates ROM identity. */
    ref.pixels=changed;
    begin(b,1,1);upload(b,0,changed);emit(b,&ref);finish(b,&ref,1,2,1);
    begin(b,1,1);assert(wf_saturn_yaul_upload_rom_tile(b,0,124u,read_rom)==WF_OK);
    emit(b,&ref);finish(b,&ref,1,2,1);assert(rom_reads==reads+2u);
    /* Abort changed staging: no DMA/list submission and no crop using that
     * uncommitted data in the next frame when the slot is not uploaded. */
    dma=transfers;unsigned puts=submissions;
    begin(b,1,1);upload(b,0,old);emit(b,&ref);ops->abort_frame(b);
    assert(transfers==dma && submissions==puts);
    begin(b,0,1);emit(b,&ref);finish(b,&ref,1,16,1);
    ref.pixels=old;
    begin(b,1,1);upload(b,0,old);emit(b,&ref);finish(b,&ref,1,4,1);assert(transfers==dma+1u);
    /* An incomplete end_frame has the same invalid staging semantics. */
    begin(b,2,1);upload(b,0,changed);emit(b,&ref);
    assert(ops->end_frame(b)==WF_BAD_STATE);
    begin(b,0,1);emit(b,&ref);finish(b,&ref,1,16,1);
    ref.pixels=changed;
    begin(b,1,1);upload(b,0,changed);emit(b,&ref);finish(b,&ref,1,2,1);
    /* Retry through ROM IDs after an aborted source change must re-read. */
    memcpy(source,old,128u);reads=rom_reads;
    begin(b,1,1);assert(wf_saturn_yaul_upload_rom_tile(b,0,123u,read_rom)==WF_OK);ops->abort_frame(b);
    begin(b,1,1);assert(wf_saturn_yaul_upload_rom_tile(b,0,123u,read_rom)==WF_OK);
    ref.pixels=old;emit(b,&ref);finish(b,&ref,1,4,1);assert(rom_reads==reads+2u);
    /* Reinitialization clears residency even when stage memory is retained. */
    wf_saturn_yaul_backend_init(b,stage,sizeof stage);
    begin(b,0,1);emit(b,&ref);finish(b,&ref,1,16,1);
}
static void corpus(wf_saturn_yaul_backend_t *b,const char *path)
{
    FILE *file=fopen(path,"rb");uint8_t header[8],tile[128];
    assert(file && fread(header,1,sizeof header,file)==sizeof header && !memcmp(header,"R18H",4));
    unsigned count=(unsigned)header[4]*256u+header[5];
    assert(count>0u && header[6]==0u && header[7]==128u);
    assert(fseek(file,count>2044u?8192L:4096L,SEEK_SET)==0);
    uint64_t before=hardware_rows,full_before=full_rows;
    for(unsigned i=0;i<count;++i){
        assert(fread(tile,1,sizeof tile,file)==sizeof tile);
        /* This independent corpus measurement only specifies work. Pixel
         * equivalence always uses the untouched complete source above. */
        unsigned first=16u,last=0u;
        for(unsigned y=0;y<16u;++y)for(unsigned x=0;x<16u;++x)if(pen_at(tile,x,y)){
            if(first==16u)first=y;last=y;
        }
        unsigned height=first==16u?0u:last-first+1u;
        for(unsigned flip=0;flip<4u;++flip){
            reference_sprite_t ref={.cmd={.x=(int16_t)(i%3u==0u?-7:117),
                .y=(int16_t)(i%3u==1u?233:83),.cram_slot=(uint8_t)(i&15u),
                .flip_x=(uint8_t)(flip&1u),.flip_y=(uint8_t)(flip>>1)},.pixels=tile};
            begin(b,1,1);upload(b,0,tile);emit(b,&ref);finish(b,&ref,1,height,height!=0u);
        }
    }
    fclose(file);
    printf("Corpus %u tiles/all4 flips: %llu full rows -> %llu submitted rows\n",count,
           (unsigned long long)(full_rows-full_before),(unsigned long long)(hardware_rows-before));
}
int main(int argc,char **argv)
{
    assert(argc<=2 && sizeof(vdp1_cmdt_t)==32u);
    wf_saturn_yaul_backend_t backend;
    wf_saturn_yaul_backend_init(&backend,stage,sizeof stage);setup_palettes(&backend);
    exhaustive_patterns(&backend);order_and_high_slots(&backend);cached_mutations_and_aborts(&backend);
    if(argc==2)corpus(&backend,argv[1]);
    assert(hardware_rows<full_rows);
    printf("PASS %u framebuffer comparisons: trims/flips/palettes/penF/clip/order/cache/abort/retry; %llu -> %llu VDP1 rows\n",
           frames,(unsigned long long)full_rows,(unsigned long long)hardware_rows);
    return 0;
}
