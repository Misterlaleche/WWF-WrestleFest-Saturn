#!/usr/bin/env python3
"""Exhaust the real Huffman increment against independent bit reversal.

A changed carry start bit, skipped carry clear, or missing final carry set
must disagree with reverse(reverse(huff) + 1 modulo 2**len). The increment
block is extracted from production, not copied into a test implementation.
The full inflate_table differential separately checks its caller invariants.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
START = '        /* backwards increment the len-bit code huff */'
END = '        /* go to next symbol, update count, len */'

DRIVER = r'''
#include <stdint.h>
#include <stdio.h>
#include "wf_sh2_bits.h"

static unsigned increment(unsigned huff, unsigned len)
{
    unsigned incr;
@BLOCK@
    return huff;
}

/* Independent definition: increment an ordinary integer after bit reversal. */
static unsigned reverse_bits(unsigned value, unsigned width)
{
    unsigned result = 0;
    for (unsigned bit = 0; bit < width; ++bit) {
        result = result * 2u + value % 2u;
        value /= 2u;
    }
    return result;
}

int main(void)
{
    unsigned cases = 0;
    for (unsigned len = 1; len <= 15; ++len) {
        unsigned bound = 1u << len;
        for (unsigned huff = 0; huff < bound; ++huff) {
            unsigned want = reverse_bits((reverse_bits(huff, len) + 1u) % bound, len);
            unsigned got = increment(huff, len);
            if (got != want) {
                fprintf(stderr, "MISMATCH carry len=%u huff=%u got=%u expected=%u\n",
                        len, huff, got, want);
                return 90;
            }
            ++cases;
        }
    }
    printf("{\"status\":\"PASS\",\"cases\":%u}\n", cases);
    return 0;
}
'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT/'port/zlib/inftrees.c')
    parser.add_argument('--output', type=Path, default=ROOT/'build/inftrees-carry')
    args = parser.parse_args()
    source, out = args.source.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    text = source.read_text()
    assert text.count(START) == text.count(END) == 1, 'Cannot isolate the production increment block'
    block = text.split(START, 1)[1].split(END, 1)[0]
    protected = {str(path): sha(path) for path in
                 (source, Path(__file__).resolve(), ROOT/'port/zlib/wf_sh2_bits.h')}
    variants = {'candidate': block}
    needle = 'incr = wf_zlib_power[len - 1];'
    assert block.count(needle) == 1
    variants['negative-carry-start'] = block.replace(needle, 'incr = wf_zlib_power[len];')
    # These two controls apply when the recovered XOR implementation is present.
    clear = '            huff ^= incr;'
    final = '        huff ^= incr;'
    if clear in block and block.count('huff ^= incr;') == 2:
        variants['negative-carry-clear'] = block.replace(clear, '            /* omitted carry clear */', 1)
        before, after = block.rsplit(final, 1)
        variants['negative-carry-set'] = before + '        /* omitted final carry set */' + after
    report = dict(status='RUNNING', scope='Complete legal Huffman increment domain; no timing claim.',
                  source_sha256=protected[str(source)], commands=[], variants={})
    flags = ['cc', '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
             '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-fno-pie', '-no-pie', '-I'+str(ROOT/'port/zlib')]
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')

    def run(command, label):
        command = [str(value) for value in command]
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True)
        log = out/(label+'.log')
        log.write_text(result.stdout + result.stderr)
        report['commands'].append(dict(argv=command, returncode=result.returncode, log=str(log)))
        return result

    try:
        for label, actual_block in variants.items():
            translation = out/(label+'.c')
            translation.write_text(DRIVER.replace('@BLOCK@', actual_block))
            binary = out/label
            compiled = run([*flags, translation, '-o', binary], label+'-build')
            assert compiled.returncode == 0, compiled.stderr
            tested = run([binary], label+'-run')
            if label == 'candidate':
                assert tested.returncode == 0, tested.stderr
                result = json.loads(tested.stdout)
                assert result == {'status': 'PASS', 'cases': 65534}
            else:
                assert tested.returncode == 90 and tested.stderr.startswith('MISMATCH carry '), (
                    label, tested.returncode, tested.stderr)
                result = dict(status='EXPECTED_CARRY_MISMATCH', diagnostic=tested.stderr.strip())
            report['variants'][label] = dict(result, binary_sha256=sha(binary))
            print(label+': '+result['status'], flush=True)
        report['status'] = 'PASS'
    except Exception as error:
        report['status'], report['error'] = 'FAIL', repr(error)
        raise
    finally:
        report['changed_sources'] = [name for name, digest in protected.items() if sha(Path(name)) != digest]
        if report['changed_sources']:
            report['status'] = 'FAIL'
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    assert not report['changed_sources'], 'Source/header changed while checking carry'


if __name__ == '__main__':
    main()
