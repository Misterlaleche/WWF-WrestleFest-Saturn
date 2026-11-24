#!/usr/bin/env python3
"""Real SH-2 adapter paths with a fake BIOS/MMIO edge; BIOS input is optional."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def verify_original_bios(path):
    """Decode the relevant SH-2 instructions from a supplied MPR-17933.

    No BIOS bytes are bundled in the test. Instruction masks below are the
    SH-2 ISA; the addresses identify the original BUP_Init call path.
    """
    data = path.read_bytes()
    assert len(data) == 524288, 'expected a complete MPR-17933 BIOS'
    word = lambda at: int.from_bytes(data[at:at+2], 'big')
    pc = 0x7d610
    load = word(pc)
    assert load & 0xf000 == 0x9000, 'expected PC-relative MOV.W'
    register = load >> 8 & 15
    literal = pc + 4 + (load & 255) * 2
    address = word(literal)
    dereference = word(0x7d616)
    assert dereference & 0xf00f == 0x6002, 'expected data MOV.L @Rm,Rn'
    assert dereference >> 4 & 15 == register
    assert dereference >> 8 & 15 == register
    assert address == 0x5f8
    assert int.from_bytes(data[address:address+4], 'big') == 0x1f04
    return {'sha256': hashlib.sha256(data).hexdigest(),
            'read_pc': '0x7d616', 'data_address': '0x5f8', 'target': '0x1f04'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', type=Path, default=ROOT/'port',
                        help='Adapter C/header directory; accepts an isolated candidate or old source')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--bios', type=Path, help='Optional private MPR-17933; verifies the original low data read')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    port = args.source_dir.resolve()
    report = {'bios': verify_original_bios(args.bios) if args.bios else None,
              'bios_required_for_host_tests': False, 'physical_hardware_verified': False}
    with tempfile.TemporaryDirectory(prefix='wf-save-bios-') as temporary:
        build = args.build_dir.resolve() if args.build_dir else Path(temporary)
        build.mkdir(parents=True, exist_ok=True)
        source = (port/'wf_jo_save.c').read_text()
        report['adapter_sha256'] = hashlib.sha256(source.encode()).hexdigest()
        old = '#define BIOS_ENTRY(offset) (*(volatile uintptr_t *)(*(volatile uintptr_t *)0x06000354u+(offset)))'
        assert source.count(old) == 1
        source = source.replace(old, '#define BIOS_ENTRY(offset) test_bios_entry(offset)')
        old = '(*(volatile uintptr_t *)0x06000358u)'
        assert source.count(old) == 1
        source = source.replace(old, 'test_bios_init_entry')
        source = source.replace('(volatile uint16_t *)0xffffff48u', '&test_bbra')
        (build/'adapter-under-test.c').write_text(source)
        command = [os.environ.get('CC', 'cc'), '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                   '-D__sh__', '-DJO_DEBUG', '-I'+str(build), '-I'+str(ROOT/'tests/fixtures/save_bios'),
                   '-I'+str(port), '-I'+str(ROOT/'core/include')]
        if args.sanitize:
            command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        command += [str(ROOT/'tests/test_save_ubc.c'), str(ROOT/'core/src/wf_r18_save.c'),
                    '-o', str(build/'test')]
        subprocess.run(command, check=True)
        run = subprocess.run([str(build/'test')], capture_output=True, text=True,
                             env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        (build/'run.log').write_text(run.stdout+run.stderr)
        print(run.stdout+run.stderr, end='')
        report['returncode'] = run.returncode
        (build/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        return run.returncode


if __name__ == '__main__':
    raise SystemExit(main())
