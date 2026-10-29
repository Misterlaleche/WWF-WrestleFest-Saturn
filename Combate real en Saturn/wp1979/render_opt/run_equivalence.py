#!/usr/bin/env python3
"""Compile baseline/proposal independently; compare full records and plans."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys

root=Path(__file__).resolve().parent
runtime=root.parent/'native/src/runtime'
fast='--fast' in sys.argv
build=root/('build-fast' if fast else 'build')
build.mkdir(exist_ok=True)
public=[
 'wf_r18_present_backup_workspace_begin','wf_r18_present_backup_workspace_end',
 'wf_r18_present_init','wf_r18_rom_preload','wf_r18_rom_pages_preload',
 'wf_r18_rom_extra_pages_preload','wf_r18_rom_read','wf_r18_hot_sprites_load',
 'wf_r18_hot_sprites_bytes','wf_r18_tile_read','wf_r18_present_records_buffer',
 'wf_r18_present_records_commit','wf_r18_present_batch','wf_r18_present_records_count',
 'wf_r18_present_commands','wf_r18_present_frame_tiles',
 'wf_vdp1_plan_reset','wf_vdp1_plan_build','wf_vdp1_plan_validate','wf_vdp1_plan_retain_tile_slots']
flags=['-std=c11','-O2','-fno-builtin','-fno-omit-frame-pointer','-ffunction-sections','-fdata-sections',
       '-Wall','-Wextra','-Werror','-I'+str(runtime/'include')]
if '--sanitize' in sys.argv:flags+=['-O1','-g','-fsanitize=address,undefined']
if fast:flags+=['-DWF_R18_RETAIN_TEST_STATS=1']
objects=[]
pairs=[('proposed','ref'),('proposed-fast','opt')] if fast else [('baseline','ref'),('proposed','opt')]
for kind,prefix in pairs:
 wrapper=''.join(f'#define {s} {prefix}_{s}\n' for s in public)
 wrapper+=f'#include "{root/kind/"wf_r18_present.c"}"\n'
 wrapper+=f'''\nvoid {prefix}_poison(void){{
 memset(&plan,0xa5,sizeof plan);memset(plan_tile_index,0,sizeof plan_tile_index);plan_tile_epoch=0;
}}
wf_status_t {prefix}_evaluate(const wf_r18_sprite_record_t *input,size_t n,const uint16_t *resident,const uint8_t *bits,wf_vdp1_frame_plan_t *out,wf_r18_video_t *v){{
 memcpy(records,input,n*sizeof *input);memset(&backend,0,sizeof backend);
 memcpy(backend.rom_source,resident,sizeof backend.rom_source);
 memcpy(backend.resident,bits,sizeof backend.resident);memset(backend.rom_identity,255,sizeof backend.rom_identity);
 wf_status_t s=submit_records(v,n);memcpy(out,&plan,sizeof plan);return s;
}}
'''
 path=build/f'{prefix}_present_wrapper.c';path.write_text(wrapper)
 for source,defs in [(path,[]),(root/kind/'wf_vdp1_plan.c',[f'-D{s}={prefix}_{s}' for s in public if s.startswith('wf_vdp1_')])]:
  obj=build/f'{prefix}_{source.stem}.o';objects.append(obj)
  subprocess.run(['cc',*flags,*defs,'-c',str(source),'-o',str(obj)],check=True)
exe=build/('equivalence-asan' if '--sanitize' in sys.argv else 'equivalence')
subprocess.run(['cc',*flags,str(root/'equivalence.c'),*(str(o) for o in objects),'-Wl,--gc-sections','-o',str(exe)],check=True)
env=dict(os.environ)
if '--sanitize' in sys.argv:
 # The sandbox cannot read /proc/<pid>/task. This test owns no heap objects;
 # disable the incompatible leak scan, while keeping ASan and UBSan active.
 env['ASAN_OPTIONS']='detect_leaks=0'
result=subprocess.run([str(exe)],check=True,text=True,capture_output=True,env=env)
print(result.stdout,end='')
report={'command':sys.argv,'result':result.stdout.strip(),'sha256':{}}
for kind,_ in pairs:
 for path in sorted((root/kind).glob('*.c')):report['sha256'][str(path.relative_to(root))]=hashlib.sha256(path.read_bytes()).hexdigest()
(build/('equivalence-asan.json' if '--sanitize' in sys.argv else 'equivalence.json')).write_text(json.dumps(report,indent=2)+'\n')
