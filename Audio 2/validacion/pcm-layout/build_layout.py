"""Compile offsetof/sizeof with the exact existing68000 toolchain; no firmware build."""
from pathlib import Path
import hashlib,importlib.util,json,os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;VALIDATION=HERE.parent;REPAIR=VALIDATION.parents[1];RUNTIME=REPAIR.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
header=RUNTIME/'src/audio/native/wf_scsp_sfx.h';native=HERE/'native';native.mkdir(exist_ok=True);(native/header.name).write_bytes(header.read_bytes())
probe=HERE/'probe.c';probe.write_text('#include "wf_scsp_sfx.h"\nconst unsigned wf_pcm_counter_layout[]={sizeof(wf_scsp_sfx_t),offsetof(wf_scsp_sfx_t,underruns),sizeof(((wf_scsp_sfx_t*)0)->underruns)};\n')
commands=json.loads((REPAIR/'firmware/commands.json').read_text());old=commands[0]['command'];compiler=Path(old[0]);flags=old[1:old.index('-I')]
environment=os.environ.copy();environment['PATH']=str(compiler.parent)+os.pathsep+environment['PATH']
version=subprocess.run([str(compiler),'--version'],capture_output=True,text=True,env=environment);version.check_returncode()
cmd=[str(compiler),*flags,'-I',str(native),'-S',str(probe),'-o',str(HERE/'probe.s')]
r=subprocess.run(cmd,cwd=HERE,env=environment,capture_output=True,text=True);(HERE/'build.log').write_text(r.stdout+r.stderr);r.check_returncode()
values=[int(x,0)for x in re.findall(r'^\s*\.long\s+(0x[0-9a-fA-F]+|\d+)\s*$',(HERE/'probe.s').read_text(),re.M)]
assert len(values)==3,values
spec=importlib.util.spec_from_file_location('observe_pcm_layout',VALIDATION/'observe_cold.py');observe=importlib.util.module_from_spec(spec);spec.loader.exec_module(observe)
sys.path.insert(0,str(REPAIR/'emulator'));from cold_overlay import Elf
profiles=[('music',REPAIR/'firmware/SCSP.ELF',REPAIR/'final/SCSP.ELF',0x4d46),('sfx',REPAIR/'firmware/legacy-profile/SCSP.ELF',REPAIR/'firmware/legacy-profile/SCSP.ELF',0x53f1)]
summary=[]
for name,debug_path,release_path,profile in profiles:
 debug,release=Elf(debug_path),Elf(release_path);assert 'wf_sfx_engine'in debug.symbols
 address,symbol_bytes=debug.symbols['wf_sfx_engine'];assert symbol_bytes==values[0],(name,symbol_bytes,values)
 layout=dict(schema='wf_scsp_sfx_counter_v1',native_profile=profile,debug_elf_sha256=sha(debug_path),release_elf_sha256=sha(release_path),load_segments_sha256=observe.scsp_load_digest(debug),engine_symbol='wf_sfx_engine',engine_bytes=values[0],underruns_offset=values[1],underruns_bytes=values[2],compiler_id=version.stdout.splitlines()[0],compiler_sha256=sha(compiler),header_sha256=sha(header),probe_source_sha256=sha(probe),compiler_command=cmd,debug_path=str(debug_path),release_path=str(release_path))
 path=HERE/(name+'-layout.json');path.write_text(json.dumps(layout,indent=2)+'\n')
 binding=observe.bind_scsp_counter(debug,release,layout,sha(path));summary.append(dict(profile=name,layout=str(path),**binding))
(HERE/'RESULT.json').write_text(json.dumps(dict(passed=True,compiled_sizeof_offsetof=values,bindings=summary,firmware_modified=False,emulator_run=False,disc_images_created=False),indent=2)+'\n');print(json.dumps(dict(passed=True,values=values,bindings=summary),indent=2))
