"""Rebuild pixel/DMA/ROM-reader checks with assertions enabled."""
from pathlib import Path
import json, subprocess
root=Path(__file__).resolve().parents[2]; runtime=root/'runtime18/src/runtime'
out=root/'runtime18/validation/audit-20261025';out.mkdir(exist_ok=True)
cc=str(root/'toolchains/host/cc.exe')
hot=(runtime/'audit/runtime18-cd/HOTSPR.BIN').read_bytes()
count=int.from_bytes(hot[4:6],'big');prefix=24 if hot[:4]==b'R18D' else 8
header_size=16384 if count>4092 else 8192 if count>2044 else 4096
header=bytearray(header_size);header[:4]=b'R18H';header[4:8]=hot[4:8];header[8:8+count*2]=hot[prefix:prefix+count*2]
source=(runtime/'audit/runtime18-cd/SPRITES.PAK').read_bytes()
ids=[int.from_bytes(hot[prefix+i*2:prefix+i*2+2],'big') for i in range(count)]
fixture=out/'crop-raw-fixture.bin';fixture.write_bytes(header+b''.join(source[i*128:(i+1)*128] for i in ids))
common=['src/wf_saturn_memory.c','src/wf_saturn_render_bridge.c','src/wf_vdp1_plan.c']
cases={
 'sprite-crop':['-DWF_PLATFORM_SATURN','-Itests/r18/mock-yaul','tests/r18/test_sprite_crop.c',*common],
 'dma':['-DWF_PLATFORM_SATURN','-Itests/r18/mock-yaul','src/wf_saturn_yaul_backend.c',*common,'tests/r18/test_yaul_dma.c'],
 'rom-read-work':['tests/r18/test_rom_read_work.c','src/wf_r18_metasprite.c','src/wf_actor.c','src/wf_saturn_yaul_backend.c',*common]}
results=[]
for name,args in cases.items():
    exe=out/(name+'.exe');cmd=[cc,'-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*args,'-o',str(exe)]
    c=subprocess.run(cmd,cwd=runtime,capture_output=True,text=True);log=repr(cmd)+'\n'+c.stdout+c.stderr
    if c.returncode==0:
        c=subprocess.run([str(exe),*([str(fixture)] if name=='sprite-crop' else [])],cwd=runtime,capture_output=True,text=True);log+=c.stdout+c.stderr
    (out/(name+'.log')).write_text(log);results.append(dict(test=name,exit=c.returncode,output=log));print(name,c.returncode,log[-600:])
(out/'targeted-results.json').write_text(json.dumps(results,indent=2))
raise SystemExit(any(x['exit'] for x in results))
