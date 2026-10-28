"""Profile the persistent metasprite cache over an observed HOST combat pose stream.

The trace contains dispatch poses but not final camera coordinates, so all
draws use the same central coordinates. ROM read calls here are metadata
accesses, not CD reads or measured Saturn frame time.
"""
import argparse
import csv
import ctypes
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--trace', type=Path, required=True)
parser.add_argument('--snapshot', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
parser.add_argument('--limit', type=int)
parser.add_argument('--pose-cache-entries', type=int, choices=(16, 32, 64, 128), default=32,
                    help='Compile a temporary pose-offset cache-size variant; production remains unchanged')
parser.add_argument('--body-cache-entries', type=int, choices=(16, 32, 64, 128), default=32,
                    help='Compile a temporary decoded-body cache-size variant; production remains unchanged')
parser.add_argument('--require-cached-reads-max', type=int,
                    help='Fail if the persistent cache exceeds this number of ROM metadata callbacks')
args = parser.parse_args()
rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
snapshot = args.snapshot.read_bytes()
assert len(snapshot) >= 56 + 8192
ram = snapshot[56:56 + 8192]
sets = [int.from_bytes(ram[0x5b0 + i * 0x10c + 2:0x5b0 + i * 0x10c + 4], 'big')
        for i in range(9)]

READ = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32,
                        ctypes.c_void_p, ctypes.c_size_t)
class Record(ctypes.Structure):
    _fields_ = [('bytes', ctypes.c_uint8 * 16)]
class BodyEntry(ctypes.Structure):
    _fields_ = [('address', ctypes.c_uint32), ('end', ctypes.c_uint32),
                ('header', ctypes.c_uint16), ('skip_prefix', ctypes.c_uint8),
                ('valid', ctypes.c_uint8), ('rows', (ctypes.c_uint8 * 6) * 32)]
class SetEntry(ctypes.Structure):
    _fields_ = [('base', ctypes.c_uint32), ('rel', ctypes.c_uint16),
                ('valid', ctypes.c_uint8), ('reserved', ctypes.c_uint8)]
class PoseEntry(ctypes.Structure):
    _fields_ = [('pose', ctypes.c_uint16), ('offset', ctypes.c_uint16),
                ('set', ctypes.c_uint8), ('valid', ctypes.c_uint8)]
class Cache(ctypes.Structure):
    _fields_ = [('entries', BodyEntry * args.body_cache_entries),
                ('buckets', ctypes.c_uint8 * 64),
                ('links', ctypes.c_uint8 * args.body_cache_entries), ('next', ctypes.c_uint8),
                ('wrestler_sets', SetEntry * 12),
                ('pose_offsets', PoseEntry * args.pose_cache_entries),
                ('word_tag', ctypes.c_uint32 * 64),
                ('word_data', ctypes.c_uint16 * 64)]
class Ctx(ctypes.Structure):
    _fields_ = [('read', READ), ('user', ctypes.c_void_p),
                ('palette', ctypes.POINTER(ctypes.c_uint8)),
                ('records', ctypes.POINTER(Record)), ('capacity', ctypes.c_size_t),
                ('count', ctypes.c_size_t), ('d6', ctypes.c_uint32),
                ('d7', ctypes.c_uint32), ('cache', ctypes.POINTER(Cache))]

source = ctypes.create_string_buffer(rom)
read_calls = 0
cached_mode = False
cached_sites = {}
@READ
def read(_user, address, dst, n):
    global read_calls
    read_calls += 1
    if cached_mode:
        key = (address, n)
        cached_sites[key] = cached_sites.get(key, 0) + 1
    if address + n > len(rom):
        return 1
    ctypes.memmove(dst, ctypes.addressof(source) + address, n)
    return 0

draws = []
with args.trace.open(newline='') as stream:
    for row in csv.DictReader(stream):
        if row['kind'] != 'dispatch' or row['status'] != '0':
            continue
        slot = int(row['slot'])
        state = int(row['after_state'], 16)
        pose = int(row['after_pose'], 16)
        if state & 255 == 255 or pose & 0x7fff == 0x7fff:
            continue
        link = int(row['link'], 16) & 0xffffff
        partner = (link - 0x1c05b0) // 0x10c if 0x1c05b0 <= link < 0x1c05b0 + 9 * 0x10c \
            and (link - 0x1c05b0) % 0x10c == 0 else None
        draws.append((slot, sets[slot], pose, sets[partner] if partner is not None else 0xffff))
        if args.limit and len(draws) >= args.limit:
            break

palette = (ctypes.c_uint8 * 256)(*(i & 15 for i in range(256)))
baseline_calls = cached_calls = blocked = 0
with tempfile.TemporaryDirectory(prefix='wf-meta-live-', ignore_cleanup_errors=os.name == 'nt') as temp:
    frozen = Path(temp)
    (frozen / 'src').mkdir()
    shutil.copytree(ROOT / 'include', frozen / 'include')
    source_path = frozen / 'src/wf_r18_metasprite.c'
    shutil.copy2(ROOT / 'src/wf_r18_metasprite.c', source_path)
    header_path = frozen / 'include/wf_r18_metasprite.h'
    header = header_path.read_text()
    old = '#define WF_R18_METASPRITE_POSE_CACHE_ENTRIES 32u'
    assert header.count(old) == 1
    header_path.write_text(header.replace(old,
        f'#define WF_R18_METASPRITE_POSE_CACHE_ENTRIES {args.pose_cache_entries}u'))
    header = header_path.read_text()
    old = '#define WF_R18_METASPRITE_CACHE_ENTRIES 32u'
    assert header.count(old) == 1
    header_path.write_text(header.replace(old,
        f'#define WF_R18_METASPRITE_CACHE_ENTRIES {args.body_cache_entries}u'))
    lib = frozen / 'meta.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O2', '-UNDEBUG',
                    '-DWF_R18_PROFILE_START', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(frozen / 'include'), str(source_path),
                    '-o', str(lib)], check=True)
    native = ctypes.CDLL(str(lib))
    native.wf_r18_metasprite_emit.argtypes = [ctypes.POINTER(Ctx)] + [ctypes.c_uint16] * 5
    native.wf_r18_metasprite_cache_reset.argtypes = [ctypes.POINTER(Cache)]
    cache = Cache()
    native.wf_r18_metasprite_cache_reset(ctypes.byref(cache))
    if args.body_cache_entries == 32:
        assert ctypes.sizeof(cache) == 7300 + (args.pose_cache_entries - 32) * ctypes.sizeof(PoseEntry)
    for slot, set_id, pose, partner_set in draws:
        output = []
        for cached in (False, True):
            records = (Record * 512)()
            ctx = Ctx(read, None, palette, records, 512, 0, 0x12345678, 0x9abcdef0,
                      ctypes.pointer(cache) if cached else None)
            read_calls = 0
            cached_mode = cached
            status = native.wf_r18_metasprite_emit(ctypes.byref(ctx), set_id, pose,
                                                   160, 120, partner_set)
            cached_mode = False
            if cached:
                cached_calls += read_calls
            else:
                baseline_calls += read_calls
            output.append((status, ctx.d6, ctx.d7, ctx.count,
                           bytes(records)[:ctx.count * 16]))
        assert output[0] == output[1], (slot, set_id, pose, partner_set)
        if output[0][0]:
            blocked += 1
    hits = ctypes.c_uint32.in_dll(native, 'wf_r18_meta_cache_hits').value
    misses = ctypes.c_uint32.in_dll(native, 'wf_r18_meta_cache_misses').value
    if os.name == 'nt':
        assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
report = {
    'status': 'PASS' if not blocked else 'BLOCKED_POSES',
    'draws': len(draws), 'blocked_poses': blocked,
    'baseline_rom_read_calls': baseline_calls,
    'cached_rom_read_calls': cached_calls,
    'saved_rom_read_calls': baseline_calls - cached_calls,
    'body_cache_hits': hits, 'body_cache_misses': misses,
    'cached_read_hotspots': [
        {'address': f'{address:06X}', 'bytes': n, 'calls': count}
        for (address, n), count in sorted(cached_sites.items(),
                                           key=lambda item: (-item[1], item[0]))[:24]
    ],
    'cache_bytes': ctypes.sizeof(Cache),
    'pose_cache_entries': args.pose_cache_entries,
    'production_cache_entries': 32,
    'body_cache_entries': args.body_cache_entries,
    'word_cache_entries': 64,
    'rom_sha1': hashlib.sha1(rom).hexdigest(),
    'metasprite_source_sha256': hashlib.sha256((ROOT / 'src/wf_r18_metasprite.c').read_bytes()).hexdigest(),
    'trace_sha256': hashlib.sha256(args.trace.read_bytes()).hexdigest(),
    'snapshot_sha256': hashlib.sha256(snapshot).hexdigest(),
    'verifier_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    'limits': ['HOST observed dispatch pose order; central coordinates supplied',
               'Pre-dispatch partner link may differ from the presentation-time link',
               'ROM metadata callback calls, not CD reads, SH-2 cycles or Saturn FPS'],
}
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
if args.require_cached_reads_max is not None:
    assert cached_calls <= args.require_cached_reads_max, (cached_calls, args.require_cached_reads_max)
