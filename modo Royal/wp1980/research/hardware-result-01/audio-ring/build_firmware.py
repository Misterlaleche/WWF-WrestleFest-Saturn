#!/usr/bin/env python3
"""Reproduce R18 native music/SFX build with local Linux cross compilers.

This copy exposes source/output paths and preserves the prior compiler recipe.
The upstream build_music_driver.py first
builds an unused legacy FM diagnostic profile to obtain its OKI lookup table;
this Linux driver emits that identical table directly instead. It retains the
same DSP generator, waveform, linker script and production translation units.
"""
import hashlib
import json
import math
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

import argparse

parser = argparse.ArgumentParser(description='Isolated native DSP/SFX firmware rebuild using the supplied Linux SDK.')
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--tooling', type=Path, required=True)
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
TOOLS = args.tooling.resolve()
HERE = TOOLS/'audio'
SOURCE = args.source.resolve()
OUT = args.out.resolve()
assert OUT != Path(__file__).resolve().parent and Path(__file__).resolve().parent in OUT.parents, 'Output must be below this audio-ring research directory'
OUT.mkdir(parents=True, exist_ok=True)
sys.dont_write_bytecode = True
reference_report = json.loads((HERE/'music-driver/music-build-report.json').read_text())
sys.path.insert(0, str(HERE/'reference/tools/audio/native_probe'))
from build_dsp_mixer import generate

cc = TOOLS/'host/usr/bin/m68k-linux-gnu-gcc'
hostbin = TOOLS/'host/usr/bin'
environment = dict(os.environ)
environment['PATH'] = str(hostbin)+os.pathsep+environment.get('PATH','')
environment['LD_LIBRARY_PATH'] = str(TOOLS/'host/usr/lib/x86_64-linux-gnu')+os.pathsep+str(TOOLS/'host/lib/x86_64-linux-gnu')
logs = []

def run(name, *args):
    command = [str(hostbin/('m68k-linux-gnu-'+name)), *map(str,args)]
    result = subprocess.run(command, cwd=OUT, env=environment, text=True, capture_output=True)
    logs.append(dict(command=command, exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
    (OUT/'commands.json').write_text(json.dumps(logs,indent=2)+'\n')
    if result.returncode:
        raise RuntimeError(result.stderr)
    return result.stdout

generate(OUT)
steps=(16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552)
shifts=(-1,-1,-1,-1,2,4,6,8)
lut=[]
for i,step in enumerate(steps):
    for nibble in range(16):
        delta=(step>>3)+((step>>2) if nibble&1 else 0)+((step>>1) if nibble&2 else 0)+(step if nibble&4 else 0)
        lut.append('.word %d,%d'%(-delta if nibble&8 else delta,max(0,min(48,i+shifts[nibble&7]))*64))
(OUT/'oki-lut.inc').write_text('\n'.join(lut)+'\n')
start=(SOURCE/'wf_sfx_start.S').read_text().replace('0xb000','0xb008').replace('wf_sfx_main','wf_music_main')
(OUT/'music-start.S').write_text(start)
cycle=[round(32767*math.sin(2*math.pi*i/128)) for i in range(128)]
wave=struct.pack('>4608h',*(cycle*36))
(OUT/'music-sine.pcm').write_bytes(wave)
(OUT/'music-wave.s').write_text('.section .wf_native_wave,"a"\n.balign 2\n.incbin "music-sine.pcm"\n')
(OUT/'include').mkdir(exist_ok=True)
(OUT/'include/string.h').write_text('#include <stddef.h>\nvoid *memcpy(void *,const void *,size_t);\nvoid *memset(void *,int,size_t);\nint memcmp(const void *,const void *,size_t);\n')
flags=['-m68000','-msoft-float','-ffreestanding','-fno-builtin','-fno-pic','-fno-pie','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-Os','-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-fstack-usage','-DWF_SFX_FAST_68000=1']
objects=[]
sources=['wf_scsp_sfx.c','wf_scsp_dsp.c','wf_scsp_ym.c','wf_music_firmware.c','wf_sfx_memory.c','wf_sfx_decode.S']
for name in sources:
    obj=Path(name).stem+'.o'
    run('gcc',*(flags if name.endswith('.c') else ['-m68000']),'-I',SOURCE,'-I',OUT,'-I',OUT/'include','-c',SOURCE/name,'-o',obj)
    objects.append(obj)
run('gcc','-m68000','-c','music-start.S','-o','music-start.o')
run('as','-m68000','music-wave.s','-o','music-wave.o')
objects+=['music-start.o','music-wave.o']
# Ubuntu's m68k libgcc contains a 68020 BSR.L in __divsi3 despite this
# compiler invocation targeting 68000. Rebuild the three needed helpers
# from the official, unchanged GCC 13.3.0 assembly with baseline flags.
runtime_source=HERE/'gcc-lb1sf68.S'
for helper in ['mulsi3','udivsi3','divsi3']:
    obj='libgcc-'+helper+'.o'
    run('gcc','-m68000','-msoft-float','-fno-pic','-fno-pie','-DL_'+helper,'-c',runtime_source,'-o',obj)
    objects.append(obj)
run('gcc','-m68000','-msoft-float','-nostdlib','-no-pie','-Wl,-T,'+str(SOURCE/'wf_music_m68k.ld'),'-Wl,-Map,music.map',*objects,'-o','SCSP.ELF')
assert not run('nm','-u','SCSP.ELF').strip(), 'Undefined symbols'
disassembly=run('objdump','-d','-m','m68k:68000','SCSP.ELF')
(OUT/'disassembly.txt').write_text(disassembly)
assert not re.search(r'\t(?:reset|tas|fmov|fadd|fsub|fmul|fdiv|\.short)\b|\(bad\)',disassembly), 'Invalid instruction'
symbols={}
for line in run('nm','-n','SCSP.ELF').splitlines():
    fields=line.split()
    if len(fields)==3:
        symbols[fields[2]]=int(fields[0],16)
(OUT/'symbols.json').write_text(json.dumps(symbols,indent=2)+'\n')
(OUT/'elf.txt').write_text(run('readelf','-h','-l','-S','SCSP.ELF'))
elf=(OUT/'SCSP.ELF').read_bytes()
header=struct.unpack_from('>HHIIIIIHHHHHH',elf,16)
assert elf[:7]==b'\x7fELF\x01\x02\x01' and header[0]==2 and header[1]==4 and header[6]==0x01000000
segments=[]
for i in range(header[9]):
    kind,offset,address,physical,filesz,memsz,access,align=struct.unpack_from('>8I',elf,header[4]+i*header[8])
    if kind!=1: continue
    assert address==physical and filesz<=memsz and offset+filesz<=len(elf)
    segments.append(dict(address=address,file_bytes=filesz,memory_bytes=memsz))
    if address==0x10000: assert elf[offset:offset+filesz]==wave
assert len(segments)==3 and segments[0]['address']==0 and segments[0]['memory_bytes']<=0x3000
assert segments[1]['address']==0xc100 and segments[1]['address']+segments[1]['memory_bytes']<=0x10000
assert segments[2]['address']==0x10000 and segments[2]['file_bytes']==9216
report=dict(compiler=run('gcc','--version').splitlines()[0],target=run('gcc','-dumpmachine').strip(),flags=flags,elf_bytes=len(elf),elf_sha256=hashlib.sha256(elf).hexdigest(),segments=segments,bss_end=symbols['__bss_end'],fm_slots=32,pcm_voices=4,dsp_steps=108,dsp_ram_accesses=11,production_source_changed=any(hashlib.sha256((SOURCE/n).read_bytes()).hexdigest()!=reference_report['source_sha256'][n] for n in sources),hardware_validation=False,source_sha256={n:hashlib.sha256((SOURCE/n).read_bytes()).hexdigest() for n in sources},compiler_runtime_source='https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-13.3.0/libgcc/config/m68k/lb1sf68.S',compiler_runtime_sha256=hashlib.sha256(runtime_source.read_bytes()).hexdigest(),compiler_runtime_helpers=['mulsi3','udivsi3','divsi3'],compiler_runtime_recompiled_for_68000=True)
report['reference_elf_sha256']=reference_report['elf_sha256']
report['isolated_source']=str(SOURCE)
report['isolated_output']=str(OUT)
(OUT/'music-build-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
