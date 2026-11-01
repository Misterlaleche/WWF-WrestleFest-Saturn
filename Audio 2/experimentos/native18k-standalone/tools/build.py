"""Build this pinned ABI3 diagnostic; derive its owned workspace from the master."""
from pathlib import Path
import argparse,json,os,re,struct,subprocess
from compact import compact
from elf import Elf
from generate import generate
from pins import load,sha,verify_rom,verify_sources

def binary(folder,name):
 for suffix in ('.exe',''):
  path=folder/(name+suffix)
  if path.is_file():return path
 raise FileNotFoundError(str(folder/name))

def bind_master(package,master,pin):
 if sha(master.read_bytes())!=pin['master_sha256']:raise ValueError('Master SHA256 mismatch; no other master may own this window')
 e=Elf(master);start=e.symbols['___wf_music_hot_start'][0];end=e.symbols['___wf_music_hot_end'][0]
 private=e.symbols['___wf_sdk_private_pool_start'][0]
 bottom=e.symbols['___wf_slave_stack_bottom'][0];top=e.symbols['___slave_stack'][0]
 if not(start-private==14336 and end-start==18432 and bottom==end and top-bottom==8192):
  raise ValueError('Master partition contract mismatch')
 module=(package/'src/audio/native/wf_music_module.c').read_text()
 if module.count(str(end-start)+'u')!=2:raise ValueError('Prepare capacity is not the capacity owned by this master')
 return start,end

def build(package,toolchain,sdk,rom,out,master,compiler_prefix):
 package,toolchain,sdk,rom,out,master=map(lambda p:Path(p).resolve(),(package,toolchain,sdk,rom,out,master))
 pin=load(package);verify_sources(package,pin);data=verify_rom(rom,pin)
 start,end=bind_master(package,master,pin)
 if compiler_prefix!=pin['compiler_prefix']:raise ValueError('Compiler prefix differs from this pinned snapshot')
 cc=binary(toolchain/'bin',compiler_prefix+'-gcc');binutils=toolchain/compiler_prefix/'bin'
 libgcc=sdk/pin['libgcc_relative']
 if sha(cc.read_bytes())!=pin['compiler_sha256']:raise ValueError('Compiler executable differs from this pinned snapshot')
 if sha(libgcc.read_bytes())!=pin['libgcc_sha256']:raise ValueError('libgcc differs from this pinned snapshot')
 out.mkdir(parents=True,exist_ok=True);(out/'include').mkdir(exist_ok=True)
 (out/'sound.rom').write_bytes(data[:0xc000])
 (out/'include/stdio.h').write_text('#include <stddef.h>\ntypedef struct FILE FILE; extern FILE *stderr; int fprintf(FILE *,const char *,...); int printf(const char *,...);\n')
 src=package/'src/audio/native'
 abi=int(re.search(r'WF_MUSIC_MODULE_ABI=(\d+)',(src/'wf_music_module.h').read_text()).group(1))
 if abi!=pin['module_abi']:raise ValueError('Module ABI changed')
 link=(package/'linker/module.ld.in').read_text().replace('@HOT_START@',hex(start)).replace('@HOT_END@',hex(end)).replace('@HOT_CAPACITY@',str(end-start))
 if '@HOT_'in link:raise ValueError('Unresolved workspace binding')
 (out/'module.ld').write_text(link)
 (out/'module-data.S').write_text('''.section .wf_music_header,"a"
.long 0x57464d53
.word MODULE_ABI,32
.long 0x05c5c000,__music_image_end,_wf_music_module_prepare,_wf_music_module_run,0x25c7f000,0
.section .rodata.music_rom,"a"
.global _wf_music_sound_rom
_wf_music_sound_rom:
.incbin "sound.rom"
'''.replace('MODULE_ABI',str(abi)))
 env=dict(os.environ);env['PATH']=str(toolchain/'bin')+os.pathsep+env['PATH'];commands=[]
 def run(command):
  r=subprocess.run(list(map(str,command)),cwd=out,env=env,text=True,capture_output=True)
  commands.append(dict(command=list(map(str,command)),exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr))
  (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(r.stdout+r.stderr)
  return r.stdout
 compiler=dict(prefix=compiler_prefix,path=str(cc),sha256=sha(cc.read_bytes()),version=run([cc,'--version']),
  target=run([cc,'-dumpmachine']).strip(),binutils=str(binutils),libgcc_path=str(libgcc),libgcc_sha256=sha(libgcc.read_bytes()))
 native=generate(package,rom,out/'z80-native.c')
 flags=['-m2','-mb','-Os','-std=c11','-ffreestanding','-fno-builtin','-ffunction-sections','-fdata-sections','-fstack-usage',
  '-Wall','-Wextra','-Werror','-B',str(binutils)+os.sep,'-I',out/'include','-I',src,'-isystem',sdk/'sh2eb-elf/include']
 objects=[]
 for name in ('wf_music_seq.c','wf_music_module.c','wf_music_module_memory.c','wf_scsp_ym.c','reference_driver/z80.c','reference_driver/z80-cold.c'):
  obj=Path(name).stem+'.o';use=list(flags)+['-DWF_MUSIC_NATIVE_CONTROL=1','-I',src/'reference_driver']
  if name in ('wf_music_seq.c','reference_driver/z80.c'):use=[f for f in use if f!='-Os']+['-O2']
  if name.startswith('reference_driver/'):use.remove('-Werror')
  source=out/'z80-native.c'if name=='reference_driver/z80.c'else(out/'z80-cold.c'if name=='reference_driver/z80-cold.c'else src/name)
  run([cc,*use,'-c',source,'-o',obj]);objects.append(obj)
 run([cc,'-m2','-mb','-B',str(binutils)+os.sep,'-c',src/'wf_music_entry.S','-o','wf_music_entry.o']);objects.append('wf_music_entry.o')
 run([cc,'-m2','-mb','-B',str(binutils)+os.sep,'-c','module-data.S','-o','module-data.o']);objects.append('module-data.o')
 run([binary(binutils,'ld'),'--gc-sections','-static','-T',out/'module.ld','-Map','module.map',*objects,libgcc,'-o','MUSIC.ELF'])
 if run([binary(binutils,'nm'),'-u','MUSIC.ELF']).strip():raise ValueError('Undefined module symbols')
 (out/'MUSIC.DEBUG.ELF').write_bytes((out/'MUSIC.ELF').read_bytes())
 (out/'MUSIC.ELF').write_bytes(compact((out/'MUSIC.DEBUG.ELF').read_bytes()))
 debug=Elf(out/'MUSIC.DEBUG.ELF');transport=Elf(out/'MUSIC.ELF')
 if debug.symbols['_wf_hotcode_start'][0]!=start or debug.symbols['_wf_hotcode_end'][0]+pin['hot_state_bytes']>end:raise ValueError('Module outside owned window')
 if transport.loads()!=pin['expected_transport_loads']or debug.loads()!=pin['expected_debug_loads']:
  raise ValueError('ELF PT_LOAD bytes/ranges changed from the accepted snapshot')
 if sha(transport.data)!=pin['expected_compact_elf_sha256']:raise ValueError('Compact ELF differs from the accepted snapshot')
 trampoline=debug.symbol_bytes('_wf_music_module_run');body=debug.symbols['_wf_music_module_run_body'][0]
 if trampoline!=bytes.fromhex('d002e1112010d002402b0009fffffe92')+body.to_bytes(4,'big'):raise ValueError('Cache-coherent run trampoline changed')
 # Compile layout with this exact compiler; never infer SH2 offsets from HOST.
 layout_expr={
  'seq_bytes':'sizeof(wf_music_seq_t)','ym_bytes':'sizeof(wf_scsp_ym_t)',
  'hot_state_bytes':'sizeof(wf_music_seq_t)+sizeof(wf_scsp_ym_t)',
  'io_epoch_offset':'offsetof(wf_music_seq_t,io_epoch)','cycles_offset':'offsetof(wf_music_seq_t,cycles)',
  'last_cpu_offset':'offsetof(wf_music_seq_t,last_cpu)','steps_offset':'offsetof(wf_music_seq_t,steps)',
  'skipped_offset':'offsetof(wf_music_seq_t,skipped)','cpu_userdata_offset':'offsetof(z80,userdata)',
  'synth_write16_offset':'offsetof(wf_scsp_ym_t,write16)','synth_writes_offset':'offsetof(wf_scsp_ym_t,writes)',
  'shared_bytes':'sizeof(wf_music_module_shared_t)','workspace_offset':'offsetof(wf_music_module_shared_t,workspace)'}
 (out/'layout-probe.c').write_text('#include "wf_music_seq.h"\n#include "wf_scsp_ym.h"\n#include "wf_music_module.h"\nconst unsigned wf_layout_probe[]={'+','.join(layout_expr.values())+'};\n')
 run([cc,*flags,'-c',out/'layout-probe.c','-o','layout-probe.o'])
 raw=Elf(out/'layout-probe.o').symbol_bytes('_wf_layout_probe')
 layout=dict(zip(layout_expr,struct.unpack('>'+str(len(layout_expr))+'I',raw)))
 if any(layout[key]!=pin[key]for key in ('hot_state_bytes','shared_bytes','workspace_offset')):raise ValueError('Exact compiler layout changed')
 (out/'STATE-LAYOUT.json').write_text(json.dumps(dict(layout,compiler=compiler),indent=2)+'\n')
 report=dict(snapshot=pin['snapshot'],master_sha256=pin['master_sha256'],compiler=compiler,module_abi=abi,
  hot_start=hex(start),hot_end=hex(end),window_bytes=end-start,hot_code_bytes=debug.symbols['_wf_hotcode_end'][0]-start,
  state_bytes=layout['hot_state_bytes'],workspace_spare=end-debug.symbols['_wf_hotcode_end'][0]-layout['hot_state_bytes'],
  elf_sha256=sha(transport.data),elf_bytes=len(transport.data),transport_loads=transport.loads(),debug_loads=debug.loads(),
  exact_PT_LOAD_comparison_passed=True,exact_compact_ELF_comparison_passed=True,native_control=native,
  no_algorithm_changes=True,no_audio_repair_claim=True,physical_validation=False,media_created=False)
 (out/'BUILD-RESULT.json').write_text(json.dumps(report,indent=2)+'\n');return report

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--package',type=Path,default=Path(__file__).resolve().parents[1])
 for name in ('toolchain','sdk','rom','out','master'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--compiler-prefix',required=True)
 a=p.parse_args();print(json.dumps(build(a.package,a.toolchain,a.sdk,a.rom,a.out,a.master,a.compiler_prefix),indent=2))
