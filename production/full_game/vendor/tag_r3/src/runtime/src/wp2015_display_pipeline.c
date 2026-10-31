#include "wp2015_display_pipeline.h"
#include "wf_saturn_memory.h"
#include <yaul.h>
#include <string.h>

/* Each set reserves the old name-table layout: 2KiB FG, 2KiB BG and
 * 8KiB HUD in a 24KiB address stride. Three sets end at VDP2 bank1+
 * 0x12000, below the back color at+0x1fffe; the logical shadow is HWRAM.
 * The converted palette queues occupy VDP1's explicitly free interval;
 * VDP1 never fetches commands or textures from that interval. */
enum { MAP_BANKS=3,MAP_WORDS=6144,MAP_BYTES=MAP_WORDS*2,
       MAP_STRIDE=0x6000,MAP_SHADOW=0x12000,PALETTE_OFFSET=0x40000 };
_Static_assert(MAP_SHADOW==MAP_BANKS*MAP_STRIDE,"Name banks must not overlap");
_Static_assert(MAP_SHADOW+MAP_BYTES<WF_SATURN_VDP2_BACK_COLOR,"Keep back color separate");
_Static_assert(PALETTE_OFFSET>=WF_SATURN_VDP1_CMD_OFFSET+WF_SATURN_VDP1_CMD_BYTES,"Keep palette queue outside commands");
_Static_assert(PALETTE_OFFSET+8192u<=WF_SATURN_VDP1_DYNAMIC_OFFSET,"Keep palette queue outside textures");

typedef struct { uint16_t x[2],y[2];uint8_t priority,show,map_bank,pad;uint32_t generation; } snapshot_t;
static snapshot_t logical,snapshot[2];
static uint8_t name_stale[MAP_BANKS][MAP_WORDS/8];
/* A byte enters this bounded list only on zero -> nonzero. Preparing that
 * physical bank consumes and clears it. Thus each of the768 bytes can be
 * queued at most once, regardless of repeated writes to its eight names. */
static uint16_t name_dirty_bytes[MAP_BANKS][MAP_WORDS/8];
static uint16_t name_dirty_count[MAP_BANKS];
/* The build bank is fixed from prepare through seal. Cache the other two
 * queues once instead of searching three banks and rebuilding their
 * addresses for every changed name. IRQ publication cannot alter these
 * CPU-owned targets or make the unsealed build bank visible. */
typedef struct { uint8_t *bits;uint16_t *queue,*count; } name_target_t;
static name_target_t name_targets[2];
static unsigned name_write_delta;
static void name_targets_select(unsigned build){
    unsigned target=0;
    for(unsigned bank=0;bank<MAP_BANKS;bank++)if(bank!=build){
        name_targets[target].bits=name_stale[bank];
        name_targets[target].queue=name_dirty_bytes[bank];
        name_targets[target].count=&name_dirty_count[bank];++target;
    }
    name_write_delta=build*MAP_STRIDE;
}
static __inline__ __attribute__((always_inline)) int name_target_mark(const name_target_t *target,unsigned byte,unsigned mask){
    uint8_t *stale=target->bits+byte;unsigned bits=*stale;
    if(!bits){
        unsigned count=*target->count;
        if(count>=MAP_WORDS/8u)return -1;
        target->queue[count]=(uint16_t)byte;*target->count=(uint16_t)(count+1u);
    }
    *stale=(uint8_t)(bits|mask);return 0;
}
/* This logical CPU shadow is not scanout storage. Cached HWRAM avoids a
 * VDP2 read cycle for every changed/restored name. Physical banks remain
 * separate and are published only after preparation finishes. */
static uint16_t name_shadow[MAP_WORDS] __attribute__((aligned(32)));
static const uint8_t bit_masks[8]={1u,2u,4u,8u,16u,32u,64u,128u};
static uint8_t color_changed[2][128];
static volatile unsigned active,scan_bank,scheduled_bank;
static unsigned build_bank,build_slot,direct_bank;
static volatile uint32_t scheduled_generation,visible_generation;
static unsigned sealed[2];
volatile uint32_t wp2015_display_observer[12];

static uint16_t *logical_names(void){return name_shadow;}
static volatile uint32_t *palette_values(unsigned slot){return (volatile uint32_t*)VDP1_VRAM(PALETTE_OFFSET+slot*4096u);}
static unsigned name_offset(unsigned index){
    if(index<1024u)return index*2u;
    if(index<2048u)return 0x2000u+(index-1024u)*2u;
    return 0x4000u+(index-2048u)*2u;
}
static volatile uint16_t *physical_name(unsigned bank,unsigned index){return (volatile uint16_t*)VDP2_VRAM_ADDR(1,bank*MAP_STRIDE+name_offset(index));}
static int name_index(const volatile uint16_t *dest){
    uintptr_t offset=(uintptr_t)dest-(uintptr_t)VDP2_VRAM_ADDR(1,0);
    if((offset&1u)||offset>=0x6000u)return -1;
    if(offset<0x800u)return (int)(offset/2u);
    if(offset>=0x2000u&&offset<0x2800u)return (int)(1024u+(offset-0x2000u)/2u);
    if(offset>=0x4000u)return (int)(2048u+(offset-0x4000u)/2u);
    return -1;
}
static void scroll_set(unsigned layer,unsigned x,unsigned y){
    vdp2_scrn_t screen=layer?VDP2_SCRN_NBG1:VDP2_SCRN_NBG0;
    vdp2_scrn_scroll_x_set(screen,fix16_int32_from(x));
    vdp2_scrn_scroll_y_set(screen,fix16_int32_from(y));
}
static void priority_set(uint8_t priority,unsigned show){
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0,priority==0x78u?2u:1u);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG1,priority==0x78u?1u:priority==0x7bu?2u:3u);
    if(!show)return;
    for(unsigned i=0;i<8u;++i)vdp2_sprite_priority_set(i,priority==0x7cu?2u:3u);
    vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG2|
      (priority==0x78u?(VDP2_SCRN_DISPTP_NBG0|VDP2_SCRN_DISP_NBG1):
                       (VDP2_SCRN_DISP_NBG0|VDP2_SCRN_DISPTP_NBG1)));
}
static void maps_set(unsigned bank){
    for(unsigned layer=0;layer<3u;++layer){
        const vdp2_scrn_cell_format_t format={
          .scroll_screen=layer==0?VDP2_SCRN_NBG0:layer==1?VDP2_SCRN_NBG1:VDP2_SCRN_NBG2,
          .char_size=layer==2?VDP2_SCRN_CHAR_SIZE_1X1:VDP2_SCRN_CHAR_SIZE_2X2,
          .pnd_size=1};
        vdp2_vram_t address=VDP2_VRAM_ADDR(1,bank*MAP_STRIDE+layer*0x2000u);
        for(unsigned plane=0;plane<4u;++plane)vdp2_scrn_map_plane_set(&format,plane,address);
    }
}

void wp2015_display_name_write(volatile uint16_t *dest,uint16_t value){
    if(!active){
        int index=name_index(dest);
        if(direct_bank&&index>=0)*(volatile uint16_t*)((uintptr_t)dest+direct_bank*MAP_STRIDE)=value;
        else *dest=value;
        return;
    }
    int signed_index=name_index(dest);
    if(signed_index<0){++wp2015_display_observer[8];return;}
    unsigned index=(unsigned)signed_index,byte=index>>3,mask=bit_masks[index&7u];
    if(logical_names()[index]==value)return;
    if(build_bank==scan_bank||sealed[build_slot]){++wp2015_display_observer[8];return;}
    logical_names()[index]=value;
    if(name_target_mark(&name_targets[0],byte,mask)||name_target_mark(&name_targets[1],byte,mask)){
        ++wp2015_display_observer[8];return;
    }
    /* dest was validated as a canonical bank-zero map address above.
     * Preserve its exact offset instead of decoding and re-encoding it. */
    *(volatile uint16_t*)((uintptr_t)dest+name_write_delta)=value;
    ++wp2015_display_observer[3];
}
/* The CPU owns one unsealed build bank for the whole call. VBlank cannot
 * publish that bank until seal; cached targets therefore stay valid. Only
 * exact canonical map bases take this route. All other addresses and
 * unhealthy ownership states retain the scalar operation, in order. */
void wp2015_display_names_write(volatile uint16_t *base,const wp2015_name_write_t *writes,size_t count){
    if(!count)return;
    uintptr_t offset=(uintptr_t)base-(uintptr_t)VDP2_VRAM_ADDR(1,0);
    unsigned first,limit;
    if(offset==0u){first=0u;limit=1024u;}
    else if(offset==0x2000u){first=1024u;limit=1024u;}
    else if(offset==0x4000u){first=2048u;limit=4096u;}
    else goto scalar;
    if(!active||build_bank==scan_bank||sealed[build_slot])goto scalar;
    for(size_t i=0;i<count;i++){
        unsigned cell=writes[i].cell;uint16_t value=writes[i].value;
        if(cell>=limit){wp2015_display_name_write(base+cell,value);continue;}
        unsigned index=first+cell;
        if(name_shadow[index]==value)continue;
        unsigned byte=index>>3,mask=bit_masks[index&7u];
        name_shadow[index]=value;
        if(name_target_mark(&name_targets[0],byte,mask)||name_target_mark(&name_targets[1],byte,mask)){
            ++wp2015_display_observer[8];continue;
        }
        *(volatile uint16_t*)((uintptr_t)(base+cell)+name_write_delta)=value;
        ++wp2015_display_observer[3];
    }
    return;
scalar:
    for(size_t i=0;i<count;i++)wp2015_display_name_write(base+writes[i].cell,writes[i].value);
}

void wp2015_display_cram_write(uint16_t entry,uint32_t value){
    if(!active){*(volatile uint32_t*)VDP2_CRAM_MODE_2_OFFSET(0,entry>>4,entry&15u)=value;return;}
    if(entry>=1024u||sealed[build_slot]){++wp2015_display_observer[8];return;}
    palette_values(build_slot)[entry]=value;
    color_changed[build_slot][entry>>3]|=(uint8_t)(1u<<(entry&7u));
}
void wp2015_display_scroll(unsigned layer,unsigned x,unsigned y){
    if(layer>=2u){++wp2015_display_observer[8];return;}
    logical.x[layer]=(uint16_t)x;logical.y[layer]=(uint16_t)y;
    if(!active)scroll_set(layer,x,y);
}
void wp2015_display_priority(uint8_t priority,unsigned show){
    logical.priority=priority;logical.show=(uint8_t)!!show;
    if(!active)priority_set(priority,show);
}
void wp2015_display_reset(void){
    active=0;scan_bank=scheduled_bank=build_bank=build_slot=direct_bank=0;
    scheduled_generation=visible_generation=0;
    memset(&logical,0,sizeof logical);memset(snapshot,0,sizeof snapshot);
    memset(sealed,0,sizeof sealed);memset(name_stale,0,sizeof name_stale);
    memset(name_dirty_count,0,sizeof name_dirty_count);
    memset(color_changed,0,sizeof color_changed);
    memset((void*)wp2015_display_observer,0,sizeof wp2015_display_observer);
}
int wp2015_display_active(void){return (int)active;}
uint32_t wp2015_display_visible_generation(void){return visible_generation;}
int wp2015_display_begin(unsigned first_slot){
    if(active||direct_bank||first_slot>=2u)return -1;
    /* Boot/transition only, before measured combat. The live canonical
     * bank remains unchanged while the two inactive copies are created. */
    for(unsigned index=0;index<MAP_WORDS;++index){
        uint16_t value=*physical_name(0,index);logical_names()[index]=value;
        *physical_name(1,index)=value;*physical_name(2,index)=value;
    }
    scan_bank=scheduled_bank=0;build_bank=1;build_slot=first_slot;
    scheduled_generation=visible_generation=0;
    memset(sealed,0,sizeof sealed);memset(name_stale,0,sizeof name_stale);
    memset(name_dirty_count,0,sizeof name_dirty_count);
    memset(color_changed,0,sizeof color_changed);
    memset((void*)wp2015_display_observer,0,sizeof wp2015_display_observer);
    name_targets_select(build_bank);
    active=1;vdp2_sync();return 0;
}
int wp2015_display_seal(unsigned slot,uint32_t generation){
    if(!active)return 0;
    if(slot!=build_slot||sealed[slot]||build_bank==scan_bank){++wp2015_display_observer[8];return -1;}
    snapshot[slot]=logical;snapshot[slot].map_bank=(uint8_t)build_bank;
    snapshot[slot].generation=generation;sealed[slot]=1;
    ++wp2015_display_observer[0];return 0;
}
void wp2015_display_vblank_in(unsigned draw_slot,uint32_t draw_generation){
    if(!active||draw_generation==scheduled_generation)return;
    vdp2_tvmd_extern_latch();unsigned line=vdp2_tvmd_vcount_get();
    /* Only the normal VBlank-In boundary has the full palette-copy budget.
     * A delayed callback leaves the complete prior image selected. */
    if(!vdp2_tvmd_vblank_in()||line<240u||line>242u){++wp2015_display_observer[7];return;}
    if(draw_slot>=2u||!sealed[draw_slot]||snapshot[draw_slot].generation!=draw_generation){++wp2015_display_observer[8];return;}
    uint16_t start=cpu_frt_count_get();unsigned colors=0;
    const snapshot_t *frame=&snapshot[draw_slot];
    for(unsigned byte=0;byte<128u;++byte){
        unsigned bits=color_changed[draw_slot][byte];if(!bits)continue;
        for(unsigned bit=0;bit<8u;++bit)if(bits&(1u<<bit)){
            unsigned entry=byte*8u+bit;
            *(volatile uint32_t*)VDP2_CRAM_MODE_2_OFFSET(0,entry>>4,entry&15u)=palette_values(draw_slot)[entry];
            ++colors;
        }
    }
    maps_set(frame->map_bank);
    scroll_set(0,frame->x[0],frame->y[0]);scroll_set(1,frame->x[1],frame->y[1]);
    priority_set(frame->priority,frame->show);
    scheduled_bank=frame->map_bank;scheduled_generation=draw_generation;
    ++wp2015_display_observer[1];wp2015_display_observer[4]+=colors;
    if(colors>wp2015_display_observer[5])wp2015_display_observer[5]=colors;
    uint16_t ticks=(uint16_t)(cpu_frt_count_get()-start);
    if(ticks>wp2015_display_observer[6])wp2015_display_observer[6]=ticks;
    vdp2_tvmd_extern_latch();line=vdp2_tvmd_vcount_get();
    if(line>wp2015_display_observer[10])wp2015_display_observer[10]=line;
    if(!vdp2_tvmd_vblank_in()||line>=0x1fcu)++wp2015_display_observer[9];
}
void wp2015_display_vblank_out(void){
    if(!active)return;
    scan_bank=scheduled_bank;visible_generation=scheduled_generation;
    wp2015_display_observer[11]=scan_bank;
    /* Yaul reads the VDP2 request before calling our next VBlank-In hook.
     * Request here, not inside that hook, so its staged registers commit
     * in the same blank and cannot be overwritten by its cached flags. */
    vdp2_sync();
}
int wp2015_display_prepare(unsigned slot){
    if(!active)return 0;
    if(slot>=2u){++wp2015_display_observer[8];return -1;}
    unsigned pending=snapshot[slot^1u].map_bank;
    for(build_bank=0;build_bank<MAP_BANKS;++build_bank)
        if(build_bank!=scan_bank&&build_bank!=pending)break;
    if(build_bank==MAP_BANKS){++wp2015_display_observer[8];return -1;}
    build_slot=slot;sealed[slot]=0;memset(color_changed[slot],0,128u);
    name_targets_select(build_bank);
    unsigned count=name_dirty_count[build_bank];
    for(unsigned queued=0;queued<count;++queued){
        unsigned byte=name_dirty_bytes[build_bank][queued];
        unsigned bits=name_stale[build_bank][byte];
        for(unsigned bit=0;bits;++bit,bits>>=1u)if(bits&1u){
            unsigned index=byte*8u+bit;*physical_name(build_bank,index)=logical_names()[index];
            ++wp2015_display_observer[2];
        }
        name_stale[build_bank][byte]=0;
    }
    name_dirty_count[build_bank]=0;
    return 0;
}
void wp2015_display_stop(void){
    if(!active)return;
    /* Caller drained the final VDP1 generation. Keep its actual bank as
     * the live map for result text, whose original waits own its timing.
     * Direct result writes are relocated by retaining a simple active
     * name-bank mode rather than copying scanned pattern names. */
    direct_bank=scan_bank;active=0;
}
