#!/usr/bin/env python3
"""Measure original OKI address coverage and independent-block storage.

This is a host-only feasibility audit, not a proposed native pack ABI or a
decoder implementation. ROM-derived maps/compressed bytes stay in a new private
output directory. Every represented byte and every compressed block is verified.
"""
import argparse
from bisect import bisect_right
import ctypes
import ctypes.util
import hashlib
import json
from pathlib import Path
import struct
import zipfile
import zlib

PACKAGE = Path(__file__).resolve().parents[2]
ROM_SHA = 'e4cbe71f079e2cb95b3463115557fa1d99e18ea975f2d3354827b01994fc4615'
CAPACITY = 0x6c000


def sha(data):
    return hashlib.sha256(data).hexdigest()


def runs(mask):
    result = []
    begin = None
    for address, present in enumerate(mask + b'\0'):
        if present and begin is None:
            begin = address
        elif not present and begin is not None:
            result.append((begin, address))
            begin = None
    return result


class Lz4:
    def __init__(self):
        name = ctypes.util.find_library('lz4')
        if not name:
            raise RuntimeError('A host liblz4 is required for the requested measured comparison')
        self.lib = ctypes.CDLL(name)
        self.lib.LZ4_versionString.restype = ctypes.c_char_p
        self.lib.LZ4_compressBound.argtypes = [ctypes.c_int]
        self.lib.LZ4_compressBound.restype = ctypes.c_int
        self.lib.LZ4_compress_default.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        self.lib.LZ4_compress_default.restype = ctypes.c_int
        self.lib.LZ4_decompress_safe.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        self.lib.LZ4_decompress_safe.restype = ctypes.c_int
        self.version = self.lib.LZ4_versionString().decode()

    def compress(self, block):
        output = ctypes.create_string_buffer(self.lib.LZ4_compressBound(len(block)))
        count = self.lib.LZ4_compress_default(block, output, len(block), len(output))
        if count <= 0:
            raise RuntimeError('LZ4 compression failed')
        return output.raw[:count]

    def decompress(self, block, length):
        output = ctypes.create_string_buffer(length)
        count = self.lib.LZ4_decompress_safe(block, output, len(block), length)
        if count != length:
            raise ValueError('LZ4 block length mismatch')
        return output.raw


def coverage(rom, pack, out):
    if (pack[:4] != b'WFSX' or struct.unpack_from('>I', pack, 8)[0] != len(pack)
            or struct.unpack_from('>I', pack, 28)[0] != zlib.crc32(pack[32:])):
        raise ValueError('Pack header or CRC mismatch')
    entries, masks = [], [bytearray(0x40000), bytearray(0x40000)]
    valid_by_bank = []
    for bank, expected in ((0, 73), (1, 41)):
        valid = []
        for sample in range(128):
            at = bank * 0x40000 + sample * 8
            start = int.from_bytes(rom[at:at + 3], 'big') & 0x3ffff
            end = int.from_bytes(rom[at + 3:at + 6], 'big') & 0x3ffff
            if start < end:
                valid.append(sample)
        if valid != list(range(1, expected + 1)):
            raise ValueError('Original valid sample table extends beyond the audited pack')
        valid_by_bank.append(len(valid))
    start_table = bytearray()
    for bank, count in ((0, 73), (1, 41)):
        for sample in range(1, count + 1):
            at = bank * 0x40000 + sample * 8
            start = int.from_bytes(rom[at:at + 3], 'big')
            end = int.from_bytes(rom[at + 3:at + 6], 'big') + 1
            offset, length, _ = struct.unpack_from('>IHH', pack, 32 + (bank * 74 + sample) * 8)
            if not (0 <= start < end <= 0x40000 and end - start == length
                    and pack[offset:offset + length] == rom[bank * 0x40000 + start:bank * 0x40000 + end]):
                raise ValueError('Original sample range does not match native pack')
            entries.append(dict(bank=bank, sample=sample, start=start, end=end, offset=offset))
            start_table += start.to_bytes(3, 'big')
            masks[bank][start:end] = b'\1' * length
    required = bytes(bool(a or b) for a, b in zip(*masks))
    extra, extra_records = bytearray(), []
    for bank in (0, 1):
        missing = bytes(want and not have for want, have in zip(required, masks[bank]))
        for start, end in runs(missing):
            offset = len(pack) + len(extra)
            extra += rom[bank * 0x40000 + start:bank * 0x40000 + end]
            extra_records.append(dict(bank=bank, sample=None, start=start, end=end, offset=offset))
    header = struct.pack('>4s4HI', b'WFBA', 1, len(entries), len(extra_records), len(start_table), len(extra))
    payload_displacement = len(header) + len(start_table) + 8 * len(extra_records)
    descriptors = bytearray()
    for entry in extra_records:
        entry['offset'] += payload_displacement
        descriptors += ((entry['bank'] << 18) | entry['start']).to_bytes(3, 'big')
        descriptors += (entry['end'] - entry['start']).to_bytes(2, 'big')
        descriptors += entry['offset'].to_bytes(3, 'big')
    extension = header + start_table + descriptors + extra
    extension += b'\0' * (len(extension) & 1)
    expanded = pack + extension
    # Read the stored three-byte starts and eight-byte descriptors back. The
    # verification below therefore exercises the serialized 704B construction,
    # not an unbounded host dictionary standing in for its memory budget.
    restored = []
    for index, original in enumerate(entries):
        start = int.from_bytes(extension[16 + index * 3:19 + index * 3], 'big')
        at = 32 + (original['bank'] * 74 + original['sample']) * 8
        offset, length, _ = struct.unpack_from('>IHH', pack, at)
        restored.append(dict(bank=original['bank'], sample=original['sample'], start=start, end=start + length, offset=offset))
    for index in range(len(extra_records)):
        at = 16 + len(start_table) + index * 8
        address = int.from_bytes(extension[at:at + 3], 'big')
        length = int.from_bytes(extension[at + 3:at + 5], 'big')
        offset = int.from_bytes(extension[at + 5:at + 8], 'big')
        restored.append(dict(bank=address >> 18, sample=None, start=address & 0x3ffff,
                             end=(address & 0x3ffff) + length, offset=offset))
    if restored != entries + extra_records:
        raise ValueError('Serialized address map differs')
    mapping = [sorted((e for e in restored if e['bank'] == bank), key=lambda e: e['start'])
               for bank in (0, 1)]
    verified = 0
    for bank in (0, 1):
        starts = [e['start'] for e in mapping[bank]]
        for start, end in runs(required):
            for address in range(start, end):
                index = bisect_right(starts, address) - 1
                entry = mapping[bank][index]
                if index < 0 or address >= entry['end']:
                    raise ValueError('Address resolver has a hole')
                actual = expanded[entry['offset'] + address - entry['start']]
                if actual != rom[bank * 0x40000 + address]:
                    raise ValueError('Address resolver differs from original ROM')
                verified += 1
    (out / 'original-starts-3byte.bin').write_bytes(start_table)
    (out / 'missing-original-bytes.bin').write_bytes(extra)
    (out / 'bankmap-extension.bin').write_bytes(extension)
    (out / 'private-address-map.json').write_text(json.dumps(entries + extra_records, indent=2) + '\n')
    # Feasibility representation: original starts augment existing pack entries;
    # each extra range uses bank+18bit start (3B), length (2B), pack offset (3B).
    extension_unaligned = 16 + len(start_table) + 8 * len(extra_records) + len(extra)
    extension_bytes = (extension_unaligned + 1) & ~1
    if len(extension) != extension_bytes:
        raise ValueError('Serialized extension size differs from the byte budget')
    return {
        'pack_bytes': len(pack), 'pack_capacity_bytes': CAPACITY, 'current_pack_free_bytes': CAPACITY - len(pack),
        'original_entries_verified': len(entries), 'sample_table_entries_checked': 256,
        'valid_original_sample_entries_by_bank': valid_by_bank,
        'original_address_bytes_verified': verified,
        'relative_address_union_bytes': sum(required),
        'per_bank_own_address_coverage': [sum(m) for m in masks],
        'per_bank_cross_bank_missing_bytes': [sum(w and not h for w, h in zip(required, m)) for m in masks],
        'missing_range_count': len(extra_records), 'missing_payload_bytes': len(extra),
        'start_table_bytes': len(start_table), 'extra_range_record_bytes': len(extra_records) * 8,
        'feasibility_header_bytes': 16, 'aligned_extension_bytes': extension_bytes,
        'pack_free_after_extension_bytes': CAPACITY - len(pack) - extension_bytes,
        'resolver_equality': True,
        'private_map_sha256': sha((out / 'private-address-map.json').read_bytes()),
        'private_start_table_sha256': sha(start_table), 'private_missing_payload_sha256': sha(extra),
        'private_serialized_extension_sha256': sha(extension), 'serialized_extension_roundtrip': True,
        'scope': 'All byte addresses inside the union of the 114 verified original sample ranges, read under either global bank. No native runtime or arbitrary invalid sample-table entry claim.',
        'format_status': 'Feasibility arithmetic only; existing pack ABI, loader and decoder are unchanged.'}


def compressed_blocks(rom, block_size, algorithm, deduplicate, lz4, out):
    blocks = [rom[at:at + block_size] for at in range(0, len(rom), block_size)]
    if deduplicate:
        unique, ids, indices = [], {}, []
        for block in blocks:
            if block not in ids:
                ids[block] = len(unique)
                unique.append(block)
            indices.append(ids[block])
    else:
        unique, indices = blocks, None
    payload, offsets, compressed_count = bytearray(), [], 0
    for block in unique:
        if algorithm == 'lz4':
            encoded = lz4.compress(block)
            decoded = lz4.decompress(encoded, len(block))
        elif algorithm == 'zlib9':
            encoded = zlib.compress(block, 9)
            decoded = zlib.decompress(encoded)
        else:
            encoder = zlib.compressobj(9, zlib.DEFLATED, -15)
            encoded = encoder.compress(block) + encoder.flush()
            decoded = zlib.decompress(encoded, -15)
        if decoded != block:
            raise ValueError('Compression roundtrip differs')
        raw = len(encoded) >= len(block)
        offsets.append(len(payload) | (0x80000000 if raw else 0))
        payload += block if raw else encoded
        compressed_count += not raw
    offsets.append(len(payload))
    # An explicit audit-only 16B header, uint32 offsets/raw flags, and optional
    # uint16 page dictionary. These bytes are counted, not omitted as overhead.
    header = struct.pack('>4sI4H', b'OKIA', len(rom), block_size, len(blocks), len(unique), int(deduplicate))
    index_bytes = b'' if indices is None else struct.pack('>' + 'H' * len(indices), *indices)
    offset_bytes = struct.pack('>' + 'I' * len(offsets), *offsets)
    encoded_pack = header + index_bytes + offset_bytes + payload
    restored = bytearray()
    for i in range(len(blocks)):
        index = i if indices is None else indices[i]
        start, end = offsets[index] & 0x7fffffff, offsets[index + 1] & 0x7fffffff
        data = bytes(payload[start:end])
        if offsets[index] & 0x80000000:
            result = data
        elif algorithm == 'lz4':
            result = lz4.decompress(data, block_size)
        else:
            result = zlib.decompress(data, 15 if algorithm == 'zlib9' else -15)
        restored += result
    if restored != rom:
        raise ValueError('Entire address-space reconstruction differs')
    filename = f'{algorithm}-{block_size}-' + ('dedup' if deduplicate else 'independent') + '.bin'
    (out / filename).write_bytes(encoded_pack)
    return dict(algorithm=algorithm, block_bytes=block_size, deduplicated=deduplicate,
        logical_blocks=len(blocks), stored_blocks=len(unique), compressed_blocks=compressed_count,
        header_bytes=len(header), dictionary_bytes=len(index_bytes), offset_table_bytes=len(offset_bytes),
        payload_bytes=len(payload), total_bytes=len(encoded_pack), sha256=sha(encoded_pack),
        entire_original_rom_roundtrip=True, four_voice_one_block_cache_bytes=block_size * 4,
        budget_excludes='Native decoder code, decompressor working state, command/event metadata, decode checkpoints and alignment required by a future implementation')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom-zip', required=True, type=Path)
    parser.add_argument('--pack', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    args.out = args.out.resolve()
    if args.out == PACKAGE or PACKAGE in args.out.parents:
        parser.error('Private output must remain outside the source package')
    with zipfile.ZipFile(args.rom_zip) as archive:
        rom = archive.read('31j10.ic73')
    if len(rom) != 524288 or sha(rom) != ROM_SHA:
        raise ValueError('Original sample ROM identity mismatch')
    pack = args.pack.read_bytes()
    args.out.mkdir(parents=True, exist_ok=False)
    lz4 = Lz4()
    source = Path(__file__).read_bytes()
    report = dict(kind='HOST_OKI_STORAGE_FEASIBILITY_ONLY', source_sha256=sha(source),
        original_rom_sha256=sha(rom), original_rom_bytes=len(rom), native_pack_sha256=sha(pack),
        host_lz4_version=lz4.version, host_zlib_version=zlib.ZLIB_RUNTIME_VERSION,
        coverage=coverage(rom, pack, args.out), compression=[], checkpoint_arithmetic=[])
    for size in (256, 512, 1024):
        for algorithm in ('lz4', 'zlib9', 'rawdeflate9'):
            for deduplicate in (False, True):
                report['compression'].append(compressed_blocks(rom, size, algorithm, deduplicate, lz4, args.out))
    for stride in (32, 64, 128, 256, 512):
        count = 4 * (4096 // stride + 1)
        report['checkpoint_arithmetic'].append(dict(stride_pcm_samples=stride, slots_four_voices=count,
            packed_signal_index_bytes=count * 3, aligned_signal_index_tag_bytes=count * 4,
            current_full_decoder_struct_bytes=count * 14, maximum_redecode_to_boundary_per_voice=stride - 1))
    report.update(source_stable=Path(__file__).read_bytes() == source,
        native_engine_changed=False, native_decompression_or_rollback_timing_measured=False,
        checkpoint_scope='Only storage arithmetic. Sparse checkpoints require bank-boundary/history handling and a validated playback cursor; they do not repair prefetched PCM by themselves.',
        compression_scope='Lossless host measurements, including independent block offsets and raw fallback. Deduplicated variants also count uint16 logical-block indices. Deflate is a conceptual storage comparison, not a validated 68000 decoder.')
    (args.out / 'oki-storage-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'report': str(args.out / 'oki-storage-audit.json'),
        'original_address_bytes_verified': report['coverage']['original_address_bytes_verified'],
        'compression_roundtrips': len(report['compression']), 'source_stable': report['source_stable']}))


if __name__ == '__main__':
    main()
