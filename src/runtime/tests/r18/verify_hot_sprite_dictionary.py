#!/usr/bin/env python3
"""Verify exact R18H/R18D packs in the real native cache, including ASan.

Pass a sorted measured tile-ID union. Canonical hints/assets are read-only.
"""
import argparse
import hashlib
import importlib.util
import json
import os
import re
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('ids', type=Path)
p.add_argument('--output', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'audit/runtime18-cd/SPRITES.PAK').read_bytes()
measured_ids = [int(x) for x in a.ids.read_text().splitlines() if x and not x.startswith('#')]
assert measured_ids == sorted(set(measured_ids)) and measured_ids
memory = (root / 'include/wf_saturn_memory.h').read_text()
cache_bytes = int(re.search(r'#define WF_SATURN_LWRAM_TILE_BYTES\s+(0x[0-9a-f]+)u', memory)[1], 16)
index_bytes = int(re.search(r'#define WF_SATURN_HWRAM_HOT_INDEX_BYTES\s+(0x[0-9a-f]+)u', memory)[1], 16)
scene_bytes = int(re.search(r'#define WF_SATURN_LWRAM_SCENE_BYTES\s+(0x[0-9a-f]+)u', memory)[1], 16)
assert max((root / 'audit/runtime18-cd' / name).stat().st_size for name in ('R18RING.BIN', 'R18MAP2.BIN')) <= scene_bytes
files = ['src/wf_r18_present.c', 'src/wf_r18_metasprite.c', 'src/wf_actor.c',
         'src/wf_saturn_yaul_backend.c', 'src/wf_saturn_memory.c',
         'src/wf_saturn_render_bridge.c', 'src/wf_vdp1_plan.c']
extra = ['tests/r18/test_hot_sprites.c', 'tools/build_r18_hot_sprites.py', 'include/wf_saturn_memory.h']
report = {'result': 'pass', 'build_variants': ['normal', 'address+undefined sanitizers'],
          'sources_sha256': {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
                                           for name in files + extra}, 'cases': []}
with tempfile.TemporaryDirectory(prefix='wf-hot-dictionary-') as directory:
    temp = Path(directory)
    shutil.copytree(root / 'include', temp / 'include')
    for name in files + extra:
        (temp / name).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(root / name, temp / name)
    pack = temp / 'SPRITES.PAK'
    pack.write_bytes(source)
    executables = []
    for kind, flags in [('normal', []), ('asan', ['-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
        binary = temp / ('test-' + kind)
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O1',
                        '-Iinclude', *flags, *files, *extra[:1], '-o', str(binary)], cwd=temp, check=True)
        executables.append(binary)
    env = {**os.environ, 'ASAN_OPTIONS': 'halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1'}

    def build(name, ids, mode='auto', width=None):
        table = temp / (name + '.txt')
        output = temp / (name + '.bin')
        table.write_text(''.join(f'{tile}\n' for tile in ids))
        subprocess.run([sys.executable, str(temp / extra[1]), str(table), str(pack),
                        str(output), '--format', mode,
                        *([] if width is None else ['--chunk-bytes', str(width)])],
                       check=True, stdout=subprocess.PIPE, text=True)
        return output

    def check(name, output, reject=False, reference=pack):
        for binary in executables:
            result = subprocess.run([str(binary), str(reference), str(output), *(['--reject'] if reject else [])],
                                    env=env, text=True, capture_output=True)
            assert result.returncode == 0, (name, binary.name, result.stdout, result.stderr)
        report['cases'].append(name)

    # The last raw case must leave one 128-byte fallback slot. Above that
    # boundary, auto chooses R18D; expecting an R18H literal was invalid.
    for count in sorted({1204, 2044, 2045, 2195, 4092, 4093, 4513,
                         cache_bytes // 128 - 1}):
        ids = [i * (17 if count <= 2195 else 13 if count <= 4092 else 11) for i in range(count)]
        output = build(f'legacy-{count}', ids)
        data = output.read_bytes()
        header_bytes = 4096 if count <= 2044 else 8192 if count <= 4092 else 16384
        literal = bytearray(header_bytes)
        literal[:8] = b'R18H' + struct.pack('>HH', count, 128)
        for i, tile in enumerate(ids):
            struct.pack_into('>H', literal, 8 + i * 2, tile)
        assert data == bytes(literal) + b''.join(source[t * 128:(t + 1) * 128] for t in ids)
        check(f'legacy-{count}', output)
    for width in (4, 16, 32):
        for count in (1, 2, 2036, 2037, 4084, 4085, 5120):
            name = f'dictionary-{width}-{count}'
            output = build(name, list(range(count)), 'dictionary', width)
            dictionary_data = output.read_bytes()
            assert dictionary_data[:4] == b'R18D'
            assert int.from_bytes(dictionary_data[12:16], 'big') == count * (128 // width) * 2
            check(name, output)
    check('explicit-16-byte-width-forces-dictionary', build('auto16-small', [0, 1], width=16))
    # Boundary-only synthetic source: each logical tile has exact zero bytes.
    # Canonical artwork is still checked independently for every measured ID.
    zeros = temp / 'zero-tiles.pak'
    zeros.write_bytes(bytes(len(source)))
    for count, width in ((1792, 4), (1793, 4), (8180, 4), (8180, 16), (8180, 32)):
        mapping = count * (128 // width) * 2
        header_bytes = 4096 if 24 + count * 2 <= 4096 else 8192 if 24 + count * 2 <= 8192 else 16384
        header = bytearray(header_bytes)
        header[:24] = b'R18D' + struct.pack('>HH4I', count, 128, 1, mapping, width, header_bytes)
        for i in range(count):
            struct.pack_into('>H', header, 24 + i * 2, i)
        path = temp / f'capacity-{width}.bin'
        path.write_bytes(header + bytes((mapping + 31) // 32 * 32 + width))
        check(f'index-table-{width}-byte-{count}-tiles', path, reference=zeros)
    # The largest BE16 index is valid. All dictionary chunks are zero only
    # in this independent boundary fixture, including chunk65535.
    # The synthetic all-65536-chunk dictionary is much larger than artwork.
    # Bound its index table by the actual LWRAM lease plus HWRAM prefix.
    count = min(8180,(cache_bytes - 128 - 65536 * 4 + index_bytes) // 64)
    mapping = count * 64
    header = bytearray(16384)
    header[:24] = b'R18D' + struct.pack('>HH4I', count, 128, 65536, mapping, 65536 * 4, 16384)
    for i in range(count):
        struct.pack_into('>H', header, 24 + i * 2, i)
    payload = bytearray(mapping + 65536 * 4)
    payload[mapping - 2:mapping] = b'\xff\xff'
    boundary = temp / 'maximum-BE16-index.bin'
    boundary.write_bytes(header + payload)
    check('maximum-valid-BE16-index', boundary, reference=zeros)
    output = build('measured', measured_ids)
    data = output.read_bytes()
    repeated = build('measured-repeat', measured_ids).read_bytes()
    assert data == repeated, 'Dictionary output is not deterministic'
    assert data[:4] == b'R18D', 'Measured union must exercise dictionary auto-selection'
    check('measured-all-tiles', output)
    header_bytes = int.from_bytes(data[20:24], 'big')
    payload = len(data) - header_bytes
    dictionary_bytes = int.from_bytes(data[16:20], 'big')
    mapping_bytes = int.from_bytes(data[12:16], 'big')
    parts = mapping_bytes // (len(measured_ids) * 2)
    assert parts in (4, 8, 32)
    width = 128 // parts
    assert dictionary_bytes == int.from_bytes(data[8:12], 'big') * width
    spill = max(0, ((mapping_bytes + 31) // 32) * 32 - index_bytes)
    rounded = (spill + dictionary_bytes + 127) // 128 * 128
    report['measured'] = {'logical_IDs': len(measured_ids), 'chunks': int.from_bytes(data[8:12], 'big'),
                          'file_bytes': len(data), 'payload_bytes': payload,
                          'chunk_bytes': width,
                          'mapping_bytes': mapping_bytes,
                          'HWRAM_indices_bytes': min(index_bytes, ((mapping_bytes + 31) // 32) * 32),
                          'LWRAM_indices_bytes': spill,
                          'dictionary_bytes': dictionary_bytes,
                          'resident_bytes': rounded, 'cold_slots': (cache_bytes - rounded) // 128,
                          'sha256': hashlib.sha256(data).hexdigest()}
    assert rounded + 128 <= cache_bytes
    for name, ids, forced in [('forced-32-cannot-fit', measured_ids, 32),
                              ('neither-width-can-fit', [i * 8 for i in range(8180)], None)]:
        table = temp / (name + '.txt')
        output = temp / (name + '.bin')
        table.write_text(''.join(f'{tile}\n' for tile in ids))
        result = subprocess.run([sys.executable, str(temp / extra[1]), str(table), str(pack), str(output),
                                 '--format', 'dictionary',
                                 *([] if forced is None else ['--chunk-bytes', str(forced)])],
                                text=True, capture_output=True)
        if name == 'forced-32-cannot-fit' and width == 32:
            assert result.returncode == 0
        else:
            assert result.returncode != 0 and not output.exists(), (name, result.stdout, result.stderr)
        report['cases'].append(name)
    spec = importlib.util.spec_from_file_location('dictionary_builder', temp / extra[1])
    builder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(builder)
    unique_chunks = b''.join(struct.pack('>I', i) for i in range(2050 * 32))
    try:
        builder.build_pack(list(range(2050)), unique_chunks, 'dictionary', 4)
    except SystemExit as error:
        assert 'BE16' in str(error)
    else:
        raise AssertionError('Builder accepted more than65536 dictionary chunks')
    report['cases'].append('builder-BE16-index-overflow')
    malformed = {
        'truncated-first-header': data[:4095],
        'truncated-extended-header': data[:header_bytes - 1],
        'truncated-HWRAM-index-prefix': data[:header_bytes + min(mapping_bytes, index_bytes) - 1],
        'truncated-last-payload-byte': data[:-1],
    }
    if spill:
        malformed['truncated-LWRAM-index-spill'] = data[:header_bytes + mapping_bytes - 1]
    for name, value in [('zero-count', 0), ('header-ID-overflow', 65535),
                        ('dictionary-header-ID-capacity-overflow', 8181)]:
        bad = bytearray(data)
        struct.pack_into('>H', bad, 4, value)
        malformed[name] = bad
    for name, offset, value in [('chunk-overflow', 8, 0xFFFFFFFF),
                                ('mapping-overflow', 12, 0xFFFFFFFF),
                                ('dictionary-overflow', 16, 0xFFFFFFFF),
                                ('header-mismatch', 20, 0xFFFFFFFF)]:
        bad = bytearray(data)
        struct.pack_into('>I', bad, offset, value)
        malformed[name] = bad
    for name, at in [('first-index', header_bytes),
                     ('last-index', header_bytes + mapping_bytes - 2)]:
        bad = bytearray(data)
        bad[at:at + 2] = b'\xff\xff'
        malformed[name] = bad
    if mapping_bytes > index_bytes:
        for name, at in [('last-HWRAM-index', header_bytes + index_bytes - 2),
                         ('first-LWRAM-index', header_bytes + index_bytes)]:
            bad = bytearray(data)
            bad[at:at + 2] = b'\xff\xff'
            malformed[name] = bad
    # A self-consistent BE16-range dictionary occupying exactly all resident
    # slots must fail before reads; it cannot borrow the required cold slot.
    full_mapping = 8180 * 64
    full_dictionary = cache_bytes - (full_mapping - index_bytes)
    assert full_dictionary % 4 == 0 and full_dictionary // 4 <= 65536
    bad = bytearray(16384)
    bad[:24] = b'R18D' + struct.pack('>HH4I', 8180, 128, full_dictionary // 4,
                                   full_mapping, full_dictionary, 16384)
    for i in range(8180):
        struct.pack_into('>H', bad, 24 + i * 2, i)
    malformed['dictionary-with-no-cold-slot'] = bad + bytes(full_mapping + full_dictionary)
    bad = bytearray(data)
    struct.pack_into('>I', bad, 12, len(measured_ids) * 12)
    malformed['unsupported-parts-count'] = bad
    bad = bytearray(data)
    struct.pack_into('>I', bad, 12, len(measured_ids) * (8 if parts == 8 else 16))
    malformed['mapping-width-dictionary-width-mismatch'] = bad
    too_many = 8181
    bad = bytearray(16384)
    bad[:24] = b'R18D' + struct.pack('>HH4I', too_many, 128, 1, too_many * 16, 16, 16384)
    malformed['16-byte-header-ID-overflow'] = bad + bytes((too_many * 16 + 31) // 32 * 32 + 16)
    bad = bytearray(data)
    struct.pack_into('>I', bad, 8, 65537)
    struct.pack_into('>I', bad, 16, 65537 * width)
    malformed['BE16-index-range-overflow'] = bad
    raw_header = bytearray(16384)
    raw_header[:8] = b'R18H' + struct.pack('>HH', len(measured_ids), 128)
    for i, tile in enumerate(measured_ids):
        struct.pack_into('>H', raw_header, 8 + i * 2, tile)
    malformed['raw-over-capacity'] = bytes(raw_header) + b''.join(source[t * 128:(t + 1) * 128] for t in measured_ids)
    for name, bad in malformed.items():
        path = temp / (name + '.bin')
        path.write_bytes(bad)
        check(name, path, reject=True)
text = json.dumps(report, indent=2) + '\n'
if a.output:
    a.output.write_text(text)
print(text, end='')
