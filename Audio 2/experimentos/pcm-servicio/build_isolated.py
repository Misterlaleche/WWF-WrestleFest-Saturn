"""Exact-flag local M68K build, only below late-fix; no media or canonical writes."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess

HERE = Path(__file__).resolve().parent
REPAIR = HERE.parents[2]
REPO = REPAIR.parents[2]
EXPERIMENT = HERE / "late-fix"
COMPILER = Path("C:/Users/VersusPC/Documents/Codex/2026-10-25/https-github-com-misterlaleche-wwf-wrestlefest/work/m68k-toolchain/portable/m68k-elf/bin/m68k-elf-gcc.exe")
FLAGS = ['-m68000','-msoft-float','-ffreestanding','-fno-builtin','-fno-pic',
         '-fno-unwind-tables','-fno-asynchronous-unwind-tables','-Os','-std=c11',
         '-Wall','-Wextra','-Werror','-Wpedantic','-fstack-usage','-DWF_SFX_FAST_68000=1']
ASSETS = ('oki-lut.inc','wf_dsp_program.h','music-start.S','music-sine.pcm','music-wave.s')


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def loads(p):
    data = p.read_bytes()
    offset = struct.unpack_from('>I',data,28)[0]
    count = struct.unpack_from('>H',data,44)[0]
    result=[]
    for i in range(count):
        kind,off,va,pa,size,mem,flags,align=struct.unpack_from('>8I',data,offset+32*i)
        if kind==1:result.append((va,pa,size,mem,flags,data[off:off+size]))
    return result


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--seed',action='store_true')
    parser.add_argument('--name',required=True)
    parser.add_argument('--expect-baseline',action='store_true')
    args=parser.parse_args()
    if not args.name.replace('-','').isalnum():raise ValueError('simple build name required')
    source=EXPERIMENT/'repo/src/audio/native'
    if args.seed:
        shutil.copytree(REPO/'src/audio/native',source)
        shutil.copytree(REPO/'tests/audio/native-driver',EXPERIMENT/'repo/tests/audio/native-driver')
    out=EXPERIMENT/args.name
    out.mkdir(parents=True,exist_ok=True)
    for name in ASSETS:shutil.copyfile(REPAIR/'firmware'/name,out/name)
    env=dict(os.environ)
    env['PATH']=str(COMPILER.parent)+os.pathsep+env.get('PATH','')
    commands=[]

    def run(tool,*arguments):
        command=[str(COMPILER.with_name('m68k-elf-'+tool+'.exe')),*map(str,arguments)]
        process=subprocess.run(command,cwd=out,env=env,text=True,capture_output=True)
        commands.append(dict(command=command,returncode=process.returncode,stdout=process.stdout,stderr=process.stderr))
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if process.returncode:raise RuntimeError(process.stdout+process.stderr)
        return process.stdout

    objects=[]
    for name in ('wf_scsp_sfx.c','wf_scsp_dsp.c','wf_scsp_ym.c','wf_music_firmware.c','wf_sfx_memory.c','wf_sfx_decode.S'):
        obj=Path(name).stem+'.o';objects.append(obj)
        run('gcc',*(FLAGS if name.endswith('.c') else ['-m68000']),'-I',source,'-I',out,'-c',source/name,'-o',obj)
    run('gcc','-m68000','-c','music-start.S','-o','music-start.o')
    run('as','-m68000','music-wave.s','-o','music-wave.o')
    run('gcc','-m68000','-msoft-float','-nostdlib','-Wl,-T,'+str(source/'wf_music_m68k.ld'),
        '-Wl,-Map,music.map',*objects,'music-start.o','music-wave.o','-lgcc','-o','SCSP.ELF')
    assert not run('nm','-u','SCSP.ELF').strip()
    symbols={}
    for line in run('nm','-n','SCSP.ELF').splitlines():
        pieces=line.split()
        if len(pieces)==3:symbols[pieces[2]]=int(pieces[0],16)
    (out/'symbols.json').write_text(json.dumps(symbols,indent=2)+'\n')
    (out/'disassembly.txt').write_text(run('objdump','-d','-m','m68k:68000','SCSP.ELF'))
    exact=loads(out/'SCSP.ELF')==loads(REPAIR/'firmware/SCSP.ELF')
    if args.expect_baseline:assert exact,'Unmodified build loads differ from pinned original'
    report=dict(elf_sha256=sha(out/'SCSP.ELF'),baseline_loads_exact=exact,
                compiler_sha256=sha(COMPILER),flags=FLAGS,bss_end=symbols['__bss_end'],
                code_and_data_below_ring_limit=True,no_media=True,no_canonical_writes=True,
                source_hashes={str(p.relative_to(source)):sha(p) for p in source.rglob('*') if p.is_file()})
    (out/'BUILD.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='source_hashes'},indent=2))


if __name__=='__main__':main()
