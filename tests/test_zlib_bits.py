#!/usr/bin/env python3
"""Fresh bit helper gates and SH2 Adler instruction-block verification.

The Adler gate interprets the actual generated64-instruction DO16 blocks and
compares against an independent weighted-sum formula. It is not Saturn timing
or a replacement for a full target corpus execution.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def verify_adler(assembly):
    # The bundled SH2 backend marks inline assembly with source-line comments
    # rather than the generic GCC #APP/#NO_APP pair.
    blocks = re.findall(r'^! \d+ "[^"\n]*adler32\.c" 1\n(.*?)^! \d+ "[^"\n]*" 2',
                        assembly, re.S | re.M)
    checked = 0
    rng = random.Random(0xAD1E32)
    for block in blocks:
        lines = [line.strip() for line in block.splitlines() if line.strip()
                 and not line.strip().startswith(('#', '!', '.'))]
        if not any('mov.b @' in line for line in lines):
            continue
        if len(lines) != 64:
            raise AssertionError(('Unexpected Adler block length', len(lines)))
        instructions = []
        for line in lines:
            match = re.fullmatch(r'(mov\.b|extu\.b|add)\s+(@?r\d+\+?),\s*(r\d+)', line)
            if not match:
                raise AssertionError(('Unexpected SH2 operation/clobber', line))
            instructions.append(match.groups())
        pointer, scratch = instructions[0][1].strip('@+'), instructions[0][2]
        s1, s2 = instructions[2][2], instructions[3][2]
        if len({pointer, scratch, s1, s2}) != 4:
            raise AssertionError('Adler early-clobber register alias')
        for case in range(10000):
            data = bytes(rng.getrandbits(8) for _ in range(16))
            first, second = rng.getrandbits(32), rng.getrandbits(32)
            address = 0x1000 + case % 4
            registers = {f'r{i}': 0 for i in range(16)}
            registers[pointer], registers[s1], registers[s2] = address, first, second
            for op, source, target in instructions:
                if op == 'mov.b':
                    reg = source.strip('@+')
                    byte = data[registers[reg]-address]
                    registers[target] = byte if byte < 128 else byte-256
                    registers[reg] += 1
                elif op == 'extu.b':
                    registers[target] = registers[source] & 255
                else:
                    registers[target] = (registers[target]+registers[source]) & 0xFFFFFFFF
            expected1 = (first+sum(data)) & 0xFFFFFFFF
            expected2 = (second+16*first+sum((16-i)*value for i, value in enumerate(data))) & 0xFFFFFFFF
            if (registers[s1], registers[s2], registers[pointer]) != (expected1, expected2, address+16):
                raise AssertionError('Generated Adler instruction block differs from weighted sum')
            checked += 1
    if not checked:
        raise AssertionError('No SH2 Adler DO16 assembly was verified')
    return checked


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir', type=Path, default=ROOT/'build/recovery-assets/bits')
    p.add_argument('--sanitize', action='store_true')
    p.add_argument('--codegen', action='store_true')
    args = p.parse_args(); build = args.build_dir.resolve(); build.mkdir(parents=True, exist_ok=True)
    report = {'status': 'RUNNING', 'commands': [], 'source_sha256': {}}
    for path in [Path(__file__), ROOT/'tests/test_zlib_bits.c',
                 ROOT/'port/zlib/wf_sh2_bits.h', ROOT/'port/zlib/adler32.c', ROOT/'port/zlib/inftrees.h']:
        report['source_sha256'][str(path.resolve())] = hashlib.sha256(path.read_bytes()).hexdigest()
    def run(command, name):
        result = subprocess.run([str(v) for v in command], cwd=ROOT, text=True,
            capture_output=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        (build/(name+'.log')).write_text(result.stdout+result.stderr)
        report['commands'].append({'argv': [str(v) for v in command], 'cwd': str(ROOT),
            'returncode': result.returncode, 'log': str(build/(name+'.log'))})
        result.check_returncode();return result.stdout
    try:
        flags = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-I'+str(ROOT/'port/zlib')]
        if args.sanitize:flags += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        run([*flags, ROOT/'tests/test_zlib_bits.c', '-o', build/'bits'], 'compile-host')
        print(run([build/'bits'], 'bit-operations').strip())
        if args.codegen:
            compiler = ROOT/'sdk/Compiler/LINUX/bin/sh-none-elf-gcc-8.2.0'
            probe = build/'probe.c'
            probe.write_text('#include "zutil.h"\n#include "inftrees.h"\n#include "wf_sh2_bits.h"\n'
                'unsigned long drop(unsigned long x,unsigned n){return WF_ZLIB_DROP(x,n);}\n'
                'unsigned insert(unsigned x,unsigned n){return x*wf_zlib_power[n];}\n'
                'unsigned observe(const code *p){code c=wf_zlib_code_load(p);return c.op+c.bits+c.val;}\n'
                'typedef char alignment_proof[__alignof__(code)>=4?1:-1];\n')
            cross = [compiler, '-m2', '-O2', '-fno-peephole2', '-fno-builtin', '-DZ_SOLO', '-DNO_GZIP',
                     '-I'+str(ROOT/'port/zlib'), '-S']
            run([*cross, probe, '-o', build/'probe.s'], 'compile-bits-sh2')
            assembly = (build/'probe.s').read_text().lower()
            if not all(op in assembly for op in ('dmulu.l', 'mul.l')) or re.search(r'__(?:ashl|ashr|lshr)', assembly):
                raise AssertionError('SH2 bit helper code generation regressed')
            body = re.search(r'_observe:\n(.*?)\.size\s+_observe,', assembly, re.S).group(1)
            loads = re.findall(r'mov\.l\s+@r\d+,r\d+', body)
            if len(loads) != 1 or re.search(r'mov\.[bw]\s+@', body):
                raise AssertionError('Huffman entry is not a single aligned word load')
            report['huffman_entry_word_loads'] = len(loads)
            run([*cross, ROOT/'port/zlib/adler32.c', '-o', build/'adler32.s'], 'compile-adler-sh2')
            report['adler_instruction_comparisons'] = verify_adler((build/'adler32.s').read_text())
            print('PASS SH2 multiplication codegen; Adler generated-block comparisons:', report['adler_instruction_comparisons'])
        report['status'] = 'PASS'
    except Exception as error:
        report['status'] = 'FAIL'; report['error'] = repr(error);raise
    finally:
        (build/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')


if __name__ == '__main__':main()
