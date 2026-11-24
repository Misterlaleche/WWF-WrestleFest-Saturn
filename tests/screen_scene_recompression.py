#!/usr/bin/env python3
"""Isolated lossless WFSC recompression screen; never changes runtime/assets.

Keep the original seven-file directory, logical references, block boundaries and
first-use deduplication. Compare each compressed candidate with Python zlib and
the independently supplied canonical source files. This measures storage only.
"""
import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import struct
import time
import zlib

ROOT = Path(__file__).resolve().parents[1]
NAMES = ('R18RING.BIN', 'R18MENU.BIN', 'R18MAP1.BIN', 'R18MAP2.BIN',
         'R18MAP4.BIN', 'R18MAP5.BIN', 'R18MAP6.BIN')
LENGTHS = (114918, 124938, 127138, 95978, 129998, 115568, 95198)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def align(value, boundary):
    return (value + boundary - 1) // boundary * boundary


def unpack(blob, canonical):
    magic, version, size, files, blocks, data_at, total, checksum = struct.unpack_from('>4s7I', blob)
    assert (magic, version, size, files, total) == (b'WFSC', 2, 8192, 7, len(blob))
    assert data_at == align(32 + 7*16 + blocks*8, 32)
    assert zlib.adler32(blob[32:data_at]) & 0xffffffff == checksum
    spans = [struct.unpack_from('>2I', blob, 32 + 7*16 + i*8) for i in range(blocks)]
    physical, references, packed, raw = {}, [], [], []
    cursor = data_at
    for start, end in spans:
        assert data_at <= start < end <= total
        if (start, end) not in physical:
            assert start == cursor
            cursor = end
            physical[start, end] = len(packed)
            stream = blob[start:end]
            decoder = zlib.decompressobj()
            data = decoder.decompress(stream) + decoder.flush()
            assert decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
            packed.append(stream)
            raw.append(data)
        references.append(physical[start, end])
    assert cursor == total
    first = 0
    for i, name in enumerate(NAMES):
        length, begin, count, reserved = struct.unpack_from('>4I', blob, 32+i*16)
        assert (length, begin, count, reserved) == (LENGTHS[i], first, (length+8191)//8192, 0)
        contents = b''.join(raw[references[b]] for b in range(begin, begin+count))
        assert contents == canonical[name], name
        for b in range(count):
            assert len(raw[references[begin+b]]) == min(8192, length-b*8192)
        first += count
    assert first == blocks
    return dict(data_at=data_at, directory=blob[32:32+7*16], references=references,
                packed=packed, raw=raw)


def repack(base, payloads):
    offsets = [base['data_at']]
    for payload in payloads:
        offsets.append(offsets[-1] + len(payload))
    metadata = base['directory'] + b''.join(struct.pack('>2I', offsets[i], offsets[i+1])
                                              for i in base['references'])
    metadata += bytes(base['data_at'] - 32 - len(metadata))
    return struct.pack('>4s7I', b'WFSC', 2, 8192, 7, len(base['references']),
                       base['data_at'], offsets[-1], zlib.adler32(metadata) & 0xffffffff) + metadata + b''.join(payloads)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pack', type=Path, default=ROOT/'cd/SCENES.WFP')
    p.add_argument('--source-dir', type=Path, default=ROOT/'source_assets')
    p.add_argument('--output', type=Path, default=ROOT/'build/perf-scene-pack-20261113')
    p.add_argument('--iterations', type=int, nargs='+', default=[15, 100])
    p.add_argument('--workers', type=int, default=2)
    a = p.parse_args()
    import zopfli
    import zopfli.zlib
    import zopfli.zopfli
    assert all(n > 0 for n in a.iterations)
    out = a.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    inputs = [a.pack.resolve(), *(a.source_dir.resolve()/name for name in NAMES)]
    protected = {str(path): sha(path.read_bytes()) for path in inputs}
    canonical = {name: (a.source_dir/name).read_bytes() for name in NAMES}
    original = a.pack.read_bytes()
    base = unpack(original, canonical)
    assert repack(base, base['packed']) == original
    report = dict(status='RUNNING', scope='Lossless storage screen; no Saturn timing claim.',
                  input_sha256=protected, script_sha256=sha(Path(__file__).read_bytes()),
                  zopfli_version=zopfli.__version__, python_zlib_version=zlib.ZLIB_RUNTIME_VERSION,
                  compressor_library_sha256=sha(Path(zopfli.zopfli.__file__).read_bytes()),
                  files=len(NAMES), logical_blocks=len(base['references']), unique_blocks=len(base['raw']),
                  original_bytes=len(original), original_sha256=sha(original),
                  existing_reserved_bytes=282624, existing_cache_bytes=565248,
                  preserved_music_prefix_bytes=114688, variants={})
    best = list(base['packed'])
    chosen = ['original'] * len(best)

    def save(label, payloads, elapsed):
        blob = repack(base, payloads)
        check = unpack(blob, canonical)
        assert check['raw'] == base['raw'] and check['references'] == base['references']
        path = out/label/'SCENES.WFP'
        path.parent.mkdir(exist_ok=True)
        path.write_bytes(blob)
        reserve = align(len(blob), 4096)
        result = dict(path=str(path), bytes=len(blob), sha256=sha(blob), seconds=elapsed,
                      savings=len(original)-len(blob), minimum_4k_reservation=reserve,
                      whole_4k_slots_freed=(282624-reserve)//4096,
                      potential_cache_bytes=565248+282624-reserve,
                      source_files_byte_identical=True, logical_references_unchanged=True,
                      payload_bytes=[len(item) for item in payloads])
        report['variants'][label] = result
        (out/'summary.json').write_text(json.dumps(report, indent=2)+'\n')
        print(label, json.dumps({key: result[key] for key in ('bytes', 'savings', 'minimum_4k_reservation',
                                                           'whole_4k_slots_freed', 'seconds')}), flush=True)

    save('baseline', best, 0)
    try:
        for iterations in a.iterations:
            began = time.monotonic()
            options = dict(numiterations=iterations, blocksplitting=1, blocksplittinglast=0, blocksplittingmax=15)
            payloads = []
            def compress(pair):
                index, data = pair
                stream = zopfli.zlib.compress(data, **options)
                assert zlib.decompress(stream) == data
                return index, stream
            with concurrent.futures.ThreadPoolExecutor(max_workers=a.workers) as pool:
                for index, stream in pool.map(compress, enumerate(base['raw'])):
                    assert index == len(payloads)
                    payloads.append(stream)
                    if len(stream) < len(best[index]):
                        best[index] = stream
                        chosen[index] = f'zopfli-{iterations}'
                    if (index+1) % 16 == 0:
                        print(f'zopfli-{iterations}: {index+1}/{len(base["raw"])} streams', flush=True)
            save(f'zopfli-{iterations}', payloads, time.monotonic()-began)
            report['variants'][f'zopfli-{iterations}']['options'] = options
            save('best', best, time.monotonic()-began)
        report['chosen_per_unique_block'] = chosen
        report['status'] = 'PASS'
    finally:
        report['changed_inputs'] = [path for path, digest in protected.items() if sha(Path(path).read_bytes()) != digest]
        assert not report['changed_inputs'], report['changed_inputs']
        (out/'summary.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
