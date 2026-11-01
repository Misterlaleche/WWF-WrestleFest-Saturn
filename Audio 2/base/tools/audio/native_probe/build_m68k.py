"""Compile the existing SCSP probe and observed assets to 68000 ELF objects only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

FLAGS = [
    '-m68000', '-msoft-float', '-ffreestanding', '-fno-builtin', '-fno-pic',
    '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-Os', '-std=c11',
    '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-fstack-usage',
]
R04_SHA = 'f1500df193b7aec9470b965fdd72c2aa3784b54f8457ea3b5fe45bf1443892a6'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def elf_sections(path):
    data = path.read_bytes()
    if data[:7] != b'\x7fELF\x01\x02\x01':
        raise ValueError(f'Expected ELF32 big-endian: {path.name}')
    header = struct.unpack_from('>HHIIIIIHHHHHH', data, 16)
    if header[0] != 1 or header[1] != 4 or header[6] not in (0, 0x01000000):
        raise ValueError(f'Expected relocatable baseline m68k ELF: {path.name}')
    if header[3] != 0 or header[9] != 0:
        raise ValueError('Unexpected program entry/program headers')
    table, stride, count, strings_index = header[5], header[10], header[11], header[12]
    if stride != 40 or not 0 <= strings_index < count:
        raise ValueError('Invalid ELF section table')
    sections = [struct.unpack_from('>10I', data, table + i * stride) for i in range(count)]
    strings = sections[strings_index]
    names = data[strings[4]:strings[4] + strings[5]]
    result = {}
    for section in sections:
        name = names[section[0]:].split(b'\0', 1)[0].decode('ascii')
        content = (b'' if section[1] == 8 else
                   data[section[4]:section[4] + section[5]])
        result[name] = {'size': section[5], 'flags': section[2], 'data': content}
    # GNU as leaves CPU flags unset for data-only objects, including -m68000.
    # Every object containing instructions must explicitly carry EF_M68000.
    if any(s['flags'] & 4 and s['size'] for s in result.values()) and header[6] != 0x01000000:
        raise ValueError(f'Executable sections require the m68000 flag: {path.name}')
    return result


def build(repo, toolchain, instrument, output):
    repo, toolchain, instrument, output = [
        p.resolve() for p in (repo, toolchain, instrument, output)]
    r04 = repo / 'src/audio/livevm/wf_audio_live_vm.c'
    r04_bytes = r04.read_bytes()
    if sha(r04_bytes.replace(b'\r\n', b'\n')) != R04_SHA:
        raise ValueError('R04 differs from its preserved reference')
    output.mkdir(parents=True, exist_ok=True)
    suffix = '.exe' if os.name == 'nt' else ''
    commands = []
    env = dict(os.environ)
    env['PATH'] = str(toolchain / 'bin') + os.pathsep + env.get('PATH', '')

    def run(tool, *args):
        executable = toolchain / 'bin' / ('m68k-elf-' + tool + suffix)
        command = [str(executable), *map(str, args)]
        result = subprocess.run(command, cwd=output, env=env,
                                capture_output=True, text=True)
        commands.append({'tool': executable.name, 'arguments': list(map(str, args)),
                         'exit_code': result.returncode,
                         'stdout': result.stdout, 'stderr': result.stderr})
        if result.returncode:
            (output / 'build-commands.json').write_text(json.dumps(commands, indent=2) + '\n')
            raise RuntimeError(f'{tool} failed: {result.stderr}')
        return result.stdout

    def relative(path):
        return Path(os.path.relpath(path, output)).as_posix()

    version = run('gcc', '--version').splitlines()[0]
    target = run('gcc', '-dumpmachine').strip()
    if target != 'm68k-elf':
        raise ValueError(f'Expected m68k-elf toolchain, got {target}')
    include = relative(repo / 'src/audio/native')
    source = relative(repo / 'src/audio/native/wf_scsp_fm_probe.c')
    core = 'wf_scsp_fm_probe.m68000.o'
    patch = 'wf_scsp_fm_patch.m68000.o'
    wave = 'wf_scsp_fm_wave.m68000.o'
    bundle = 'wf_scsp_fm_bundle.m68000.o'
    run('gcc', *FLAGS, '-I', include, '-c', source, '-o', core)

    header = (instrument / 'first-instrument.h').read_text(encoding='ascii')
    declaration = 'static const wf_scsp_fm_probe_patch_t wf_first_fm_patch'
    if header.count(declaration) != 1:
        raise ValueError('Observed instrument declaration changed')
    # Export the exact observed constant; never rewrite the supplied instrument.
    generated = header.replace(declaration,
        'const wf_scsp_fm_probe_patch_t wf_scsp_fm_first_patch', 1)
    generated += (
        '\n#if !defined(__mc68000__) || defined(__mc68020__) || defined(__HAVE_68881__)\n'
        '#error Expected baseline 68000 without FPU\n#endif\n'
        '_Static_assert(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__, "expected big endian");\n'
        '_Static_assert(sizeof(int)==4 && sizeof(void*)==4, "expected 32-bit ABI");\n'
        '_Static_assert(sizeof(wf_scsp_fm_probe_patch_t)==100, "patch ABI changed");\n'
        '_Static_assert(offsetof(wf_scsp_fm_probe_patch_t,words)==4, "patch layout changed");\n')
    (output / 'observed-patch.c').write_text(generated, encoding='ascii', newline='\n')
    run('gcc', *FLAGS, '-I', include, '-c', 'observed-patch.c', '-o', patch)
    wave_bytes = (instrument / 'sine-three-cycles.pcm').read_bytes()
    if len(wave_bytes) != 6144:
        raise ValueError('Expected 6144-byte three-cycle waveform')
    (output / 'sine-three-cycles.pcm').write_bytes(wave_bytes)
    assembly = (
        '.section .rodata.wf_scsp_wave,"a",@progbits\n.balign 2\n'
        '.global wf_scsp_fm_wave\n.global wf_scsp_fm_wave_end\n'
        '.type wf_scsp_fm_wave,@object\nwf_scsp_fm_wave:\n'
        '.incbin "sine-three-cycles.pcm"\nwf_scsp_fm_wave_end:\n'
        '.size wf_scsp_fm_wave,wf_scsp_fm_wave_end-wf_scsp_fm_wave\n')
    (output / 'observed-wave.s').write_text(assembly, encoding='ascii', newline='\n')
    run('as', '-m68000', 'observed-wave.s', '-o', wave)
    run('ld', '-r', '-o', bundle, core, patch, wave)

    symbols = run('nm', '-u', bundle)
    if symbols.strip():
        raise ValueError(f'Unexpected undefined symbols: {symbols}')
    exported = run('nm', '-g', '--defined-only', bundle)
    if not all(name in exported for name in (
            'wf_scsp_fm_probe_start', 'wf_scsp_fm_probe_stop',
            'wf_scsp_fm_probe_upload', 'wf_scsp_fm_first_patch',
            'wf_scsp_fm_wave', 'wf_scsp_fm_wave_end')):
        raise ValueError('Missing public module symbols')
    parts = {}
    for name in (core, patch, wave, bundle):
        sections = elf_sections(output / name)
        parts[name] = {'sha256': sha((output / name).read_bytes()),
                       'file_bytes': (output / name).stat().st_size,
                       'sections': {k: v['size'] for k, v in sections.items()
                                    if v['flags'] & 2}}
    bundled = elf_sections(output / bundle)
    if bundled['.rodata.wf_scsp_wave']['data'] != wave_bytes:
        raise ValueError('Embedded waveform differs from original PCM16BE')
    definition = json.loads((instrument / 'first-instrument.json').read_text())
    flat_words = [word for row in definition['slot_words'] for word in row]
    expected_patch = struct.pack('>I48H', definition['wave_start'], *flat_words)
    if bundled['.rodata']['data'] != expected_patch:
        raise ValueError('Compiled patch differs from observed register plan')
    disassembly = run('objdump', '-dr', '-m', 'm68k:68000', bundle)
    if '(bad)' in disassembly:
        raise ValueError('Invalid instructions in 68000 disassembly')
    (output / 'disassembly-m68000.txt').write_text(disassembly, encoding='utf-8')
    (output / 'elf-header.txt').write_text(run('readelf', '-h', bundle))
    (output / 'sections.txt').write_text(run('size', '-A', bundle))
    (output / 'symbols.txt').write_text(exported)
    (output / 'build-commands.json').write_text(json.dumps(commands, indent=2) + '\n')
    if r04.read_bytes() != r04_bytes:
        raise ValueError('R04 changed during the build')
    report = {
        'compiler': version, 'target': target, 'cpu': '68000', 'fpu': False,
        'flags': FLAGS, 'elf_class': 32, 'endian': 'big', 'elf_type': 'REL',
        'elf_machine': 'MC68000', 'elf_flags': '0x01000000 (m68000)',
        'parts': parts, 'undefined_symbols': [], 'wave_bytes_verified': 6144,
        'patch_bytes_verified': 100, 'r04_unchanged': True,
        'r04_source_lf_sha256': R04_SHA,
        'source_sha256': sha((repo / 'src/audio/native/wf_scsp_fm_probe.c').read_bytes()),
        'scope': 'cross-compilation and relocatable link only; not a bootable sound driver',
        'hardware_validation': 'NOT PERFORMED', 'bin_cue_iso_generated': False,
    }
    (output / 'cross-build-report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--toolchain', type=Path, required=True)
    parser.add_argument('--instrument', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.repo, args.toolchain, args.instrument, args.out), indent=2))
