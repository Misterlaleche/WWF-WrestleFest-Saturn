#!/usr/bin/env python3
"""Build the shared session with locked HW1 owners; never write upstream."""
import argparse, hashlib, json, os, re, subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--probe',type=Path)
    p.add_argument('--sanitize',action='store_true')
    p.add_argument('--sh2',action='store_true')
    p.add_argument('--entry',action='append',default=[],help='Additional native public root, without underscore')
    args=p.parse_args()
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
    out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    flags=['-std=c11','-Wall','-Wextra','-Werror','-O2','-I'+str(native/'include'),
           '-I'+str(root/'src'),'-DWF_R18_ROM_EXTRA_CAPACITY=14u',
           '-DWF_SATURN_HWRAM_HOT_INDEX_BYTES=0x14000u']
    cc=os.environ.get('CC','cc')
    if args.sh2:
        if args.probe or args.sanitize:p.error('--sh2 builds a core archive, without host probes/sanitizers')
        sdk=Path(os.environ['YAUL_INSTALL_ROOT']);cc=str(sdk/'bin/sh2eb-elf-gcc')
        if subprocess.check_output([cc,'-dumpfullversion'],text=True).strip()!='13.2.0':
            raise ValueError('Expected supplied SH-2 GCC13.2.0')
        flags+=['-Os','-m2','-mb','-flto','-ffreestanding','-ffunction-sections','-fdata-sections',
                '-I'+str(sdk/'sh2eb-elf/include'),'-I'+str(sdk/'sh2eb-elf/include/yaul'),'-I'+str(root/'src/saturn'),'-DWP1980_DISABLE_MUSIC=1']
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    if args.probe:sources+=[root/'tests/session_host_audit.c',args.probe.resolve()]
    def build(source):
        obj=out/(source.stem+'.o')
        local_flags=['-Dwf_r18_video_batch=wf_video_batch_reference'] if source.name=='wf_r18_video.c' else []
        compiled=source
        if source.name=='wf_r18_frontend_driver.c':
            # Rename only the definition: recursive/internal calls must still
            # cross the production transport hook. Locked original untouched.
            signature='wf_status_t wf_r18_frontend_driver_call(wf_r18_frontend_driver_t*d,uint32_t owner){'
            original=source.read_text();assert original.count(signature)==1
            compiled=out/'wf_frontend_transport_reference.c'
            compiled.write_text(original.replace(signature,signature.replace('wf_r18_frontend_driver_call','wf_frontend_call_reference')))
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
                ('static vdp1_cmdt_t g_cmdts[WF_VDP1_COMMANDS_MAX+4u] __aligned(32);','#define g_cmdts (wf_workspace.combat.commands)'),
                ('vdp1_sync_interval_set(0);vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);','vdp1_sync_interval_set(-1);vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);'),
                ('/* Map the original visible source Y8..247 onto physical Y0..239. */', '''/* Explicit complete clear, same transport proven by Entrada R02.
     * Local Y=-8: polygon Y8..247 covers physical Y0..239. */
    const vdp1_cmdt_draw_mode_t clear={.pre_clipping_disable=true,.trans_pixel_disable=true,.end_code_disable=true};
    const int16_vec2_t corners[4]={INT16_VEC2_INITIALIZER(0,8),INT16_VEC2_INITIALIZER(319,8),INT16_VEC2_INITIALIZER(319,247),INT16_VEC2_INITIALIZER(0,247)};
    vdp1_cmdt_t *clear_cmd=&g_cmdts[g_cmd_index++];memset(clear_cmd,0,sizeof *clear_cmd);
    vdp1_cmdt_polygon_set(clear_cmd);vdp1_cmdt_draw_mode_set(clear_cmd,clear);vdp1_cmdt_vtx_set(clear_cmd,corners);clear_cmd->cmd_colr=0;
    /* Map the original visible source Y8..247 onto physical Y0..239. */''')]
        }
        if args.sh2 and source.name in replacements:
            original=source.read_text()
            for before,after in replacements[source.name]:
                assert original.count(before)==1,(source,before)
                original=original.replace(before,after)
            compiled=out/(source.stem+'_lease.c')
            compiled.write_text('#include "wf_workspace.h"\n'+original)
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
            'bootable_disc':False,'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],
            'flags':flags,'sources':{str(s.relative_to(root.parents[1])):{'sha256':hashlib.sha256(s.read_bytes()).hexdigest()} for s in sources}}
    (out/'build.json').write_text(json.dumps(report,indent=2)+'\n')
    print(output)

if __name__=='__main__':main()
