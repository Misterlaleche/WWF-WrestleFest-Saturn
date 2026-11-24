/* Exercise the real driver with a deterministic Jo/VDP1 boundary. The mock
 * rejects a texture/map DMA whenever any displayed or pending frame owns it. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wf_jo_video_test_platform.h"
#include "wf_saturn_memory.h"

static uint8_t vram1[0x80000],vram2[0x80000];
static uint32_t cram[1024];
static wf_jo_command_t built[1000];
static unsigned short keys[1000];
static unsigned built_count,build_tag=100,field_count,show_tag,show_valid,show_changed;
static unsigned in_vblank,presented_bank,present_count,limit=993,blanked;
static unsigned presented_layers,presented_opaque,presented_sprites;
static void (*vblank)(void);
typedef struct live_frame {unsigned tag,bank;wf_jo_command_t first;} live_frame_t;
static live_frame_t live[64];static unsigned live_count;
static uint8_t tile[128];static uint32_t version[256];
static unsigned snapshot_bank;

void __jo_core_pause(void);
void jo_core_set_frame_overlap(bool allow){(void)allow;}
void __jo_core_synch(void){__jo_core_pause();}

bool __jo_vdp1_displayed_frame_changed(unsigned *tag)
{if(!show_changed)return false;*tag=show_tag;return true;}
unsigned __jo_vdp1_frame_tag(void){return build_tag;}
unsigned jo_core_get_vblank_count(void){return field_count;}
int jo_core_add_vblank_callback(void (*fn)(void)){vblank=fn;return 1;}
unsigned __jo_core_disable_interrupts(void){return 0;}
void __jo_core_restore_interrupts(unsigned sr){(void)sr;}
int jo_vdp1_frame_command_count(void){return (int)built_count;}
int jo_vdp1_command_max(void){return (int)limit;}
jo_vdp1_command *__jo_vdp1_reserve_commands(unsigned n,unsigned short **out,unsigned *reserved)
{*reserved=n<limit-built_count?n:limit-built_count;*out=keys+built_count;return built+built_count;}
void __jo_vdp1_commit_commands(unsigned n){built_count+=n;}
void jo_wf_vdp2_init(void){blanked=1;}
void jo_wf_vdp2_blank(void){blanked=1;}
void jo_wf_vdp2_gate(void){blanked=1;}
void jo_wf_vdp2_diagnostics(void){blanked=0;}
void jo_wf_vdp2_present(unsigned bank,const unsigned short *scroll,unsigned char priority,
                       unsigned layers,unsigned opaque,unsigned sprites)
{assert(in_vblank);assert(bank<WF_JO_TEXTURE_BANKS);(void)scroll;(void)priority;
 for(unsigned i=0;i<live_count;i++)if(live[i].tag==show_tag)assert(live[i].bank==bank);
 presented_bank=bank;presented_layers=layers;presented_opaque=opaque;presented_sprites=sprites;
 ++present_count;blanked=0;}
static int protected_bank(unsigned bank)
{for(unsigned i=0;i<live_count;i++)if(live[i].bank==bank)return 1;return 0;}
void jo_dma_copy(const void *src,void *dst,unsigned n)
{
    uintptr_t address=(uintptr_t)dst;
    if(address>=JO_VDP1_VRAM&&address+n<=JO_VDP1_VRAM+sizeof vram1){
        unsigned off=(unsigned)(address-JO_VDP1_VRAM);
        assert(off>=WF_JO_TEXTURE_FIRST);
        assert(off+n<=WF_JO_TEXTURE_FIRST+WF_JO_TEXTURE_BYTES);
        for(unsigned i=0;i<live_count;i++){
            unsigned first=((unsigned)live[i].first.srca*8u-WF_JO_TEXTURE_FIRST)&~127u;
            first+=WF_JO_TEXTURE_FIRST;
            assert(off+n<=first||off>=first+128u);
        }
        memcpy(vram1+off,src,n);return;
    }
    if(address>=JO_VDP2_VRAM_A0&&address+n<=JO_VDP2_VRAM_A0+sizeof vram2){
        unsigned off=(unsigned)(address-JO_VDP2_VRAM_A1);
        unsigned bank=off/WF_JO_MAP_GROUP_BYTES;
        assert(bank<WF_JO_TEXTURE_BANKS&&!protected_bank(bank));
        memcpy(vram2+address-JO_VDP2_VRAM_A0,src,n);return;
    }
    assert(address==JO_VDP2_CRAM&&n==sizeof cram&&in_vblank);
    memcpy(cram,src,n);
}
static void display(unsigned tag)
{
    unsigned out=0;for(unsigned i=0;i<live_count;i++)if(live[i].tag>=tag)live[out++]=live[i];
    live_count=out;show_tag=tag;show_valid=show_changed=1;++field_count;
    in_vblank=1;vblank();in_vblank=0;show_changed=0;
}
void __jo_core_pause(void)
{
    for(unsigned i=0;i<live_count;i++)if(!show_valid||live[i].tag>show_tag){display(live[i].tag);return;}
    assert(!"The test has no pending frame that can release a bank");
}
static void close_frame(void)
{
    assert(built_count==1&&live_count<64);
    assert(snapshot_bank<WF_JO_TEXTURE_BANKS);
    live[live_count++]=(live_frame_t){build_tag,snapshot_bank,built[0]};
    ++build_tag;built_count=0;
}
static unsigned prepare_tile(unsigned logical,uint16_t source,const uint8_t *pixels)
{
    wf_jo_texture_ref_t ref;unsigned hit;
    assert(wf_jo_video_texture_resolve(logical,source,1,&ref,&hit)==WF_OK);
    if(!hit){
        assert(wf_jo_video_texture_transfer(ref.physical,pixels,1)==WF_OK);
        assert(wf_jo_video_texture_ready(logical,source,1,0,16)==WF_OK);
    }
    return WF_JO_TEXTURE_FIRST+ref.physical*128u;
}
static unsigned publish(void)
{
    unsigned bank;assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    assert(wf_jo_video_texture_begin(1)==WF_OK);
    unsigned offset=prepare_tile(0,(uint16_t)version[0],tile);
    wf_vdp1_sprite_cmd_t sprite={0};wf_jo_crop_t crop={0,16,0,16};wf_jo_command_t command;
    assert(wf_jo_sprite_build(&sprite,&crop,offset,&command)==WF_OK);
    assert(wf_jo_video_submit(bank,&command,1)==WF_OK);snapshot_bank=bank;
    assert(wf_jo_video_commit()==WF_OK);return bank;
}
static void reset(void)
{
    wf_jo_video_diagnostics();memset(vram1,0xcc,sizeof vram1);memset(vram2,0xcc,sizeof vram2);
    memset(cram,0,sizeof cram);memset(tile,0x11,sizeof tile);memset(version,0,sizeof version);version[0]=1;
    build_tag=100;built_count=field_count=show_tag=show_valid=show_changed=0;
    live_count=present_count=0;snapshot_bank=0;limit=993;
    assert(wf_jo_video_init()==WF_OK);assert(blanked);
}
static void lifetime_and_snapshot(void)
{
    reset();
    wf_jo_video_cram_write(10,0x112233);assert(publish()==0);close_frame();
    wf_jo_video_cram_write(10,0x445566);assert(publish()==1);close_frame();
    wf_jo_video_cram_write(10,0x778899);assert(publish()==2);close_frame();
    wf_jo_video_cram_write(10,0xaabbcc);assert(publish()==3);close_frame();
    assert(wf_jo_video_stats.bank_wait_fields==0);
    unsigned bank;assert(wf_jo_video_acquire_bank(&bank)==WF_OK&&bank==0);
    assert(wf_jo_video_stats.bank_wait_fields==2);wf_jo_video_release_bank(bank);
    assert(show_tag==101&&cram[10]==0x445566&&presented_bank==1&&!blanked);
    assert(wf_jo_video_repeat()==WF_OK);assert(wf_jo_video_repeat()==WF_OK);
    assert(built_count==1);close_frame();
    display(103);assert(cram[10]==0xaabbcc&&presented_bank==3);
    assert(wf_jo_video_acquire_bank(&bank)==WF_OK&&bank!=3);wf_jo_video_release_bank(bank);
}
static void transfers_and_palette_only_frame(void)
{
    reset();assert(wf_jo_video_pending_changes());
    wf_jo_video_cram_write(5,0x123456);
    assert(publish()==0);assert(!wf_jo_video_pending_changes());close_frame();display(100);
    assert(publish()==1);close_frame();display(101);
    assert(publish()==0);assert(wf_jo_video_stats.last_texture_bytes==0);close_frame();display(102);
    assert(wf_jo_video_stats.texture_bytes==128&&wf_jo_video_stats.map_bytes==2*12288u);
    assert(wf_jo_video_stats.palette_bytes==4096);
    wf_jo_video_cram_write(5,0xabcdef);
    assert(wf_jo_video_pending_changes());
    assert(wf_jo_video_repeat()==WF_OK);assert(wf_jo_video_stats.last_texture_bytes==0);
    snapshot_bank=1;
    assert(!wf_jo_video_pending_changes());
    close_frame();assert(cram[5]==0x123456);display(103);assert(cram[5]==0xabcdef);
    assert(wf_jo_video_stats.map_bytes==2*12288u);
}
static void frozen_repeat_cannot_change_inflight_state(void)
{
    reset();wf_jo_video_cram_write(3,0x100);publish();close_frame();display(100);
    assert(wf_jo_video_repeat()==WF_OK);
    wf_jo_video_cram_write(3,0x200);
    /* The repeated list still points to the currently displayed map bank. */
    assert(wf_jo_video_commit()==WF_BLOCKED);
    close_frame();display(101);assert(cram[3]==0x100);
    assert(wf_jo_video_commit()==WF_OK);snapshot_bank=1;close_frame();display(102);assert(cram[3]==0x200);
}
static void failed_clone_does_not_damage_saved_commands(void)
{
    reset();publish();close_frame();display(100);
    wf_jo_video_cram_write(3,0x200);limit=0;
    assert(wf_jo_video_commit()==WF_BLOCKED);limit=993;
    assert(wf_jo_video_commit()==WF_OK);
    /* Map snapshots rotate; the shared physical texture address is stable. */
    assert(built[0].srca==live[0].first.srca);snapshot_bank=1;
    close_frame();display(101);assert(presented_bank==1);
}
static void frontend_layers_and_gate_are_frame_bound(void)
{
    reset();assert(!wf_jo_video_frame_queued());
    wf_jo_video_layers(4,4,3);publish();assert(wf_jo_video_frame_queued());close_frame();
    assert(!wf_jo_video_frame_queued());
    wf_jo_video_gate();assert(blanked);
    wf_jo_video_layers(7,1,2);publish();close_frame();
    display(100);assert(blanked&&present_count==0);
    display(101);assert(!blanked&&presented_layers==7&&presented_opaque==1&&presented_sprites==2);
    wf_jo_video_layers(4,4,3);assert(wf_jo_video_repeat()==WF_OK);snapshot_bank=0;close_frame();
    assert(presented_layers==7);display(102);
    assert(presented_layers==4&&presented_opaque==4&&presented_sprites==3);
}
static void borrowed_commands_do_not_alias_inflight_or_survive_abort(void)
{
    reset();publish();close_frame();display(100);
    unsigned bank;assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    wf_jo_command_t *workspace=wf_jo_video_commands_begin(1);assert(workspace);
    assert(workspace==built); /* Jo's completed-DMA CPU prefix is the writer. */
    wf_jo_command_t original=live[0].first;
    memset(workspace,0xa5,sizeof *workspace);tile[0]=0xff;++version[0];
    assert(!memcmp(&live[0].first,&original,sizeof original));
    wf_jo_video_commands_abort();wf_jo_video_release_bank(bank);
    assert(wf_jo_video_repeat()==WF_BAD_STATE);
    wf_jo_video_cram_write(2,0x200);assert(wf_jo_video_commit()==WF_BAD_STATE);
    assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    workspace=wf_jo_video_commands_begin(1);assert(workspace);
    assert(wf_jo_video_texture_begin(1)==WF_OK);
    unsigned offset=prepare_tile(0,(uint16_t)version[0],tile);
    wf_vdp1_sprite_cmd_t sprite={0};wf_jo_crop_t crop={0,16,0,16};
    assert(wf_jo_sprite_build(&sprite,&crop,offset,workspace)==WF_OK);
    assert(wf_jo_video_submit(bank,workspace,1)==WF_OK);snapshot_bank=bank;
    assert(wf_jo_video_commit()==WF_OK);
    assert(!wf_jo_video_commands_begin(1));
    close_frame();display(101);assert(wf_jo_video_repeat()==WF_OK);
}
static void command_preflight_preserves_previous_frame(void)
{
    reset();publish();close_frame();display(100);
    unsigned bank;assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    limit=0;assert(wf_jo_video_commands_begin(1)==NULL);
    wf_jo_video_release_bank(bank);limit=993;
    assert(wf_jo_video_repeat()==WF_OK);close_frame();display(101);
    assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
    built_count=1;assert(wf_jo_video_commands_begin(1)==NULL);
    built_count=0;wf_jo_video_release_bank(bank);
}
static void inactive_slots_preserve_shared_pixels_and_identity(void)
{
    reset();uint8_t pixels[384];uint32_t versions[3]={1,0,3};
    memset(pixels,0x12,128);memset(pixels+128,0x34,128);memset(pixels+256,0x56,128);
    for(unsigned frame=0;frame<5u;frame++){
        if(frame==1u||frame==2u){versions[1]=9;versions[2]=0;}
        if(frame>=3u){versions[1]=0;versions[2]=3;}
        unsigned bank;assert(wf_jo_video_acquire_bank(&bank)==WF_OK);
        assert(wf_jo_video_texture_begin(3)==WF_OK);
        for(unsigned i=0;i<3;i++)if(versions[i]){
            wf_jo_texture_ref_t ref;unsigned hit;
            assert(wf_jo_video_texture_protect(i,(uint16_t)versions[i],&ref,&hit)==WF_OK);
        }
        unsigned offset=0;
        for(unsigned i=0;i<3;i++)if(versions[i]){
            unsigned at=prepare_tile(i,(uint16_t)versions[i],pixels+i*128u);
            if(!i)offset=at;
        }
        wf_vdp1_sprite_cmd_t sprite={0};wf_jo_crop_t crop={0,16,0,16};wf_jo_command_t command;
        assert(wf_jo_sprite_build(&sprite,&crop,offset,&command)==WF_OK);
        assert(wf_jo_video_submit(bank,&command,1)==WF_OK);snapshot_bank=bank;
        assert(wf_jo_video_commit()==WF_OK);
        assert(wf_jo_video_stats.last_texture_bytes==(!frame?256u:frame==1u?128u:0u));
        const uint16_t *physical=wf_jo_video_texture_frame_slots();
        for(unsigned i=0;i<3;i++){
            if(versions[i])assert(!memcmp(vram1+WF_JO_TEXTURE_FIRST+physical[i]*128u,pixels+i*128u,128));
            else assert(physical[i]==WF_JO_GPU_NONE);
        }
        close_frame();display(100u+frame);
    }
}
int main(void)
{
    lifetime_and_snapshot();transfers_and_palette_only_frame();
    frozen_repeat_cannot_change_inflight_state();failed_clone_does_not_damage_saved_commands();
    frontend_layers_and_gate_are_frame_bound();
    borrowed_commands_do_not_alias_inflight_or_survive_abort();
    command_preflight_preserves_previous_frame();
    inactive_slots_preserve_shared_pixels_and_identity();
    puts("Jo pipeline: asynchronous fences, exact snapshots, sparse transfers, repeats and failure rollback: PASS");
    return 0;
}
