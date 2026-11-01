"""Compile the SH2 half of the experimental sound integration, never a disc image."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def build_host(repo, toolchain, yaul_include, firmware, output):
    repo, toolchain, yaul_include, firmware, output = [
        p.resolve() for p in (repo, toolchain, yaul_include, firmware, output)]
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    env['PATH'] = str(toolchain/'bin')+os.pathsep+env.get('PATH', '')
    suffix = '.exe' if os.name == 'nt' else ''
    commands = []

    def run(path, *args):
        command = [str(path), *map(str, args)]
        r = subprocess.run(command, cwd=output, env=env, text=True, capture_output=True)
        commands.append(dict(command=command, exit_code=r.returncode,
                             stdout=r.stdout, stderr=r.stderr))
        (output/'sh2-commands.json').write_text(json.dumps(commands, indent=2)+'\n')
        if r.returncode:
            raise RuntimeError(r.stderr)
        return r.stdout

    cc=toolchain/'bin'/('sh-elf-gcc'+suffix)
    binutils=toolchain/'sh-elf/bin'
    version=run(cc,'--version').splitlines()[0]
    flags=['-m2','-mb','-ffreestanding','-fno-builtin','-Os','-std=c11',
           '-Wall','-Wextra','-Werror','-Wpedantic','-fstack-usage',
           '-B',str(binutils)+os.sep,
           '-I',str(repo/'src/audio/native'),
           '-isystem',str(yaul_include),'-isystem',str(yaul_include/'yaul')]
    sources=[repo/'src/audio/native'/name for name in
             ['wf_native_host.c','wf_native_wp24.c','wf_native_yaul.c']]
    sources += [repo/'src/audio/wp24'/name for name in
                ['wf24_music_stream.c','wf24_audio_contract.c']]
    sources += [firmware/'wf_native_image.c']
    objects=[]
    for src in sources:
        obj=src.stem+'.sh2.o'
        run(cc,*flags,'-c',src,'-o',obj)
        objects.append(obj)
        data=(output/obj).read_bytes()
        h=struct.unpack_from('>HHIIIIIHHHHHH',data,16)
        if data[:7]!=b'\x7fELF\x01\x02\x01' or h[0]!=1 or h[1]!=42 or h[6]!=2:
            raise ValueError('Expected SH2 big-endian ELF REL')
    bundle='wf_native_integration.sh2.o'
    run(binutils/('ld'+suffix),'-r','-o',bundle,*objects)
    unresolved=[line.split()[-1] for line in
                run(binutils/('nm'+suffix),'-u',bundle).splitlines() if line.strip()]
    allowed={'___movmemSI20','___movmemSI32','___movmemSI36','_memset'}
    if not set(unresolved)<=allowed:
        raise ValueError('Unresolved integration symbol: '+repr(unresolved))
    (output/'sh2-elf.txt').write_text(
        run(binutils/('readelf'+suffix),'-h','-S',bundle))
    # Hash the SDK headers available to this compile for reproducible provenance.
    sdk_hashes={}
    for path in sorted(yaul_include.rglob('*.h')):
        sdk_hashes[path.relative_to(yaul_include).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
    (output/'yaul-header-sha256.json').write_text(json.dumps(sdk_hashes,indent=2)+'\n')
    report=dict(compiler=version,cpu='SH2',endian='big',elf_type='REL',
                strict_warnings=True,
                parts={obj:hashlib.sha256((output/obj).read_bytes()).hexdigest()
                       for obj in objects+[bundle]},
                unresolved_runtime_symbols=unresolved,linked_game=False,
                hardware_execution=False,bin_cue_iso_generated=False)
    (output/'sh2-build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3])
    p.add_argument('--toolchain',type=Path,required=True)
    p.add_argument('--yaul-include',type=Path,required=True)
    p.add_argument('--firmware',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    a=p.parse_args()
    print(json.dumps(build_host(a.repo,a.toolchain,a.yaul_include,a.firmware,a.out),indent=2))
