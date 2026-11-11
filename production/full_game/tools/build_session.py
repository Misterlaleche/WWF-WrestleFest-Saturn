#!/usr/bin/env python3
"""Build the shared session with locked HW1 owners; never write upstream."""
import argparse, hashlib, json, os, re, subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from tag_viewport import REPLACEMENTS as TAG_VIEWPORT, project as tag_viewport
from hw1_banked import project_backend, project_cache, project_display, project_vdp2_prefix, project_ring_prefix, project_tile_index
from tag_resident_geometry import project_header as tag_secondary_header
from rope_reentry import project_rope_init
from tag_transition_residency import project_secondary_header as project_transition_header, project_secondary as project_transition_secondary, project_ring as project_transition_ring, project_cd_header
from tag_cd_window import project_secondary as project_cd_window
from tag_secondary_batch2 import project_secondary as project_tag_batch2, project_cd_header as project_tag_batch2_header
from tag_cold_index import project_present as project_cold_index
from cropped_cart import project_cropped_backend, project_cropped_present
from shared_crop_pool import project_shared_present
from grouped_crop_pool import project_grouped_present
from retained_crop_pool import project_retained_present
from global_rans_cart import project_global_present
from rom_bulk_upload import project_backend as project_bulk_backend, project_header as project_bulk_header, project_present as project_bulk_present

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--probe',type=Path)
    p.add_argument('--sanitize',action='store_true')
    p.add_argument('--postmatch',action='store_true',help='Original post owners and metadata-only frontend return binding')
    p.add_argument('--sh2',action='store_true')
    p.add_argument('--memory-override',type=Path,help='Force one Saturn memory layout header into every shared-session object')
    p.add_argument('--combat-only',action='store_true',help='Separate native combat module, O2 and original HW1 framebuffer transport')
    p.add_argument('--base-royal',action='store_true',help='Restore HW1 base Royal cache budgets; requires --combat-only')
    p.add_argument('--tag-transport',action='store_true',help='Recovered TAG resident transport and generation pipeline; requires --combat-only')
    p.add_argument('--tag-secondary-batch2',action='store_true',help='Opt-in startup CD puts of at most two sectors using the existing scratch; requires --tag-transport')
    p.add_argument('--hw1-variable',action='store_true',help='Diagnostic: SDK END-gated manual swap with the recovered HW1 caches')
    p.add_argument('--hw1-banked',action='store_true',help='Diagnostic: recovered TAG display banks with the HW1 resident decoder')
    p.add_argument('--cropped-cart',action='store_true',help='Experimental R19C atlas; CART profile only, exact raw fallback retained')
    p.add_argument('--shared-crop-pool',action='store_true',help='Assemble R19C memory from shared compressed resources before combat; requires --cropped-cart')
    p.add_argument('--grouped-crop-pool',action='store_true',help='Assemble R19C from selected grouped raw blocks; requires --cropped-cart')
    p.add_argument('--retained-crop-pool',action='store_true',help='Keep selected raw blocks directly in cartridge; requires --cropped-cart')
    p.add_argument('--global-rans',action='store_true',help='Experimental complete sprite corpus in CART using the original TAG decoder')
    p.add_argument('--global-rans-small',action='store_true',help='Opt-in R24G v2 four-tile blocks; requires --global-rans')
    p.add_argument('--global-rans-cost',action='store_true',help='Diagnostic provider timers; requires global rANS and WF_NATIVE_PROFILE=1')
    p.add_argument('--global-rans-fast-hash',action='store_true',help='Opt-in exact SH2 FNV loop; requires --global-rans')
    p.add_argument('--global-rans-trace',action='store_true',help='Diagnostic tile request order; requires global rANS and WF_NATIVE_PROFILE=1')
    p.add_argument('--global-rans-priority',action='store_true',help='Opt-in cost-aware cartridge retention; requires --global-rans')
    p.add_argument('--bulk-rom-upload',action='store_true',help='Opt-in committed ROM upload batch; requires --hw1-banked')
    p.add_argument('--entry',action='append',default=[],help='Additional native public root, without underscore')
    args=p.parse_args()
    if args.combat_only and not args.sh2:p.error('--combat-only requires --sh2')
    if args.memory_override and (not args.sh2 or not args.memory_override.is_file()):
        p.error('--memory-override requires --sh2 and an existing header')
    if args.base_royal and not args.combat_only:p.error('--base-royal requires --combat-only')
    if args.tag_transport and (not args.combat_only or args.base_royal):p.error('--tag-transport requires --combat-only and excludes --base-royal')
    if args.tag_secondary_batch2 and not args.tag_transport:p.error('--tag-secondary-batch2 requires --tag-transport')
    if args.hw1_variable and (not args.combat_only or args.tag_transport):p.error('--hw1-variable requires a HW1 combat module')
    if args.hw1_banked and (not args.combat_only or args.tag_transport or args.hw1_variable):p.error('--hw1-banked requires a separate HW1 combat profile')
    if args.bulk_rom_upload and not args.hw1_banked:p.error('--bulk-rom-upload requires --hw1-banked')
    if args.cropped_cart and (not args.hw1_banked or args.base_royal):p.error('--cropped-cart requires the non-base HW1 banked CART profile')
    if args.shared_crop_pool and not args.cropped_cart:p.error('--shared-crop-pool requires --cropped-cart')
    if args.grouped_crop_pool and not args.cropped_cart:p.error('--grouped-crop-pool requires --cropped-cart')
    if args.retained_crop_pool and not args.cropped_cart:p.error('--retained-crop-pool requires --cropped-cart')
    if args.global_rans and not args.cropped_cart:p.error('--global-rans requires --cropped-cart')
    if args.global_rans_small and not args.global_rans:p.error('--global-rans-small requires --global-rans')
    if args.global_rans_fast_hash and not args.global_rans:p.error('--global-rans-fast-hash requires --global-rans')
    if args.global_rans_priority and not args.global_rans:p.error('--global-rans-priority requires --global-rans')
    if args.global_rans_cost and (not args.global_rans or os.environ.get('WF_NATIVE_PROFILE','0')!='1'):p.error('--global-rans-cost requires --global-rans and WF_NATIVE_PROFILE=1')
    if args.global_rans_trace and (not args.global_rans or os.environ.get('WF_NATIVE_PROFILE','0')!='1'):p.error('--global-rans-trace requires --global-rans and WF_NATIVE_PROFILE=1')
    if sum((args.grouped_crop_pool,args.shared_crop_pool,args.retained_crop_pool,args.global_rans))>1:p.error('Choose one pool transport')
    if args.combat_only and not (args.tag_transport or args.hw1_banked or args.hw1_variable):p.error('Royal combat requires --hw1-banked; the unbanked AUTO diagnostic failed the reader-ownership review')
    banked=args.tag_transport or args.hw1_banked
    native_profile=args.sh2 and os.environ.get('WF_NATIVE_PROFILE','0')=='1'
    root=Path(__file__).resolve().parents[1]
    native=root.parents[1]/'modo Royal/wp1980/work/hwfix01/native'
    subprocess.run(['python3',str(root/'tools/check_sources.py')],check=True)
    make=(native/'Makefile').read_text()
    names=[]
    for key in ('CORE','R18_HOST'):
        names+=re.search(r'^'+key+r'=(.*)$',make,re.M).group(1).split()
    names+=['src/wp1980_inflate.c','src/wp1980_lz4.c','src/wp1980_inflate_dynamic.c',
            'src/wp1980_vram_cache.c','src/wp1980_royal_init.c']
    sources=[native/n for n in names]+[root/'src/wf_session.c',root/'src/wf_video_bus.c',root/'src/wf_transport.c',root/'src/wf_combat.c']
    if args.postmatch:sources+=[root/'src/wf_postmatch.c',root/'src/wf_finale.c']
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    tag=root/'vendor/tag_r3/src/runtime'
    if banked:
        renderers={'wf_r18_present.c','wf_saturn_yaul_backend.c','wf_saturn_memory.c','wf_r18_ring.c','wf_r18_hud.c','wf_vdp1_plan.c','wf_r18_world.c'}
        if args.hw1_banked:renderers-= {'wf_r18_present.c','wf_saturn_yaul_backend.c'}
        sources=[tag/'src'/s.name if s.name in renderers else s for s in sources]
        sources += [tag/'src'/n for n in ('wp2015_display_pipeline.c','wp2015_vdp1_observer.c')]
        if args.tag_transport:sources += [tag/'src'/n for n in ('wp2015_secondary.c','wp2015_secondary_codec.c')]
        if args.hw1_banked:sources += [root/'src/wf_vram_reader.c']
        if args.hw1_banked and args.base_royal:sources += [root/'src/wf_vdp2_resident.c']
        headers=out/'include';headers.mkdir(exist_ok=True)
        for name in ('wf_r18_present.h','wf_saturn_yaul_backend.h','wf_saturn_memory.h','wf_vdp1_plan.h','wf_r18_ring.h','wp2015_secondary.h'):
            if args.hw1_banked and name=='wp2015_secondary.h':continue
            content=((native if args.hw1_banked and name in ('wf_r18_present.h','wf_saturn_yaul_backend.h') else tag)/'include'/name).read_text()
            if args.hw1_banked and name=='wf_saturn_yaul_backend.h':
                end=content.rfind('#endif')
                content=content[:end]+'''void wf_saturn_yaul_vblank_out(void);
wf_status_t wf_saturn_yaul_frame_sync(void);
void wf_saturn_yaul_frame_wait(void);
void wf_saturn_yaul_display_drain(void);
wf_status_t wf_saturn_yaul_display_begin(void);
'''+content[end:]
            if args.hw1_banked and name=='wf_saturn_memory.h':
                macro='#define WF_SATURN_HWRAM_HOT_INDEX_BYTES    0x0000ca80u'
                assert content.count(macro)==1
                content=content.replace(macro,'#ifndef WF_SATURN_HWRAM_HOT_INDEX_BYTES\n'+macro+'\n#endif')
            if name=='wp2015_secondary.h':content=tag_secondary_header(content,16384 if os.environ.get('WF_TRACE_ALL')=='1' else 0)
            if args.tag_transport and name=='wp2015_secondary.h':content=project_transition_header(content)
            if args.bulk_rom_upload and name=='wf_saturn_yaul_backend.h':content=project_bulk_header(content)
            (headers/name).write_text(content)
        if args.tag_transport:
            content=project_cd_header((tag/'include/wp2015_cd_cache.h').read_text())
            if args.tag_secondary_batch2:content=project_tag_batch2_header(content)
            (headers/'wp2015_cd_cache.h').write_text(content)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(native/'include'),
           '-I'+str(root/'src'),'-DWF_R18_ROM_EXTRA_CAPACITY='+('55u' if args.base_royal else '28u'),
           '-DWF_SATURN_HWRAM_HOT_INDEX_BYTES='+(('0x21000u' if args.hw1_banked else '0x25000u' if args.hw1_variable else '0x27000u') if args.base_royal else '0x14000u')]
    if args.memory_override:flags+=['-include',str(args.memory_override.resolve())]
    cc=os.environ.get('CC','cc')
    if args.sh2:
        if args.probe or args.sanitize:p.error('--sh2 builds a core archive, without host probes/sanitizers')
        sdk=Path(os.environ['YAUL_INSTALL_ROOT']);cc=str(sdk/'bin/sh2eb-elf-gcc')
        if subprocess.check_output([cc,'-dumpfullversion'],text=True).strip()!='13.2.0':
            raise ValueError('Expected supplied SH-2 GCC13.2.0')
        flags+=['-Os','-m2','-mb','-flto','-ffreestanding','-ffunction-sections','-fdata-sections',
                '-I'+str(sdk/'sh2eb-elf/include'),'-I'+str(sdk/'sh2eb-elf/include/yaul'),'-I'+str(root/'src/saturn'),'-DWP1980_DISABLE_MUSIC=1',
                '-DWP1980_VRAM_CACHE_SLOTS=3584u']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    if args.combat_only:flags+=['-O2','-DWF_COMBAT_MODULE=1','-I'+str(tag/'include')]
    if args.hw1_variable:flags+=['-DWF_COMBAT_HW1_MANUAL=1']
    if args.hw1_banked:flags=['-I'+str(headers),*flags,'-DWF_COMBAT_BANKED_TRANSPORT=1','-DWF_COMBAT_HW1_BANKED=1','-DWF_PLATFORM_SATURN=1']
    if args.global_rans:flags+=['-DWF_COMBAT_GLOBAL_RANS=1']
    if args.global_rans_small:flags+=['-DWF_GLOBAL_RANS_SMALL_BLOCKS=1']
    if args.global_rans_cost:flags+=['-DWF_GLOBAL_RANS_PROFILE=1']
    if args.global_rans_fast_hash:flags+=['-DWF_GLOBAL_RANS_FAST_HASH=1']
    if args.global_rans_trace:flags+=['-DWF_GLOBAL_RANS_TRACE=1']
    if args.global_rans_priority:flags+=['-DWF_GLOBAL_RANS_PRIORITY=1']
    if args.base_royal:flags+=['-DWF_COMBAT_BASE_ROYAL=1','-DWP1980_HOT_HEADER_BYTES=4096u']
    if args.tag_transport:
        flags=[f for f in flags if not f.startswith(('-DWF_R18_ROM_EXTRA_CAPACITY=','-DWF_SATURN_HWRAM_HOT_INDEX_BYTES='))]
        flags=['-I'+str(headers),*flags,'-I'+str(tag/'include'),'-DWF_COMBAT_TAG_TRANSPORT=1','-DWF_COMBAT_BANKED_TRANSPORT=1','-DWF_R18_ROM_EXTRA_CAPACITY=23u','-DWF_PLATFORM_SATURN=1']
    if args.probe:sources+=[root/'tests/session_host_audit.c',args.probe.resolve()]
    def build(source):
        obj=out/(source.stem+'.o')
        local_flags=['-Dwf_r18_video_batch=wf_video_batch_reference'] if source.name=='wf_r18_video.c' else []
        if args.hw1_banked and source.name=='wf_r17_actions_r11.c':
            # The global renderer switch must not opt this HW1 action unit
            # back into its old Os pragma. Keep recovered combat O2.
            local_flags+=['-UWF_PLATFORM_SATURN']
        compiled=source
        if args.tag_transport and source.name in ('wp2015_secondary.c','wf_r18_ring.c'):
            project=project_transition_secondary if source.name=='wp2015_secondary.c' else project_transition_ring
            compiled=out/(source.stem+'_transition.c')
            content=project(source.read_text())
            if source.name=='wp2015_secondary.c':content=project_cd_window(content)
            if source.name=='wp2015_secondary.c' and args.tag_secondary_batch2:content=project_tag_batch2(content)
            compiled.write_text(content)
        if source.name=='wf_r17_fixed_four_10120.c':
            compiled=out/'wf_fixed_four_alias.c'
            compiled.write_text(project_rope_init(source.read_text()))
        if source.name=='wf_r18_frontend_driver.c':
            # Rename only the definition: recursive/internal calls must still
            # cross the production transport hook. Locked original untouched.
            signature='wf_status_t wf_r18_frontend_driver_call(wf_r18_frontend_driver_t*d,uint32_t owner){'
            original=source.read_text();assert original.count(signature)==1
            compiled=out/'wf_frontend_transport_reference.c'
            content=original.replace(signature,signature.replace('wf_r18_frontend_driver_call','wf_frontend_call_reference'))
            if args.postmatch:content+='\n#include "wf_frontend_resume.inc"\n'
            compiled.write_text(content)
        if args.sh2 and source.name in ('wf_r18_present.c','wf_r18_hud.c','wf_saturn_yaul_backend.c','wf_saturn_memory.c'):
            local_flags+=['-DWF_PLATFORM_SATURN=1']
        replacements={
            'wf_r17_participant_helpers.c':[('static wf_participant_workspace_t participant_workspace;','#define participant_workspace (wf_workspace.combat.participant)')],
            'wf_r18_video.c':[('static wf_r17_fg0_write_t stage[4096];','#define stage ((wf_r17_fg0_write_t*)wf_workspace_video_stage())')],
            'wf_r18_present.c':[
                ('static uint8_t hot_indices[WF_SATURN_HWRAM_HOT_INDEX_BYTES] __attribute__((aligned(32)));','#define hot_indices (wf_workspace.combat.hot_indices)'),
                ('static wf_r18_sprite_record_t records[512];static wf_vdp1_frame_plan_t plan;','static wf_r18_sprite_record_t records[512];\n#define plan (wf_workspace.combat.plan)'),
                ('static uint8_t dma_stage[WF_VDP1_STREAM_TILES_MAX*WF_VDP1_TILE_BYTES_16X16_4BPP] __attribute__((aligned(32)));','#define dma_stage (wf_workspace.combat.dma_stage)')],
            'wf_saturn_yaul_backend.c':[
                ('static vdp1_cmdt_t g_cmdts[WF_VDP1_COMMANDS_MAX+4u] __aligned(32);',
                 '#define g_cmdts (wf_workspace.combat.commands)\nstatic wf_damage_t g_damage;\nvolatile uint32_t wf_clear_pixels,wf_clear_rects,wf_clear_total;'),
                ('memset(b,0,sizeof(*b));b->hwr_stage=(uint8_t*)h;b->hwr_stage_bytes=n;',
                 'memset(b,0,sizeof(*b));b->hwr_stage=(uint8_t*)h;b->hwr_stage_bytes=n;wf_damage_init(&g_damage);'),
                ('/* Map the original visible source Y8..247 onto physical Y0..239. */', '''/* Clear both old footprints before this frame. Actual normal-sprite
     * geometry below records every plotted pixel, including crop/flip. */
    wf_damage_rect_t rects[WF_DAMAGE_RECTS_MAX];
    unsigned count=wf_damage_begin(&g_damage,rects);wf_clear_pixels=0;wf_clear_rects=count;
    const vdp1_cmdt_draw_mode_t clear={.pre_clipping_disable=true,.trans_pixel_disable=true,.end_code_disable=true};
    for(unsigned i=0;i<count;i++){
        wf_damage_rect_t r=rects[i];
        int16_t x=r.x,y=r.y+8,x1=r.x+r.width-1,y1=r.y+r.height+7;
        const int16_vec2_t corners[4]={INT16_VEC2_INITIALIZER(x,y),INT16_VEC2_INITIALIZER(x1,y),INT16_VEC2_INITIALIZER(x1,y1),INT16_VEC2_INITIALIZER(x,y1)};
        vdp1_cmdt_t *cmd=&g_cmdts[g_cmd_index++];memset(cmd,0,sizeof *cmd);
        vdp1_cmdt_polygon_set(cmd);vdp1_cmdt_draw_mode_set(cmd,clear);vdp1_cmdt_vtx_set(cmd,corners);cmd->cmd_colr=0;
        wf_clear_pixels+=(uint32_t)r.width*r.height;
    }
    wf_clear_total+=wf_clear_pixels;
    /* Map the original visible source Y8..247 onto physical Y0..239. */'''),
                ('cmd->cmd_grda=cmd->reserved=0;',
                 'cmd->cmd_grda=cmd->reserved=0;wf_damage_add(&g_damage,cmd->cmd_xa,cmd->cmd_ya-8,width,height);'),
                ('words[4]=words[5]=words[6]=words[7]=0;',
                 'words[4]=words[5]=words[6]=words[7]=0;wf_damage_add(&g_damage,cmd->cmd_xa,cmd->cmd_ya-8,width,height);'),
                ('if(g_cmd_index>=WF_VDP1_COMMANDS_MAX+3u)return WF_BLOCKED;',
                 'if(g_cmd_index>=WF_COMBAT_COMMAND_CAPACITY-1u)return WF_BLOCKED;'),
                ('if(index>WF_VDP1_COMMANDS_MAX+3u||count>WF_VDP1_COMMANDS_MAX+3u-index)return WF_BLOCKED;',
                 'if(index>WF_COMBAT_COMMAND_CAPACITY-1u||count>WF_COMBAT_COMMAND_CAPACITY-1u-index)return WF_BLOCKED;'),
                ('if(g_cmd_index>=WF_VDP1_COMMANDS_MAX+4u){b->active=0;return WF_BLOCKED;}',
                 'if(g_cmd_index>=WF_COMBAT_COMMAND_CAPACITY){b->active=0;return WF_BLOCKED;}'),
                ('if(!b->defer_sync){vdp1_sync();vdp1_sync_wait();}',
                 'if(!b->defer_sync){vdp1_sync();vdp1_sync_wait();}wf_damage_commit(&g_damage);')]
        }
        if args.combat_only:
            damage_replacements=list(replacements['wf_saturn_yaul_backend.c'])
            replacements['wf_saturn_yaul_backend.c']=[('static vdp1_cmdt_t g_cmdts[WF_VDP1_COMMANDS_MAX+4u] __aligned(32);','#define g_cmdts (wf_workspace.combat.commands)')]
            if not banked:
                # Read-only instrumentation at the actual HW1 DMA sites.
                # A submitted/ACKed frame alone does not prove safe VRAM use.
                replacements['wf_saturn_yaul_backend.c'] += [
                    ('/* Each DMA contains only a consecutive run whose resident bytes changed. */',
                     'if(!wf_hw1_fence_begin()){b->active=0;return WF_BLOCKED;}\n    wp2015_vdp1_observe_begin();\n    /* Each DMA contains only a consecutive run whose resident bytes changed. */'),
                    ('scu_dma_transfer(0,(void*)VDP1_VRAM(destination),b->hwr_stage+first*128u,bytes);',
                     'if(!wf_hw1_fence_check()){b->active=0;return WF_BLOCKED;}\n        wp2015_vdp1_observe_dma(0);scu_dma_transfer(0,(void*)VDP1_VRAM(destination),b->hwr_stage+first*128u,bytes);'),
                    ('vdp1_sync_cmdt_list_put(&list,0);vdp1_sync_put_wait();vdp1_sync_render();',
                     'if(!wf_hw1_fence_check()){b->active=0;return WF_BLOCKED;}\n    wp2015_vdp1_observe_dma(1);vdp1_sync_cmdt_list_put(&list,0);vdp1_sync_put_wait();wp2015_vdp1_observe_end();wf_hw1_fence_publish();vdp1_sync_render();')]
                if args.hw1_variable:
                    replacements['wf_saturn_yaul_backend.c']=damage_replacements+[(a,b.replace('if(!wf_hw1_fence_begin()){b->active=0;return WF_BLOCKED;}','').replace('if(!wf_hw1_fence_check()){b->active=0;return WF_BLOCKED;}','').replace('wf_hw1_fence_publish();','')) for a,b in replacements['wf_saturn_yaul_backend.c'][1:]]
                    replacements['wf_saturn_yaul_backend.c'].append(('vdp1_sync_interval_set(0);','vdp1_sync_interval_set(-1);'))
                    replacements['wf_saturn_yaul_backend.c'].append(('vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);','vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);'))
        if args.tag_transport:
            replacements['wf_r18_present.c'].append(('#define WF_R18_ROM_EXTRA_CAPACITY 14u','#ifndef WF_R18_ROM_EXTRA_CAPACITY\n#define WF_R18_ROM_EXTRA_CAPACITY 14u\n#endif'))
        if args.hw1_banked and source.name=='wf_saturn_yaul_backend.c':
            compiled=out/'wf_backend_banked.c';compiled.write_text(project_backend(source.read_text(),(tag/'src'/source.name).read_text()))
            if args.cropped_cart:compiled.write_text(project_cropped_backend(compiled.read_text()))
            if args.bulk_rom_upload:compiled.write_text(project_bulk_backend(compiled.read_text()))
        if args.hw1_banked and source.name=='wp1980_vram_cache.c':
            compiled=out/'wf_cache_readers.c';compiled.write_text(project_cache(source.read_text()))
        if args.hw1_banked and source.name=='wp2015_display_pipeline.c':
            compiled=out/'wf_display_palette_lease.c';compiled.write_text(project_display(source.read_text()))
        if args.base_royal and args.hw1_banked and source.name=='wf_r18_present.c':
            compiled=out/'wf_present_small_blocks.c';compiled.write_text(project_vdp2_prefix(source.read_text()))
        if args.hw1_banked and source.name=='wf_r18_present.c':
            content=project_tile_index(compiled.read_text())
            if args.cropped_cart:content=project_cropped_present(content)
            if args.shared_crop_pool:content=project_shared_present(content)
            if args.grouped_crop_pool:content=project_grouped_present(content)
            if args.retained_crop_pool:content=project_retained_present(content)
            if args.global_rans:content=project_global_present(content)
            if args.bulk_rom_upload:content=project_bulk_present(content)
            compiled=out/'wf_present_indexed.c';compiled.write_text(content)
        if args.combat_only and os.environ.get('WF_TRACE_ALL')=='1' and source.name=='wf_r18_present.c':
            content=compiled.read_text()
            for signature,call in [('wf_status_t wf_r18_rom_read(void*u,uint32_t a,void*dest,size_t n){','wf_trace_rom(a,n);'),
                                   ('wf_status_t wf_r18_tile_read(uint16_t tile,const uint8_t **out){','wf_trace_tile(tile);')]:
                assert content.count(signature)==1
                content=content.replace(signature,signature+call)
            if args.hw1_banked:
                marker='status=wf_saturn_yaul_rom_frame_prepare(&backend,&plan);'
                assert content.count(marker)==2
                # Include requests satisfied by the warmed physical VRAM
                # cache. A decoded-tile callback alone misses these IDs.
                content=content.replace(marker,'for(unsigned trace_i=0;trace_i<plan.tile_count;trace_i++)wf_trace_tile(plan.tile_upload[trace_i].source_tile);\n '+marker)
            compiled=out/'wf_present_trace.c'
            compiled.write_text('#include <stddef.h>\n#include <stdint.h>\nvoid wf_trace_rom(uint32_t,size_t);\nvoid wf_trace_tile(uint16_t);\n'+content)
        if args.hw1_banked and source.name=='wf_r18_ring.c':
            original=source.read_text();needle=' wp2015_secondary_reset();';assert original.count(needle)==1
            compiled=out/'wf_ring_hw1.c';compiled.write_text(project_ring_prefix(original) if args.base_royal else original.replace(needle,' /* This profile has no TAG secondary lease. */'))
        if args.sh2 and source.name in replacements:
            original=compiled.read_text()
            for before,after in replacements[source.name]:
                assert original.count(before)==1,(source,before)
                original=original.replace(before,after)
            compiled=out/(source.stem+'_lease.c')
            compiled.write_text('#include "wf_workspace.h"\n'+
                               ('#include "wp2015_vdp1_observer.h"\n#include "wf_hw1_fence.h"\n' if args.combat_only and not banked and source.name=='wf_saturn_yaul_backend.c' else '')+original)
        if banked and source.name in TAG_VIEWPORT and (args.tag_transport or source.name not in ('wf_r18_present.c','wf_saturn_yaul_backend.c')):
            original=tag_viewport(source.name,compiled.read_text())
            compiled=out/(source.stem+'_viewport.c')
            compiled.write_text(original)
        if args.tag_transport and source.name=='wf_r18_present.c':
            original=project_cold_index(compiled.read_text())
            compiled=out/'wf_present_cold_index.c';compiled.write_text(original)
        if native_profile and source.name=='wf_r18_present.c':
            original=compiled.read_text()
            prefix='''#include "wf_workspace.h"
volatile uint32_t wf_render_cost[7] __attribute__((used,externally_visible));
volatile uint32_t wf_render_sum[7] __attribute__((used,externally_visible));
static unsigned render_stamp;
static void render_mark(unsigned slot){unsigned now=cpu_frt_count_get();wf_render_cost[slot]=(uint16_t)(now-render_stamp);wf_render_sum[slot]+=wf_render_cost[slot];render_stamp=now;}
'''
            markers=[
                ('busy=1;memset(&parser,0,sizeof parser);','busy=1;render_stamp=cpu_frt_count_get();memset(&parser,0,sizeof parser);'),
                ('status=submit_records(v,parser.count);','render_mark(0);status=submit_records(v,parser.count);'),
                (' status=wf_saturn_yaul_rom_frame_prepare(&backend,&plan);\n if(status!=WF_OK)goto done;\n uint16_t palette_mask=',
                 ' render_mark(1);status=wf_saturn_yaul_rom_frame_prepare(&backend,&plan);\n if(status!=WF_OK)goto done;\n render_mark(2);uint16_t palette_mask='),
                (' for(unsigned i=0;i<16;i++)if(palette_mask&(1u<<i)){status=ops->upload_palette_xbgr444_16',
                 ' render_mark(3);for(unsigned i=0;i<16;i++)if(palette_mask&(1u<<i)){status=ops->upload_palette_xbgr444_16'),
                (' status=wf_saturn_yaul_emit_sprites(&backend,plan.cmd,plan.cmd_count);',' render_mark(4);status=wf_saturn_yaul_emit_sprites(&backend,plan.cmd,plan.cmd_count);'),
                (' status=ops->end_frame(&backend);if(status!=WF_OK)goto done;\n wf_r18_video_palette_ack',
                 ' render_mark(5);status=ops->end_frame(&backend);if(status!=WF_OK)goto done;\n render_mark(6);wf_r18_video_palette_ack')]
            if args.tag_transport:
                prefix+='volatile uint32_t wf_render_sequence,wf_render_generation __attribute__((used,externally_visible));\n'
                # TAG prepares/uploads tiles in different calls from HW1.
                # Diagnostics only: retain every original statement and
                # measure parse, plan, begin, tiles, palettes, commands, end.
                markers=markers[:2]+[
                    (' uint16_t palette_mask=wf_r18_video_palette_pending(v,WF_R18_PALETTE_SPRITES);',
                     ' render_mark(1);uint16_t palette_mask=wf_r18_video_palette_pending(v,WF_R18_PALETTE_SPRITES);'),
                    (' status=wf_saturn_yaul_upload_rom_tiles(&backend,plan.tile_upload,plan.tile_count,wf_r18_tile_read,hot_crop_read);',
                     ' render_mark(2);status=wf_saturn_yaul_upload_rom_tiles(&backend,plan.tile_upload,plan.tile_count,wf_r18_tile_read,hot_crop_read);'),
                    (' for(unsigned i=0;i<16;i++)if(palette_mask&(1u<<i)){status=ops->upload_palette_xbgr444_16',
                     ' render_mark(3);for(unsigned i=0;i<16;i++)if(palette_mask&(1u<<i)){status=ops->upload_palette_xbgr444_16'),
                    (' status=wf_saturn_yaul_emit_sprites(&backend,plan.cmd,plan.cmd_count);',
                     ' render_mark(4);status=wf_saturn_yaul_emit_sprites(&backend,plan.cmd,plan.cmd_count);'),
                    (' status=ops->end_frame(&backend);if(status!=WF_OK)goto done;\n wf_r18_video_palette_ack',
                     ' render_mark(5);status=ops->end_frame(&backend);if(status!=WF_OK)goto done;\n render_mark(6);wf_r18_video_palette_ack')]
                markers[0]=(markers[0][0],markers[0][1].replace(
                    'busy=1;render_stamp=', 'busy=1;++wf_render_sequence;render_stamp='))
                markers.append(('done:busy=0;return status;}',
                    'done:wf_render_generation=status==WF_OK?r->g.frame_counter+1u:UINT32_MAX;++wf_render_sequence;busy=0;return status;}'))
                (out/'render-profile.json').write_text(json.dumps({
                    'diagnostic_only':True,
                    'publication':'Even unchanged wf_render_sequence; successful batch publishes arcade frame_counter+1, failure UINT32_MAX; attribute to an ACK only when wf_render_generation equals wf_combat_frame',
                    'slots':['metasprite parse','command plan and retained tiles','palette mask and begin_frame',
                             'upload_rom_tiles','palette uploads','emit_sprites','end_frame'],
                    'units':'uint16 FRT/32 delta; includes instrumentation and possible interrupt time',
                    'changes':[{'before':before,'after':after} for before,after in markers]
                },indent=2)+'\n')
            for before,after in markers:
                assert original.count(before)==1,(source,before)
                original=original.replace(before,after)
            compiled=out/'wf_present_profile.c';compiled.write_text(prefix+original)
        subprocess.run([cc,*flags,*local_flags,'-I'+str(source.parent),'-c',str(compiled),'-o',str(obj)],check=True)
        return obj
    if len({s.stem for s in sources})!=len(sources):raise ValueError('Duplicate object names')
    with ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(build,sources))
    if args.sh2:
        library=out/'libwf_session.a'
        # Keep GCC LTO through the final executable link, as in the approved
        # Yaul builds. An ld -r GC pass loses whole-program optimization and
        # exceeds base Saturn HWRAM once both original owners are linked.
        temporary=out/'libwf_session.new.a'
        if temporary.exists():temporary.unlink()
        subprocess.run([str(sdk/'bin/sh2eb-elf-gcc-ar'),'rcsD',str(temporary),*map(str,objects)],check=True)
        temporary.replace(library)
        output=library
    elif args.probe:
        output=out/args.probe.stem
        subprocess.run([cc,*flags,*map(str,objects),'-o',str(output)],check=True)
    else:p.error('Host requires --probe')
    report={'output':str(output),'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
            'bootable_disc':False,'native_profile':native_profile,'combat_only':args.combat_only,'postmatch':args.postmatch,'tag_secondary_batch2':args.tag_secondary_batch2,'cropped_cart':args.cropped_cart,'shared_crop_pool':args.shared_crop_pool,'grouped_crop_pool':args.grouped_crop_pool,'retained_crop_pool':args.retained_crop_pool,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],
            'flags':flags,'sources':{str(s.relative_to(root.parents[1])):{'sha256':hashlib.sha256(s.read_bytes()).hexdigest()} for s in sources}}
    if args.postmatch:
        report['frontend_resume']={str(path.relative_to(root)):{'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
                                   for path in (root/'src/wf_frontend_resume.h',root/'src/wf_frontend_resume.inc')}
    (out/'build.json').write_text(json.dumps(report,indent=2)+'\n')
    print(output)

if __name__=='__main__':main()
