"""Build the32-FM-slot + DSP effects firmware to ELF only."""
import argparse,hashlib,json,math,os,re,subprocess,struct
from pathlib import Path
from build_m68k import FLAGS
from build_sfx_driver import build_sfx
from build_driver import load_segments
from build_dsp_mixer import generate
def build_music(repo,toolchain,instrument,out):
 repo,toolchain,instrument,out=[p.resolve() for p in (repo,toolchain,instrument,out)]
 build_sfx(repo,toolchain,instrument,out/'legacy-profile')
 out.mkdir(parents=True,exist_ok=True);generate(out)
 (out/'oki-lut.inc').write_bytes((out/'legacy-profile/oki-lut.inc').read_bytes())
 start=(repo/'src/audio/native/wf_sfx_start.S').read_text().replace('0xb000','0xb008').replace('wf_sfx_main','wf_music_main')
 (out/'music-start.S').write_text(start)
 cycle=[round(32767*math.sin(2*math.pi*i/128)) for i in range(128)]
 wave=struct.pack('>4608h',*(cycle*36));(out/'music-sine.pcm').write_bytes(wave)
 (out/'music-wave.s').write_text('.section .wf_native_wave,"a"\n.balign 2\n.incbin "music-sine.pcm"\n')
 env=dict(os.environ);env['PATH']=str(toolchain/'bin')+os.pathsep+env.get('PATH','');logs=[]
 def run(name,*args):
  cmd=[str(toolchain/'bin'/('m68k-elf-'+name+'.exe')),*map(str,args)]
  r=subprocess.run(cmd,cwd=out,env=env,text=True,capture_output=True)
  logs.append(dict(command=cmd,exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr));(out/'commands.json').write_text(json.dumps(logs,indent=2)+'\n')
  if r.returncode:raise RuntimeError(r.stderr)
  return r.stdout
 def rel(p):return Path(os.path.relpath(p,out)).as_posix()
 src=repo/'src/audio/native';objects=[]
 for name in ['wf_scsp_sfx.c','wf_scsp_dsp.c','wf_scsp_ym.c','wf_music_firmware.c','wf_sfx_memory.c','wf_sfx_decode.S']:
  obj=Path(name).stem+'.o';objects.append(obj)
  run('gcc',*(FLAGS+['-DWF_SFX_FAST_68000=1'] if name.endswith('.c') else ['-m68000']),'-I',rel(src),'-I','.', '-c',rel(src/name),'-o',obj)
 run('gcc','-m68000','-c','music-start.S','-o','music-start.o');run('as','-m68000','music-wave.s','-o','music-wave.o')
 objects+=['music-start.o','music-wave.o']
 run('gcc','-m68000','-msoft-float','-nostdlib','-Wl,-T,'+rel(src/'wf_music_m68k.ld'),'-Wl,-Map,music.map',*objects,'-lgcc','-o','SCSP.ELF')
 if run('nm','-u','SCSP.ELF').strip():raise ValueError('Undefined symbols')
 syms={}
 for line in run('nm','-n','SCSP.ELF').splitlines():
  p=line.split()
  if len(p)==3:syms[p[2]]=int(p[0],16)
 (out/'symbols.json').write_text(json.dumps(syms,indent=2)+'\n')
 dis=run('objdump','-d','-m','m68k:68000','SCSP.ELF');(out/'disassembly.txt').write_text(dis)
 if re.search(r'\t(?:reset|tas|fmov|fadd|fsub|fmul|fdiv)\b|\(bad\)',dis):raise ValueError('Invalid instruction')
 segs=load_segments(out/'SCSP.ELF')
 if segs[-1][1]!=wave:raise ValueError('Wave mismatch')
 (out/'elf.txt').write_text(run('readelf','-h','-l','-S','SCSP.ELF'))
 dsp=json.loads((out/'dsp-program.json').read_text())
 report=dict(elf_bytes=(out/'SCSP.ELF').stat().st_size,elf_sha256=hashlib.sha256((out/'SCSP.ELF').read_bytes()).hexdigest(),segments=[dict(address=a,file_bytes=len(b),memory_bytes=n) for a,b,n in segs],bss_end=syms['__bss_end'],fm_slots=32,pcm_voices=4,dsp_steps=dsp['steps'],dsp_ram_accesses=dsp['ram_accesses'],wave_period=128,wave_guard_cycles=18,bin_cue_iso_generated=False,hardware_validation=False)
 (out/'music-build-report.json').write_text(json.dumps(report,indent=2)+'\n');return report
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
 for n in ['toolchain','instrument','out']:p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();print(json.dumps(build_music(a.repo,a.toolchain,a.instrument,a.out),indent=2))
