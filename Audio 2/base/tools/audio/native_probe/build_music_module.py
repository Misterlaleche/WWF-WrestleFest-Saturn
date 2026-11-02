"""Build the standalone slave-SH2 control module, not a disc image."""
import argparse,hashlib,json,os,subprocess,struct,re
from pathlib import Path
from generate_z80_control import generate
from compact_music_elf import compact
def build(repo,toolchain,sdk,rom,out):
 repo,toolchain,sdk,rom,out=[p.resolve() for p in (repo,toolchain,sdk,rom,out)];out.mkdir(parents=True,exist_ok=True)
 data=rom.read_bytes()
 if hashlib.sha1(data).hexdigest()!='30073963e965250d94f0dc3bd261a054850adf95':raise ValueError('Unsupported sound ROM')
 (out/'sound.rom').write_bytes(data[:0xc000]);(out/'include').mkdir(exist_ok=True)
 (out/'include/stdio.h').write_text('#include <stddef.h>\ntypedef struct FILE FILE; extern FILE *stderr; int fprintf(FILE *,const char *,...); int printf(const char *,...);\n')
 abi=int(re.search(r'WF_MUSIC_MODULE_ABI=(\d+)',(repo/'src/audio/native/wf_music_module.h').read_text()).group(1))
 (out/'module-data.S').write_text('''.section .wf_music_header,"a"
.long 0x57464d53
.word MODULE_ABI,32
.long 0x05c5c000,__music_image_end,_wf_music_module_prepare,_wf_music_module_run,0x25c7f000,0
.section .rodata.music_rom,"a"
.global _wf_music_sound_rom
_wf_music_sound_rom:
.incbin "sound.rom"
'''.replace('MODULE_ABI',str(abi)))
 cc=toolchain/'bin/sh-elf-gcc.exe';binutils=toolchain/'sh-elf/bin';env=dict(os.environ);env['PATH']=str(toolchain/'bin')+os.pathsep+env['PATH'];logs=[]
 def run(cmd):
  r=subprocess.run(list(map(str,cmd)),cwd=out,env=env,text=True,capture_output=True);logs.append(dict(command=list(map(str,cmd)),exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr));(out/'commands.json').write_text(json.dumps(logs,indent=2)+'\n')
  if r.returncode:raise RuntimeError(r.stderr)
  return r.stdout
 native_report=generate(repo,rom,out/'z80-native.c')
 src=repo/'src/audio/native';flags=['-m2','-mb','-Os','-std=c11','-ffreestanding','-fno-builtin','-ffunction-sections','-fdata-sections','-fstack-usage','-Wall','-Wextra','-Werror','-B',str(binutils)+os.sep,'-I',out/'include','-I',src,'-isystem',sdk/'sh2eb-elf/include']
 objects=[]
 for name in ['wf_music_seq.c','wf_music_module.c','wf_music_module_memory.c','wf_scsp_ym.c','reference_driver/z80.c']:
  obj=Path(name).stem+'.o';use=list(flags)+['-DWF_MUSIC_NATIVE_CONTROL=1','-I',src/'reference_driver']
  if name in ('wf_music_seq.c','reference_driver/z80.c'):use=[f for f in use if f!='-Os']+['-O2']
  if name.startswith('reference_driver/'):use.remove('-Werror')
  run([cc,*use,'-c',(out/'z80-native.c' if name=='reference_driver/z80.c' else src/name),'-o',obj]);objects.append(obj)
 run([cc,'-m2','-mb','-B',str(binutils)+os.sep,'-c','module-data.S','-o','module-data.o']);objects.append('module-data.o')
 libgcc=sdk/'lib/gcc/sh2eb-elf/13.2.0/libgcc.a'
 run([binutils/'ld.exe','--gc-sections','-static','-T',src/'wf_music_module.ld','-Map','module.map',*objects,libgcc,'-o','MUSIC.ELF'])
 if run([binutils/'nm.exe','-u','MUSIC.ELF']).strip():raise ValueError('Undefined module symbols')
 (out/'symbols.txt').write_text(run([binutils/'nm.exe','-n','MUSIC.ELF']))
 (out/'MUSIC.DEBUG.ELF').write_bytes((out/'MUSIC.ELF').read_bytes())
 (out/'MUSIC.ELF').write_bytes(compact((out/'MUSIC.DEBUG.ELF').read_bytes()))
 (out/'elf.txt').write_text(run([binutils/'readelf.exe','-h','-l','-S','MUSIC.ELF']))
 elf=(out/'MUSIC.ELF').read_bytes();h=struct.unpack_from('>HHIIIIIHHHHHH',elf,16);segs=[]
 for i in range(h[9]):
  p=struct.unpack_from('>8I',elf,h[4]+i*h[8]);segs.append(dict(type=p[0],address=p[2],file_bytes=p[4],memory_bytes=p[5]))
 report=dict(elf_bytes=len(elf),elf_sha256=hashlib.sha256(elf).hexdigest(),segments=segs,entry=h[3],rom_bytes=0xc000,rom_sha256=hashlib.sha256(data).hexdigest(),synthesizer_opm_linked=False,native_control=native_report,module_abi=abi,lookahead_fs=4096,lookahead_ms=4096*1000/44100,debug_elf_bytes=(out/'MUSIC.DEBUG.ELF').stat().st_size,bin_cue_iso_generated=False,physical_validation=False)
 (out/'module-build-report.json').write_text(json.dumps(report,indent=2)+'\n');return report
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
 for n in ['toolchain','sdk','rom','out']:p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();print(json.dumps(build(a.repo,a.toolchain,a.sdk,a.rom,a.out),indent=2))
