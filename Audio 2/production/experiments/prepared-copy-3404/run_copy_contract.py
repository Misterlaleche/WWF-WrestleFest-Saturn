#!/usr/bin/env python3
"""Reproduce the prepared-copy memory contract using local, private inputs.

This is a portable C memory/state check and an emitted-instruction check.
It is not a 68000 timing emulator, a native capture or a waveform comparison.
No ROM, decoded PCM, firmware image or generated binary is embedded here.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import resource
import shutil
import subprocess


FIRMWARE_SHA256 = '3404eac10581871b4afe5ee30d1c58e415430db263845228c5c82ddec7647749'
SOURCE_SHA256 = {
    '4745461ca0439802e0f4ffc4395e07ef89d7b09ff41e83868b3bc7dfefa7d05b',
    '74578e6610677070e46ca4208f1b16abfef29b99060f8a4543118abe0d4a50e5',
}
REFERENCE_COPY = '''        for(i=0;i<256;i++){
            if(!put(s,base+i*2u,(uint16_t)prepared->samples[i]))return false;
            if(!i&&s->pcm_stride==8194&&!put(s,base+8192u,(uint16_t)prepared->samples[0]))return false;
        }'''
ORDERED_COPY_MODEL = '''        if(!put(s,base,(uint16_t)prepared->samples[0]))return false;
        if(s->pcm_stride==8194&&!put(s,base+8192u,(uint16_t)prepared->samples[0]))return false;
        i=1;
        for(unsigned block=0;block<31;block++)for(unsigned word=0;word<8;word++,i++)
            if(!put(s,base+i*2u,(uint16_t)prepared->samples[i]))return false;
        for(unsigned word=0;word<7;word++,i++)
            if(!put(s,base+i*2u,(uint16_t)prepared->samples[i]))return false;'''


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('package', 'sfx-data', 'firmware', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--cc', default='cc', help='Host C compiler executable')
    parser.add_argument('--objdump', default='m68k-linux-gnu-objdump',
                        help='Objdump executable supporting the original 68000 ELF')
    args = parser.parse_args()
    for key, value in vars(args).items():
        if isinstance(value, Path):
            setattr(args, key, value.resolve())
    here = Path(__file__).resolve().parent
    for protected in (args.package, here.parents[1]):
        require(args.out != protected and protected not in args.out.parents,
                'Generated binaries and private inputs must stay outside the source package')
    require(not args.out.exists(), '--out must be a new private directory')
    source_path = args.package / 'src/wf_scsp_sfx.c'
    fixture = here / 'test_copy_contract.c'
    inputs = {'source': source_path, 'header': args.package / 'src/wf_scsp_sfx.h',
              'sfx_data': args.sfx_data, 'firmware': args.firmware, 'fixture': fixture,
              'generator': Path(__file__).resolve()}
    before = {name: sha(path) for name, path in inputs.items()}
    require(before['source'] in SOURCE_SHA256, 'Source is not the pinned baseline or candidate')
    require(before['firmware'] == FIRMWARE_SHA256, 'Firmware is not the pinned 3404 candidate')
    source = source_path.read_text()
    require(source.count(REFERENCE_COPY) == 1, 'Expected one unchanged portable copy branch')
    compilers = {}
    for key in ('cc', 'objdump'):
        executable = shutil.which(getattr(args, key))
        require(executable is not None, key + ' executable not found')
        setattr(args, key, str(Path(executable).resolve()))
        compilers[key] = sha(getattr(args, key))
    # The deliberate assertion failure must never dump decoded local samples.
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    args.out.mkdir(parents=True)
    variants = {'base': source, 'model': source.replace(REFERENCE_COPY, ORDERED_COPY_MODEL),
                'bad-model': source.replace(REFERENCE_COPY,
                                           ORDERED_COPY_MODEL.replace('word<7', 'word<6'))}
    for name, text in variants.items():
        (args.out / (name + '.c')).write_text(text)
    (args.out / 'model.patch').write_text(''.join(difflib.unified_diff(
        source.splitlines(True), variants['model'].splitlines(True),
        fromfile='base.c', tofile='model.c')))
    commands = []

    def run(command, allow_failure=False):
        command = list(map(str, command))
        result = subprocess.run(command, text=True, capture_output=True, cwd=args.out)
        commands.append({'command': command, 'returncode': result.returncode,
                         'stdout': result.stdout, 'stderr': result.stderr})
        (args.out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        require(allow_failure or result.returncode == 0,
                'Command failed; inspect private commands.json')
        return result

    names = sorted(set(re.findall(r'\b(wf_scsp_sfx_\w+)\s*\(', source)))
    common = [args.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
              '-I', args.package / 'src']
    for name, prefix in (('base', 'base_'), ('model', 'model_'), ('bad-model', 'model_')):
        run([*common, *['-D' + name + '=' + prefix + name for name in names],
             '-c', args.out / (name + '.c'), '-o', args.out / (name + '.o')])
    for name in ('bad-model', 'model'):
        executable = args.out / ('test-' + name)
        run([*common, fixture, args.out / 'base.o', args.out / (name + '.o'),
             '-o', executable])
        execution = run([executable, args.sfx_data], allow_failure=name == 'bad-model')
        if name == 'bad-model':
            require(execution.returncode == -6 and 'a_mem' in execution.stderr
                    and 'b_mem' in execution.stderr,
                    'Negative control must fail at destination/write equality with SIGABRT')
            negative = {'returncode': execution.returncode,
                        'failed_at_destination_write_equality': True}
        else:
            result = json.loads(execution.stdout)
    require(result.get('cases') == 14592 and result.get('whole_ram_words_compared') == 328786944,
            'The complete 114-prefix/4-voice/2-stride/16-volume matrix did not run')
    require(all(result.get(name) is True for name in (
        'ordered_writes_equal', 'engine_and_decoder_state_equal',
        'prepared_source_unchanged', 'guards_and_non_target_words_equal')),
        'Memory or state contract failed')
    # Disassemble the supplied ELF here: a detached text file is not its authority.
    disassembly = run([args.objdump, '-d', '-m', 'm68k:68000', args.firmware]).stdout
    (args.out / 'firmware.disassembly.txt').write_text(disassembly)
    require(disassembly.count('<start>:') == 1, 'Expected one linked start symbol')
    section = disassembly.split('<start>:', 1)[1].split('<wf_scsp_sfx_stop_all>:', 1)[0]
    instructions = []
    for line in section.splitlines():
        match = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{4}\s+)+)\s*(\S.*)', line)
        if match:
            instructions.append((int(match[1], 16), match[3].strip()))
    hits = []
    for index, (_, instruction) in enumerate(instructions):
        if instruction == 'moveq #30,%d0':
            sequence = instructions[index + 1:index + 17]
            require(len(sequence) == 16 and all(
                row[1] == 'movew %a3@+,%a0@+' for row in sequence[:8] + sequence[9:]),
                'Candidate is not eight ordered word moves followed by seven tail moves')
            require(sequence[8][1].startswith('dbf %d0,' + format(sequence[0][0], 'x') + ' '),
                    'DBF target is not the first move in the eight-word group')
            hits.append(sequence[0][0])
    require(len(hits) == 1, 'Expected exactly one 31-group copy loop')
    after = {name: sha(path) for name, path in inputs.items()}
    require(before == after, 'An input changed during verification')
    result.update({
        'tool_version': '1.0.0',
        'scope': 'Portable memory/state contract plus static emitted-opcode check; not native cycle or waveform proof',
        'negative_off_by_one_rejected': True, 'negative_control': negative,
        'emitted_word_move_loop_groups': 31, 'moves_per_group': 8, 'tail_moves': 7,
        'remaining_word_moves': 255, 'loop_pc': hits[0],
        'firmware_sha256': before['firmware'], 'inputs_sha256': before,
        'host_tools_sha256': compilers, 'inputs_stable': True,
        'generated_pcm_or_rom_arrays': False,
    })
    (args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
