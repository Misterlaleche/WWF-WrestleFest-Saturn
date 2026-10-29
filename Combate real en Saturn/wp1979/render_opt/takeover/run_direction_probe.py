#!/usr/bin/env python3
"""Causal red/green check: physical pad mapper -> actual ROM direction -> native physics."""
from pathlib import Path
import hashlib
import json
import re
import subprocess

here=Path(__file__).resolve().parent;project=here.parents[1]
runtime=project/'native/src/runtime';build=here/'build';build.mkdir(exist_ok=True)
source_paths={'baseline':here/'wp1979_entry-baseline.c','proposed':here/'wp1979_entry-proposed.c'}
for label,path in source_paths.items():
 source=path.read_text()
 match=re.search(r'static uint8_t controls\(const smpc_peripheral_digital_t \*sample\)\n\{.*?\n\}',source,re.S)
 assert match
 (build/f'controls-{label}.inc').write_text(match[0].replace('controls(',label+'_controls(',1)+'\n')
flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-ffunction-sections','-fdata-sections',
       '-I'+str(runtime/'include'),'-I'+str(build)]
exe=build/'probe_direction'
subprocess.run(['cc',*flags,str(here/'probe_direction.c'),str(runtime/'src/wf_input.c'),
 str(runtime/'src/wf_physics.c'),str(runtime/'generated/wf_physics_tables.c'),'-Wl,--gc-sections','-o',str(exe)],check=True)
rom=project/'audit_gameplay/assets/ROM68K.BIN'
results={}
for label in source_paths:
 run=subprocess.run([str(exe),str(rom),label],text=True,capture_output=True)
 print(run.stdout,end='');results[label]={'exit_code':run.returncode,'stdout':run.stdout}
core=project/'tooling/yabause-scsp-dsp/yabause/src'
peripheral=(core/'peripheral.c').read_text()
libretro=(core/'libretro/libretro.c').read_text()
sdk=project/'tooling/sh2eb-elf/sh2eb-elf/include/yaul/smpc/peripheral.h'
header=sdk.read_text()
route={}
for name,retro_id in [('Up',4),('Down',5)]:
 mask=int(re.search(r'void PerPad'+name+r'Pressed\(PerPad_struct \* pad\)\s*\{\s*\*pad->padbits &= 0x([A-Fa-f0-9]+)',peripheral)[1],16)
 bit=(255^mask)<<8
 sdk_bit=int(re.search(r'#define PERIPHERAL_DIGITAL_'+name.upper()+r'\s+0x([A-Fa-f0-9]+)',header)[1],16)
 assert bit==sdk_bit
 assert re.search(r'RETRO_DEVICE_ID_JOYPAD_'+name.upper()+r'\)\)\s*PerKeyDown\(\(i << 8\) \+ PERPAD_'+name.upper(),libretro)
 route[name.lower()]={'libretro_id':retro_id,'core_pad_byte_active_low':hex(mask),
                      'yaul_pressed_mask_after_inversion':hex(bit),'sdk_declared_mask':hex(sdk_bit)}
report={'status':'red-green-confirmed' if results['baseline']['exit_code']==1 and results['proposed']['exit_code']==0 else 'unexpected',
        'results':results,'physical_route':route,'ROM68K_sha256':hashlib.sha256(rom.read_bytes()).hexdigest(),
        'source_sha256':{str(path.relative_to(project)):hashlib.sha256(path.read_bytes()).hexdigest() for path in source_paths.values()},
        'note':'Host mock supplies named pressed fields. Actual core and SDK raw-bit definitions independently confirm those names preserve Up/Down; no emulated-memory writes or Saturn build.'}
(here/'direction-red-green.json').write_text(json.dumps(report,indent=2)+'\n')
assert report['status']=='red-green-confirmed',report
