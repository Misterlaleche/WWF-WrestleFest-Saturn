#!/usr/bin/env python3
"""Paired public-ABI differential for the refill and table-builder changes.

The reference uses independently compiled, pinned C18/C22 inffast.c and
inflate_table implementations. Python zlib verifies valid corpus bytes. Streaming,
fixed/dynamic/stored trees, dictionaries, malformed/truncated/checksum streams
and permissive/strict distances exercise public consumption and resume effects.
A second build observes the two original refill sites without changing results.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import zlib

ROOT = Path(__file__).resolve().parents[1]
BASE_SHA = '0e516d4f01cd1b10bb5498afcaef3aa8b64e4c1e5307eca8dd43a88969d42f16'
TABLE_BASE_SHA = '1402d46f971ea81032b020aea64dec36268d90ac92832122dbf809dcb80202bf'
VALID, PERMISSIVE, TRUNCATED, DATA_ERROR, PREFIX = 1, 2, 4, 8, 16
LENGTHS = [3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35,
           43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258]
LENGTH_BITS = [0]*8 + [1]*4 + [2]*4 + [3]*4 + [4]*4 + [5]*4 + [0]
DISTANCES = [1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
             257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193,
             12289, 16385, 24577]
DISTANCE_BITS = [0]*4 + [n for n in range(1, 14) for _ in range(2)]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Bits:
    def __init__(self):
        self.data, self.value, self.count = bytearray(), 0, 0

    def put(self, value, count):
        assert 0 <= value < 1 << count
        self.value |= value << self.count
        self.count += count
        while self.count >= 8:
            self.data.append(self.value & 255)
            self.value >>= 8
            self.count -= 8

    def code(self, value, count):
        reverse = int(f'{value:0{count}b}'[::-1], 2)
        self.put(reverse, count)

    def fixed(self, symbol):
        if symbol < 144:
            self.code(0x30 + symbol, 8)
        elif symbol < 256:
            self.code(0x190 + symbol - 144, 9)
        elif symbol < 280:
            self.code(symbol - 256, 7)
        else:
            self.code(0xc0 + symbol - 280, 8)

    def match(self, length, distance):
        li = max(i for i, base in enumerate(LENGTHS) if base <= length)
        self.fixed(257 + li)
        self.put(length - LENGTHS[li], LENGTH_BITS[li])
        di = max(i for i, base in enumerate(DISTANCES) if base <= distance)
        self.code(di, 5)
        self.put(distance - DISTANCES[di], DISTANCE_BITS[di])

    def finish(self):
        if self.count:
            self.data.append(self.value)
            self.value = self.count = 0
        return bytes(self.data)


def fixed_stream(length=None, distance=None, *, invalid_symbol=None, invalid_dist=False):
    bits = Bits()
    bits.put(3, 3)  # Final fixed-Huffman block.
    if invalid_symbol is not None:
        bits.fixed(invalid_symbol)
    elif invalid_dist:
        bits.fixed(257)
        bits.code(30, 5)
    else:
        bits.match(length, distance)
        bits.fixed(ord('R'))
    bits.fixed(256)
    return bits.finish()


def match_bytes(dictionary, prefix, length, distance):
    history = bytearray(dictionary + prefix)
    result = bytearray(prefix)
    for _ in range(length):
        byte = history[-distance] if distance <= len(history) else 0
        result.append(byte)
        history.append(byte)
    result.append(ord('R'))
    return bytes(result)


def corpus(path):
    rng = random.Random(0xC012F00D)
    cases = []

    def add(name, packed, plain=b'', *, wbits=15, flags=VALID, dictionary=b'',
            end=None, first_in=0, first_out=0):
        if flags & VALID:
            kwargs = {'zdict': dictionary} if dictionary else {}
            decoder = zlib.decompressobj(wbits=wbits, **kwargs)
            actual = decoder.decompress(packed) + decoder.flush()
            assert decoder.eof and actual == plain, name
            actual_end = len(packed) - len(decoder.unused_data)
            if end is not None:
                assert actual_end == end, name
            end = actual_end
        cases.append(dict(name=name, packed=packed, plain=plain, wbits=wbits,
                          flags=flags, dictionary=dictionary,
                          end=len(packed) if end is None else end,
                          first_in=first_in, first_out=first_out))

    def compress(data, wbits, level=6, strategy=zlib.Z_DEFAULT_STRATEGY, dictionary=b''):
        kwargs = {'zdict': dictionary} if dictionary else {}
        encoder = zlib.compressobj(level, zlib.DEFLATED, wbits, 8, strategy, **kwargs)
        return encoder.compress(data) + encoder.flush()

    # Locked block444 independently covers every bits=0..14 entry at both
    # pair sites, including the rare zero-bit pre-distance refill. The
    # expected bytes come from the committed canonical graphics source.
    fixture = ROOT/'tests/native/refill-boundary-444.zlib'
    canonical = ROOT/'source_assets/SPRITES.PAK'
    assert sha(fixture) == '4447b20cdf81ffecf5a603b8be6efe516ef0584002043501babffb32d2ac8d96'
    assert sha(canonical) == '130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31'
    add('canonical-boundary-444', fixture.read_bytes(),
        canonical.read_bytes()[444*4096:445*4096], first_out=8192)

    random_period = bytes(rng.getrandbits(8) for _ in range(20000))
    small_random = bytes(rng.getrandbits(8) for _ in range(8192))
    samples = [('empty', b''), ('literal', b'R'), ('runs', b'A'*98000),
               ('cycles', bytes(range(256))*370),
               ('long-distance', random_period*5 + random_period[:123]),
               ('random', small_random),
               ('mixed', (b'WWF WRESTLEFEST\0'*1500 + small_random)*3)]
    seeds = []
    for name, plain in samples:
        for wbits in (15, -15):
            packed = compress(plain, wbits)
            label = f'{name}-{wbits}'
            add(label, packed, plain, wbits=wbits)
            if name in ('cycles', 'long-distance'):
                add(label+'-trailing', packed+b'ignored trailer'*2, plain, wbits=wbits)
                seeds.append((label, packed, plain, wbits))
    for wbits in (15, -15):
        for strategy, label in ((zlib.Z_FIXED, 'fixed'), (zlib.Z_HUFFMAN_ONLY, 'huffman')):
            plain = samples[-1][1]
            add(label+str(wbits), compress(plain, wbits, strategy=strategy), plain, wbits=wbits)
        add('stored'+str(wbits), compress(small_random, wbits, level=0), small_random, wbits=wbits)
    dictionary = bytes(rng.getrandbits(8) for _ in range(32768))
    plain = dictionary[-18000:]*3 + b'final literal'
    for wbits in (15, -15):
        add('preset'+str(wbits), compress(plain, wbits, dictionary=dictionary), plain,
            wbits=wbits, dictionary=dictionary)
    for name, history, length, distance in (
            ('window-zero-only', dictionary, 3, 32),
            ('window-zero-output', dictionary, 258, 4),
            ('window-contiguous-only', dictionary[:128], 3, 64),
            ('window-contiguous-output', dictionary[:128], 258, 64)):
        body = fixed_stream(length, distance)
        add(name, body+b'\0'*16, match_bytes(history, b'', length, distance), wbits=-15,
            dictionary=history, end=len(body))
    prefix = bytes(range(100))
    stored = b'\0'+struct.pack('<HH', len(prefix), 65535-len(prefix))+prefix
    for length, label in ((3, 'end'), (150, 'head'), (258, 'output')):
        body = stored+fixed_stream(length, 200)
        add('window-wrap-'+label, body+b'\0'*16, match_bytes(dictionary, prefix, length, 200),
            wbits=-15, dictionary=dictionary, end=len(body), first_in=len(stored), first_out=100)
    for name, history, length, distance in (
            ('permissive-zero-only', b'', 3, 24),
            ('permissive-zero-output', b'', 11, 7),
            ('permissive-zero-window', dictionary[:16], 30, 24)):
        body = fixed_stream(length, distance)
        add(name, body+b'\0'*16, match_bytes(history, b'', length, distance), wbits=-15,
            dictionary=history, flags=PERMISSIVE|DATA_ERROR, end=len(body))
    add('too-far-distance', fixed_stream(3, 32768)+b'\0'*16, wbits=-15, flags=DATA_ERROR)
    add('reserved-block-type', b'\x07'+b'\0'*20, wbits=-15, flags=DATA_ERROR)
    add('reserved-length-code', fixed_stream(invalid_symbol=286)+b'\0'*16,
        wbits=-15, flags=DATA_ERROR)
    add('reserved-distance-code', fixed_stream(invalid_dist=True)+b'\0'*16,
        wbits=-15, flags=DATA_ERROR)
    add('bad-wrapper-header', b'\x00\x00'+b'\0'*20, flags=DATA_ERROR)
    source = compress(samples[-1][1], 15)
    bad = bytearray(source)
    bad[-1] ^= 1
    add('bad-checksum', bytes(bad), samples[-1][1], flags=DATA_ERROR|PREFIX)
    for cut in sorted({0, 1, 2, 5, 6, len(source)//2, len(source)-2, len(source)-1}):
        add('truncated-'+str(cut), source[:cut], samples[-1][1], flags=TRUNCATED|PREFIX)
    for i in range(64):
        name, source, _, wbits = seeds[i % len(seeds)]
        changed = bytearray(source)
        for _ in range(1 + i % 3):
            offset = rng.randrange(len(changed))
            changed[offset] ^= 1 << rng.randrange(8)
        add(f'mutated-{i}-{name}', bytes(changed), wbits=wbits, flags=0)

    with path.open('wb') as out:
        out.write(b'WFZW0001'+struct.pack('<I', len(cases)))
        for case in cases:
            label = case['name'].encode('ascii')
            out.write(struct.pack('<Ii7I', len(label), case['wbits'], case['flags'],
                len(case['packed']), len(case['plain']), len(case['dictionary']),
                case['end'], case['first_in'], case['first_out']))
            out.write(label+case['packed']+case['plain']+case['dictionary'])
    return [{key: value for key, value in case.items() if key not in ('packed', 'plain', 'dictionary')}
            | {'packed_bytes': len(case['packed']), 'plain_bytes': len(case['plain']),
               'dictionary_bytes': len(case['dictionary'])} for case in cases]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build/refill-differential')
    parser.add_argument('--reference-source', type=Path,
                        default=ROOT/'tests/native/reference_c18_inffast.c')
    parser.add_argument('--compiler', type=Path,
                        default=ROOT/'sdk/Compiler/LINUX/bin/sh-none-elf-gcc-8.2.0')
    parser.add_argument('--compiler-include', type=Path,
                        default=ROOT/'sdk/Compiler/WINDOWS/sh-elf/include')
    parser.add_argument('--codegen-only', action='store_true')
    parser.add_argument('--skip-codegen', action='store_true')
    parser.add_argument('--configuration', choices=('normal', 'permissive', 'strict-permissive'))
    parser.add_argument('--mutant', choices=('byte-order', 'advance', 'bit-count'))
    parser.add_argument('--table-mutant', choices=('code-width', 'packed-bit'),
                        help='Negative control changes only the candidate table builder; must fail semantically.')
    args = parser.parse_args()
    assert not (args.codegen_only and args.skip_codegen)
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    reference = args.reference_source.resolve()
    reference_table = ROOT/'tests/native/reference_c23_inftrees/inftrees.c'
    sources = [Path(__file__).resolve(), ROOT/'tests/test_inflate_refill.c', reference,
               reference_table, ROOT/'tests/native/refill-boundary-444.zlib', ROOT/'source_assets/SPRITES.PAK']
    sources += sorted((ROOT/'port/zlib').glob('*.h'))
    sources += [ROOT/'port/zlib'/name for name in
                ('inflate.c', 'inffast.c', 'inftrees.c', 'adler32.c', 'zutil.c')]
    report = dict(status='RUNNING', reference_sha256=BASE_SHA,
                  reference_table_sha256=TABLE_BASE_SHA,
                  reference_table_independent=not args.codegen_only, commands=[],
                  source_sha256={str(path): sha(path) for path in sources},
                  configurations={}, mutant=args.mutant, table_mutant=args.table_mutant,
                  leak_check='LSan is unavailable under the executor tracer; both decoders guard and balance every allocation explicitly')

    def run(command, label, directory=build):
        command = [str(part) for part in command]
        # A corrupted table can otherwise loop inside a decoder call forever.
        # Timeouts are failures, never accepted as semantic negative controls.
        timeout = 60 if (args.mutant or args.table_mutant) and label.startswith('run-') else None
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=timeout,
            env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',
                     UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
        log = directory/(label+'.log')
        log.write_text(result.stdout+result.stderr)
        report['commands'].append(dict(argv=command, cwd=str(ROOT),
                                      returncode=result.returncode, log=str(log)))
        result.check_returncode()
        return result.stdout

    try:
        assert sha(reference) == BASE_SHA, 'Reference is not the pinned C18/C22 inffast.c'
        assert sha(reference_table) == TABLE_BASE_SHA, 'Reference table builder is not the pinned C22 implementation'
        table_code = (ROOT/'port/zlib/inftrees.c').read_text()
        if args.table_mutant == 'code-width':
            original = '        here.bits = (unsigned char)(len - drop);'
            assert table_code.count(original) == 2
            table_code = table_code.replace(original,
                '        here.bits = (unsigned char)((len - drop) ^ 1u);', 1)
        elif args.table_mutant == 'packed-bit':
            original = '        packed = wf_table_pack(here);'
            assert table_code.count(original) == 1
            table_code = table_code.replace(original,
                '        packed = wf_table_pack(here); '
                'if (type == LENS && here.op == 0) '
                'packed ^= wf_table_pack((code){0, 0, 1});')
        candidate_table = build/'inftrees-candidate.c'
        candidate_table.write_text(table_code)
        code = (ROOT/'port/zlib/inffast.c').read_text()
        if args.mutant == 'byte-order':
            needle = '((unsigned long)in[0] | ((unsigned long)in[1] << 8))'
            assert code.count(needle) == 2
            code = code.replace(needle, '((unsigned long)in[1] | ((unsigned long)in[0] << 8))', 1)
        elif args.mutant == 'advance':
            assert code.count('in += 2;') == 2
            code = code.replace('in += 2;', 'in += 1;', 1)
        elif args.mutant == 'bit-count':
            assert code.count('bits += 16;') == 2
            code = code.replace('bits += 16;', 'bits += 15;', 1)
        candidate = build/'inffast-candidate.c'
        candidate.write_text(code)
        if not args.skip_codegen:
            assembly = build/'inffast-sh2.s'
            run([args.compiler.resolve(), '-m2', '-O2', '-fno-builtin', '-fno-peephole2',
                 '-fmerge-all-constants', '-std=gnu99', '-DZ_SOLO', '-DNO_GZIP',
                 '-I'+str(ROOT/'port/zlib'), '-I'+str(args.compiler_include.resolve()),
                 '-S', candidate, '-o', assembly], 'codegen-build')
            body = assembly.read_text().split('_inflate_fast:\n', 1)[1].split(
                '\t.size\t_inflate_fast, .-_inflate_fast', 1)[0]
            count = len(re.findall(r'^\s*mul\.l\s', body, re.M))
            calls = re.findall(r'^\s*(?:jsr|bsr)\s.*$', body, re.M)
            report['codegen'] = dict(mull_count=count, expected_mull_count=5, calls=calls,
                assembly_sha256=sha(assembly), compiler_sha256=sha(args.compiler.resolve()),
                scope='Static codegen shape: two pair refills use one MUL.L each; three other byte insertions remain. This is not a target cycle/timing assertion.')
            assert count == 5, f'Expected two fused refills (5 total MUL.L), observed {count}'
            assert not calls, 'A helper call was introduced into inflate_fast'
        if not args.codegen_only:
            report['corpus'] = corpus(build/'corpus.bin')
            report['corpus_sha256'] = sha(build/'corpus.bin')
            report['python_zlib_version'] = zlib.ZLIB_RUNTIME_VERSION
            site = 0

            def observe(match):
                nonlocal site
                result = match[0]+'\n'+match['indent']+'    wf_test_refill_observe('+str(site)+',bits,(uintptr_t)in);'
                site += 1
                return result

            observed = re.sub(r'(?P<indent>^[ \t]*)if \(bits < 15\) \{', observe, code, flags=re.M)
            assert site == 2
            instrumented = build/'inffast-observed.c'
            instrumented.write_text('#include <stdint.h>\n'
                'extern void wf_test_refill_observe(unsigned,unsigned,uintptr_t);\n'+observed)
            exports = re.findall(r'^.*?\bZEXPORT\s+(\w+)\s*\(',
                                 (ROOT/'port/zlib/inflate.c').read_text(), re.M)
            assert len(exports) >= 15 and len(exports) == len(set(exports))
            reference_exports = exports+['inflate_fast', 'inflate_table', 'inflate_copyright']
            renames = ['-D'+name+'=wf_reference_'+name for name in reference_exports]
            report['renamed_reference_exports'] = reference_exports
            common = ['cc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                      '-Wno-implicit-fallthrough', '-fsanitize=address,undefined',
                      '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', '-DZ_SOLO', '-DNO_GZIP',
                      '-I'+str(ROOT/'port/zlib')]
            configurations = [('normal', []),
                ('permissive', ['-DINFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR']),
                ('strict-permissive', ['-DINFLATE_STRICT', '-DINFLATE_ALLOW_INVALID_DISTANCE_TOOFAR_ARRR'])]
            for name, extra in configurations:
                if args.configuration and args.configuration != name:
                    continue
                directory = build/name
                directory.mkdir(exist_ok=True)
                flags, objects = common+extra, []
                for source, output, additions in [
                        (ROOT/'port/zlib/inflate.c', 'inflate', []),
                        (ROOT/'port/zlib/inflate.c', 'reference-inflate', renames),
                        (reference, 'reference-inffast', renames),
                        (reference_table, 'reference-inftrees', renames),
                        (candidate_table, 'inftrees', []),
                        *[(ROOT/'port/zlib'/(unit+'.c'), unit, [])
                          for unit in ('adler32', 'zutil')]]:
                    obj = directory/(output+'.o')
                    run([*flags, *additions, '-c', source, '-o', obj], 'compile-'+output, directory)
                    objects.append(obj)
                results = {}
                for kind, source, additions in [('plain', candidate, []),
                        ('observed', instrumented, ['-DWF_TEST_INSTRUMENT'])]:
                    binary = directory/kind
                    run([*flags, *additions, ROOT/'tests/test_inflate_refill.c', source,
                         *objects, '-o', binary], 'link-'+kind, directory)
                    results[kind] = json.loads(run([binary, build/'corpus.bin'], 'run-'+kind, directory))
                    results[kind]['binary_sha256'] = sha(binary)
                assert {k: v for k, v in results['observed'].items() if k not in ('refill_cases', 'binary_sha256')} == {
                    k: v for k, v in results['plain'].items() if k not in ('refill_cases', 'binary_sha256')}
                report['configurations'][name] = results
                print(name+': '+json.dumps({k: v for k, v in results['plain'].items()
                      if k != 'refill_cases'}, sort_keys=True), flush=True)
            report['generated_sha256'] = {str(path): sha(path) for path in (candidate, instrumented, candidate_table)}
        report['status'] = 'PASS'
    except Exception as error:
        report['status'], report['error'] = 'FAIL', repr(error)
        raise
    finally:
        report['changed_sources'] = [str(path) for path in sources
            if sha(path) != report['source_sha256'][str(path)]]
        if report['changed_sources']:
            report['status'] = 'FAIL'
        (build/'report.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    assert not report['changed_sources'], 'Source input changed while gate was running'


if __name__ == '__main__':
    main()

