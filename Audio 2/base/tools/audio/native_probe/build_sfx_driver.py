"""Build resident SFX + first FM instrument firmware, ELF only."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from build_m68k import build,FLAGS
from build_driver import load_segments

def build_sfx(repo,toolchain,instrument,out):
    repo,toolchain,instrument,out=[p.resolve() for p in (repo,toolchain,instrument,out)]
    build(repo,toolchain,instrument,out)
    env=dict(os.environ);env['PATH']=str(toolchain/'bin')+os.pathsep+env.get('PATH','')
    logs=[]
    def run(name,*args):
        cmd=[str(toolchain/'bin'/('m68k-elf-'+name+('.exe' if os.name=='nt' else ''))),*map(str,args)]
        r=subprocess.run(cmd,cwd=out,env=env,text=True,capture_output=True)
        logs.append(dict(command=cmd,exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr))
        (out/'sfx-commands.json').write_text(json.dumps(logs,indent=2)+'\n')
        if r.returncode:raise RuntimeError(r.stderr)
        return r.stdout
    def rel(p):return Path(os.path.relpath(p,out)).as_posix()
    src=repo/'src/audio/native'
    # Relocate SA and wave only: every FM EG/pitch/modulation word is preserved.
    patch=(out/'observed-patch.c').read_text().replace('0x70000u','0x10000u').replace('0x0027u','0x0021u')
    (out/'sfx-patch.c').write_text(patch)
    wave=(out/'observed-wave.s').read_text().replace('.rodata.wf_scsp_wave','.wf_native_wave')
    (out/'sfx-wave.s').write_text(wave)
    steps=(16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552)
    shifts=(-1,-1,-1,-1,2,4,6,8);lut=[]
    for i,step in enumerate(steps):
        for nibble in range(16):
            delta=(step>>3)+((step>>2) if nibble&1 else 0)+((step>>1) if nibble&2 else 0)+(step if nibble&4 else 0)
            lut.append('.word %d,%d'%(-delta if nibble&8 else delta,max(0,min(48,i+shifts[nibble&7]))*64))
    (out/'oki-lut.inc').write_text('\n'.join(lut)+'\n')
    objects=[]
    for source in [src/'wf_scsp_sfx.c',src/'wf_sfx_firmware.c',src/'wf_sfx_memory.c',src/'wf_sfx_start.S',src/'wf_sfx_decode.S',out/'sfx-patch.c']:
        obj=source.stem+'.o';objects.append(obj)
        run('gcc',*(FLAGS+['-DWF_SFX_FAST_68000=1'] if source.suffix=='.c' else ['-m68000']),'-I',rel(src),'-c',rel(source),'-o',obj)
    run('as','-m68000','sfx-wave.s','-o','sfx-wave.o')
    objects+=['sfx-wave.o','wf_scsp_fm_probe.m68000.o']
    run('gcc','-m68000','-msoft-float','-nostdlib','-Wl,-T,'+rel(src/'wf_sfx.ld'),
        '-Wl,-Map,sfx.map',*objects,'-lgcc','-o','SCSP.ELF')
    if run('nm','-u','SCSP.ELF').strip():raise ValueError('Undefined native symbols')
    syms={}
    for line in run('nm','-n','SCSP.ELF').splitlines():
        p=line.split()
        if len(p)==3:syms[p[2]]=int(p[0],16)
    (out/'symbols.json').write_text(json.dumps(syms,indent=2)+'\n')
    dis=run('objdump','-d','-m','m68k:68000','SCSP.ELF')
    if re.search(r'\t(?:reset|tas|fmov|fadd|fsub|fmul|fdiv)\b|\(bad\)',dis):raise ValueError('Invalid instruction')
    (out/'disassembly.txt').write_text(dis)
    (out/'elf.txt').write_text(run('readelf','-h','-l','-S','SCSP.ELF'))
    segments=load_segments(out/'SCSP.ELF')
    if segments[-1][1]!=(instrument/'sine-three-cycles.pcm').read_bytes():raise ValueError('Wave mismatch')
    report=dict(elf_bytes=(out/'SCSP.ELF').stat().st_size,
                elf_sha256=hashlib.sha256((out/'SCSP.ELF').read_bytes()).hexdigest(),
                segments=[dict(address=a,file_bytes=len(b),memory_bytes=n) for a,b,n in segments],
                code_limit=0x3000,pcm_rings=[0x3000,0x5000,0x7000,0x9000],
                bss_end=syms['__bss_end'],wave=[0x10000,0x11800],stack=[0x12000,0x14000],
                resident_pack=[0x14000,0x80000],abi=3,undefined_symbols=[],
                bin_cue_iso_generated=False,hardware_validation=False)
    (out/'sfx-build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    for n in ['toolchain','instrument','out']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();print(json.dumps(build_sfx(a.repo,a.toolchain,a.instrument,a.out),indent=2))
