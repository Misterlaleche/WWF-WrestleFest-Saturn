#!/usr/bin/env python3
"""Portable build of the existing native 68000/SCSP engine; private ELF output."""
from pathlib import Path
import argparse, hashlib, json, math, os, shutil, struct, subprocess
from build_dsp_mixer import generate


def private_output(path, package):
    """Protect both selected sources and this tool's repository/history."""
    original = Path(path)
    out = original.resolve()
    packages = {Path(package).resolve(), Path(__file__).resolve().parents[1]}
    protected = set(packages)
    for source in packages:
        if source.name == 'production' and source.parent.name == 'Audio 2':
            protected.add(source.parent.parent)
        for parent in source.parents:
            if (parent / '.git').exists():
                protected.add(parent)
                break
    if original.is_symlink() or out.exists() or any(
            out == root or root in out.parents for root in protected):
        raise ValueError('Use a fresh private output outside the source package and repository')
    return out


def build(package, out, prefix, runtime_source=None, timing=True):
    package=Path(package).resolve();out=private_output(out,package);out.mkdir(parents=True,exist_ok=False)
    src=package/'src';logs=[]
    (out/'include').mkdir(exist_ok=True)
    (out/'include/string.h').write_text('#include <stddef.h>\nvoid *memcpy(void *,const void *,size_t);\nvoid *memset(void *,int,size_t);\nint memcmp(const void *,const void *,size_t);\n')
    def run(args):
        p=subprocess.run(list(map(str,args)),cwd=out,text=True,capture_output=True)
        logs.append(dict(command=list(map(str,args)),returncode=p.returncode,stdout=p.stdout,stderr=p.stderr))
        (out/'build-commands.json').write_text(json.dumps(logs,indent=2)+'\n')
        if p.returncode: raise RuntimeError(p.stderr or p.stdout)
        return p.stdout
    def tool(name):
        v=shutil.which(prefix+name)
        if not v: raise FileNotFoundError(prefix+name)
        return v
    generate(out)
    steps=(16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552)
    shifts=(-1,-1,-1,-1,2,4,6,8);lut=[]
    for i,step in enumerate(steps):
        for n in range(16):
            delta=(step>>3)+((step>>2) if n&1 else 0)+((step>>1) if n&2 else 0)+(step if n&4 else 0)
            lut.append('.word %d,%d'%(-delta if n&8 else delta,max(0,min(48,i+shifts[n&7]))*64))
    (out/'oki-lut.inc').write_text('\n'.join(lut)+'\n')
    startup=(src/'wf_sfx_start.S').read_text().replace('0xb000','0xb008').replace('wf_sfx_main','wf_music_main')
    (out/'startup.S').write_text(startup)
    cycle=[round(32767*math.sin(2*math.pi*i/128)) for i in range(128)]
    (out/'sine.pcm').write_bytes(struct.pack('>4608h',*(cycle*36)))
    (out/'wave.S').write_text('.section .wf_native_wave,"a"\n.balign 2\n.incbin "sine.pcm"\n')
    flags=['-m68000','-msoft-float','-Os','-std=c11','-ffreestanding','-fno-builtin',
           '-fno-pie','-fno-pic','-fno-common','-fomit-frame-pointer','-ffunction-sections','-fdata-sections',
           '-fstack-usage','-Wall','-Wextra','-Werror','-DWF_SFX_FAST_68000=1',
           '-DWF_MUSIC_TIMING='+str(int(timing)),'-I',out/'include','-I',src,'-I',out]
    objects=[]
    for name in ['wf_scsp_sfx.c','wf_scsp_dsp.c','wf_scsp_ym.c','wf_music_firmware.c','wf_sfx_memory.c','wf_sfx_decode.S']:
        obj=out/(Path(name).stem+'.o');objects.append(obj)
        run([tool('gcc'),*(flags if name.endswith('.c') else ['-m68000','-I',out]),'-c',src/name,'-o',obj])
    for name in ['startup','wave']:
        obj=out/(name+'.o');objects.append(obj)
        run([tool('gcc'),'-m68000','-c',out/(name+'.S'),'-o',obj])
    runtime_identity=None
    runtime_libraries=['-lgcc']
    if runtime_source is not None:
        runtime_source=Path(runtime_source).resolve()
        runtime_identity=hashlib.sha256(runtime_source.read_bytes()).hexdigest()
        if runtime_identity!='851f082a547137b70dfe686764c9312aed489df0ff00d60e2dd6c6fe64d5ba00':
            raise ValueError('Pinned baseline-68000 GCC runtime source mismatch')
        for helper in ('mulsi3','udivsi3','divsi3'):
            obj=out/('libgcc-'+helper+'.o');objects.append(obj)
            run([tool('gcc'),'-m68000','-msoft-float','-fno-pic','-fno-pie',
                 '-DL_'+helper,'-c',runtime_source,'-o',obj])
        runtime_libraries=[]
    elf=out/'SCSP.ELF'
    run([tool('gcc'),'-m68000','-msoft-float','-nostdlib','-no-pie',
         '-Wl,--build-id=none','-Wl,--gc-sections','-Wl,-T,'+str(src/'wf_music_m68k.ld'),'-Wl,-Map,'+str(out/'SCSP.map'),
         *objects,*runtime_libraries,'-o',elf])
    undefined=run([tool('nm'),'-u',elf]).strip()
    if undefined:raise ValueError('Undefined symbols: '+undefined)
    (out/'SCSP.disassembly.txt').write_text(run([tool('objdump'),'-d','-m','m68k:68000',elf]))
    (out/'SCSP.headers.txt').write_text(run([tool('readelf'),'-h','-l','-S',elf]))
    syms={}
    for line in run([tool('nm'),'-n',elf]).splitlines():
        cols=line.split()
        if len(cols)==3:syms[cols[2]]=int(cols[0],16)
    (out/'symbols.json').write_text(json.dumps(syms,indent=2)+'\n')
    raw=elf.read_bytes();phoff=struct.unpack_from('>I',raw,28)[0];phnum=struct.unpack_from('>H',raw,44)[0]
    segments=[]
    for i in range(phnum):
        typ,off,virt,phys,filesz,memsz,attrs,align=struct.unpack_from('>8I',raw,phoff+i*32)
        if typ==1:segments.append(dict(address=phys,file_bytes=filesz,memory_bytes=memsz,flags=attrs))
    report=dict(compiler=run([tool('gcc'),'--version']).splitlines()[0],
                compiler_sha256=hashlib.sha256(Path(tool('gcc')).resolve().read_bytes()).hexdigest(),
                elf_sha256=hashlib.sha256(raw).hexdigest(),elf_bytes=len(raw),segments=segments,
                bss_end=syms['__bss_end'],stack_bottom=syms['__stack_bottom'],stack_top=syms['__stack_top'],
                runtime_source_sha256=runtime_identity,
                runtime_mode='explicit -m68000 source' if runtime_identity else 'compiler libgcc; verify linked helpers separately',
                diagnostic_timing_enabled=bool(timing),
                hardware_status='HARDWARE PENDING',software_acceptance='NOT EVALUATED')
    (out/'firmware-build.json').write_text(json.dumps(report,indent=2)+'\n')
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package',type=Path,default=Path(__file__).resolve().parents[1])
    p.add_argument('--out',type=Path,required=True,help='New private directory outside the source package/repository')
    p.add_argument('--prefix',default='m68k-linux-gnu-')
    p.add_argument('--runtime-source',type=Path,help='Pinned GCC13.3 lb1sf68.S; build base68000 helpers instead of distribution libgcc')
    p.add_argument('--no-timing',action='store_true',help='Remove private measurement overhead; keep all error/drop/underrun guards')
    a=p.parse_args();print(json.dumps(build(a.package,a.out,a.prefix,a.runtime_source,not a.no_timing),indent=2))
