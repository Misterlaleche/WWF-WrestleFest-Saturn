"""Build the experimental 68000 driver as ELF and SH2 C segments; no flat image."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
from build_m68k import build, FLAGS


def load_segments(path):
    data = path.read_bytes()
    if data[:7] != b'\x7fELF\x01\x02\x01':
        raise ValueError('ELF32 BE required')
    h = struct.unpack_from('>HHIIIIIHHHHHH', data, 16)
    if h[0] != 2 or h[1] != 4 or h[6] != 0x1000000 or h[8] != 32:
        raise ValueError('baseline 68000 executable required')
    result = []
    for i in range(h[9]):
        p = struct.unpack_from('>8I', data, h[4]+i*h[8])
        if p[0] != 1:
            continue
        _, offset, address, physical, filesz, memsz, flags, align = p
        if (address != physical or filesz > memsz or offset+filesz > len(data) or
                (address | filesz | memsz) & 1):
            raise ValueError('Invalid load segment')
        if not any(lo <= address and address+memsz <= hi for lo, hi in
                   [(0, 0xbc00), (0xc100, 0x12400), (0x70000, 0x71800)]):
            raise ValueError('Load segment violates reserved RAM map')
        if any(address < b+lenmem and b < address+memsz for b, _, lenmem in result):
            raise ValueError('Overlapping segments')
        result.append((address, data[offset:offset+filesz], memsz))
    if len(result) != 3 or result[0][0] != 0 or len(result[0][1]) < 1024:
        raise ValueError('Expected three segments with reset vectors')
    stack, pc = struct.unpack_from('>II', result[0][1])
    if stack != 0x14000 or pc != h[3] or pc & 1 or not 0x400 <= pc < len(result[0][1]):
        raise ValueError('Invalid reset vectors')
    if result[-1][0] not in (0x10000,0x70000) or len(result[-1][1]) not in (6144,9216):
        raise ValueError('Invalid wave segment')
    if result[-1][0]==0x10000 and (result[0][2]>0x3000 or result[1][0]!=0xc100 or result[1][0]+result[1][2]>0x10000):
        raise ValueError('Resident SFX segment overlaps PCM or FM RAM')
    return result


def build_driver(repo, toolchain, instrument, output):
    repo, toolchain, instrument, output = [p.resolve() for p in
                                         (repo, toolchain, instrument, output)]
    build(repo, toolchain, instrument, output)
    env = dict(os.environ)
    env['PATH'] = str(toolchain/'bin')+os.pathsep+env.get('PATH', '')
    commands = []

    def run(name, *args):
        tool = toolchain/'bin'/('m68k-elf-'+name+('.exe' if os.name=='nt' else ''))
        command = [str(tool), *map(str, args)]
        r = subprocess.run(command, cwd=output, env=env, text=True, capture_output=True)
        commands.append(dict(command=command, exit_code=r.returncode,
                             stdout=r.stdout, stderr=r.stderr))
        (output/'driver-commands.json').write_text(json.dumps(commands, indent=2)+'\n')
        if r.returncode:
            raise RuntimeError(r.stderr)
        return r.stdout

    def rel(p):
        return Path(os.path.relpath(p, output)).as_posix()

    source = repo/'src/audio/native'
    objects = []
    for name in ['wf_native_driver.c', 'ponesound/wf_ponesound.c', 'wf_m68k_start.S']:
        obj = Path(name).stem+'.o'
        flags = FLAGS if name.endswith('.c') else ['-m68000']
        # Keep upstream warnings visible; its legacy style is not -Werror clean.
        if name.startswith('ponesound/'):
            flags = [f for f in flags if f not in ['-Werror', '-Wpedantic']]
            flags += ['-fno-delete-null-pointer-checks']
        run('gcc', *flags, '-I', rel(source), '-c', rel(source/name), '-o', obj)
        objects.append(obj)
    wave_s = (output/'observed-wave.s').read_text()
    wave_s = wave_s.replace('.rodata.wf_scsp_wave', '.wf_native_wave')
    (output/'driver-wave.s').write_text(wave_s)
    run('as', '-m68000', '-o', 'driver-wave.o', 'driver-wave.s')
    objects += ['wf_scsp_fm_probe.m68000.o', 'wf_scsp_fm_patch.m68000.o', 'driver-wave.o']
    run('gcc', '-m68000', '-msoft-float', '-nostdlib',
        '-Wl,-T,'+rel(source/'wf_m68k.ld'), '-Wl,-Map,wf_scsp_driver.map',
        *objects, '-o', 'wf_scsp_driver.elf')
    if run('nm', '-u', 'wf_scsp_driver.elf').strip():
        raise ValueError('Unresolved firmware symbols')
    syms = {}
    for line in run('nm', '-n', 'wf_scsp_driver.elf').splitlines():
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16)
    (output/'symbols.json').write_text(json.dumps(syms, indent=2)+'\n')
    dis = run('objdump', '-d', '-m', 'm68k:68000', 'wf_scsp_driver.elf')
    if re.search(r'\t(?:reset|tas|fmov|fadd|fsub|fmul|fdiv)\b|\(bad\)', dis):
        raise ValueError('Prohibited or invalid instruction')
    (output/'driver-disassembly.txt').write_text(dis)
    (output/'driver-elf.txt').write_text(run('readelf', '-h', '-l', '-S', 'wf_scsp_driver.elf'))
    segs = load_segments(output/'wf_scsp_driver.elf')
    if segs[-1][1] != (output/'sine-three-cycles.pcm').read_bytes():
        raise ValueError('Wave segment mismatch')
    c = ['/* Generated from the verified ELF PT_LOAD segments. */',
         '#include "wf_native_host.h"']
    for n, (address, content, memory) in enumerate(segs):
        if content:
            c.append('static const uint8_t segment_%d[] = {' % n)
            for start in range(0, len(content), 16):
                c.append('    '+','.join('0x%02x'%v for v in content[start:start+16])+',')
            c.append('};')
    c.append('static const wf_native_segment_t segments[] = {')
    for n, (address, content, memory) in enumerate(segs):
        c.append('    {0x%x, %s, %d, %d},' %
                 (address, 'segment_%d'%n if content else 'NULL', len(content), memory))
    c += ['};', 'const wf_native_image_t wf_native_driver_image = {segments, 3};']
    (output/'wf_native_image.c').write_text('\n'.join(c)+'\n')
    report = {
        'elf_sha256': hashlib.sha256((output/'wf_scsp_driver.elf').read_bytes()).hexdigest(),
        'elf_bytes': (output/'wf_scsp_driver.elf').stat().st_size,
        'cpu': '68000', 'elf_type': 'EXEC', 'entry': syms['wf_m68k_reset'],
        'stack': [0x12000, 0x14000], 'legacy_com': 0xc008, 'mail': 0xc040,
        'segments': [dict(address=a, file_bytes=len(d), memory_bytes=m) for a,d,m in segs],
        'undefined_symbols': [], 'bin_cue_iso_generated': False,
        'hardware_validation': 'NOT PERFORMED',
        'ponesound_commit': 'c2b0d3a929d6f4f124fccad5f1e0359f8862c7e2'
    }
    (output/'driver-build-report.json').write_text(json.dumps(report, indent=2)+'\n')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    p.add_argument('--toolchain', type=Path, required=True)
    p.add_argument('--instrument', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    print(json.dumps(build_driver(a.repo, a.toolchain, a.instrument, a.out), indent=2))
