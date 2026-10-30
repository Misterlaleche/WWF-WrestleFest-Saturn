#!/usr/bin/env python3
"""Read-only native SH2 counters inside standalone Mednafen on Linux.

The launcher should be Mednafen's parent process, so normal Linux /proc access
rules suffice. This module opens /proc/PID/mem with O_RDONLY and never uses
ptrace, debugger commands, process-memory writes, breakpoints or save states.

HWRAM is identified by exact ELF-derived program instruction bytes, following
the supplied observe-mednafen-runtime18.py. Mednafen stores Saturn WRAM as
native uint16 words: on this little-endian host adjacent bytes are swapped.

Samples are asynchronous observations. Boundary guards detect a counter change
during a sample; an unchanged guard does NOT make actor records an atomic CPU
snapshot. Host sample time and native VBlank/game counters are kept separate.
"""

from __future__ import annotations

import hashlib
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time


HWRAM_ADDRESS = 0x06000000
HWRAM_BYTES = 0x00100000
PROGRAM_ADDRESS = 0x06004000
SIGNATURE_BYTES = 4096
DEFAULT_NAMES = (
    'wp1980_started', 'wp1980_draws', 'wp1980_vblanks', 'wp1980_max_gap',
    'wp1980_demo_active', 'video_clock', 'fatal_owner', 'fatal_dependency',
    'fatal_frame', 'finished_frame', 'finished_result', 'finished_count',
    'finished_timer', 'finished_presentation', 'finished_vblanks',
    'finished_presentation_vblanks', 'window_misses', 'window_sectors',
    'wp1979_cd_init_rc', 'wp1979_cdfs_stage', 'wp1979_cdfs_lba',
    'wp1979_cdfs_rc', 'boot_seed_observed', 'wp1980_cart_id',
    'wp1980_cart_bytes', 'hot_raw_data', 'hot_count', 'wp1980_vram_hits',
    'wp1980_vram_misses', 'wp1980_vram_evictions',
    'wp1980_sprite_block_decodes', 'wp1980_sprite_block_hits',
    'wp1980_sprite_warm_tiles', 'wp1980_sprite_warm_vblanks',
)
GUARD_NAMES = ('video_clock', 'wp1980_started', 'wp1980_draws', 'game_frame', 'frame_ready', 'frame_stage')


class ObserverError(RuntimeError):
    """Observation cannot safely continue."""


class NotReady(ObserverError):
    """The exact executable has not yet appeared in a unique HWRAM mapping."""


def sha256_file(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as source:
        for data in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(data)
    return digest.hexdigest()


def word_swap(data):
    """Return Saturn-order bytes from a complete even-length host-word span."""
    if len(data) & 1:
        raise ValueError('word_swap requires an even byte count')
    swapped = bytearray(len(data))
    swapped[0::2] = data[1::2]
    swapped[1::2] = data[0::2]
    return bytes(swapped)


def find_tool(name, sdk=None):
    if sdk:
        candidate = Path(sdk) / 'bin' / ('sh2eb-elf-' + name)
        if candidate.is_file():
            return str(candidate)
    candidate = shutil.which('sh2eb-elf-' + name)
    if candidate:
        return candidate
    raise ValueError('Supply the existing SH2 SDK directory')


def load_symbols(elf, sdk=None):
    """Resolve exact ELF symbols, including the aliases used by the old harness."""
    symbols = {}
    text = subprocess.check_output([find_tool('nm', sdk), '-n', str(elf)], text=True)
    for line in text.splitlines():
        fields = line.split()
        if len(fields) != 3 or not re.fullmatch(r'[0-9a-fA-F]+', fields[0]):
            continue
        address, name = int(fields[0], 16), fields[2]
        symbols[name] = address
        symbols.setdefault(name.lstrip('_').split('.lto_priv')[0], address)
    return symbols


def resolve_watch(spec, symbols):
    """NAME:SYMBOL+OFFSET:SIZE; a bare symbol is a four-byte unsigned watch."""
    parts = spec.split(':')
    if len(parts) == 1:
        name, expression, size_text = parts[0], parts[0], '4'
    elif len(parts) in (2, 3):
        name, expression = parts[:2]
        size_text = parts[2] if len(parts) == 3 else '4'
    else:
        raise ValueError('Invalid watch: ' + spec)
    if not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]{0,94}', name):
        raise ValueError('Invalid watch name: ' + name)
    size = int(size_text, 0)
    if not 1 <= size <= 65536:
        raise ValueError('Watch size must be 1..65536 bytes')
    match = re.fullmatch(r'([^+\s]+)(?:\+(0[xX][0-9a-fA-F]+|[0-9]+))?', expression)
    if not match:
        raise ValueError('Invalid watch address: ' + expression)
    base, addition = match.groups()
    try:
        address = int(base, 0)
    except ValueError:
        if base not in symbols:
            raise ValueError('Unknown ELF symbol: ' + base) from None
        address = symbols[base]
    address += int(addition, 0) if addition else 0
    if address < 0 or address + size > 0x100000000:
        raise ValueError('Watch overflows the SH2 address space')
    address &= 0x1fffffff
    if address < HWRAM_ADDRESS or address + size > HWRAM_ADDRESS + HWRAM_BYTES:
        raise ValueError('This observer supports HWRAM watches only: ' + spec)
    return dict(name=name, address=address, size=size)


def build_watches(symbols, actors_layout=None, extra_specs=(), defaults=True):
    """Match the Beetle watch format, with all nine complete actor records."""
    watches = {}
    if defaults:
        for name in DEFAULT_NAMES:
            if name in symbols:
                watches[name] = resolve_watch(name, symbols)
        if 'rt' in symbols:
            watches['game_frame'] = resolve_watch('game_frame:rt+24:4', symbols)
        if 'wp1980_start_clock' in symbols:
            watches['start_clock'] = resolve_watch('start_clock:wp1980_start_clock:4', symbols)
    if actors_layout:
        data = Path(actors_layout).read_bytes()
        if len(data) < 20 or 'rt' not in symbols:
            raise ValueError('actors-layout requires five SH2 uint32 values and rt')
        offset, stride, rng, _registers, _inputs = struct.unpack('>5I', data[:20])
        if stride != 268:
            raise ValueError('Actor stride differs from the audited 0x10c ABI')
        for slot in range(9):
            name = 'actor' + str(slot)
            watches[name] = resolve_watch(f'{name}:rt+{offset + slot * stride}:{stride}', symbols)
        watches['rng_state'] = resolve_watch(f'rng_state:rt+{rng}:4', symbols)
    for spec in extra_specs:
        watch = resolve_watch(spec, symbols)
        watches[watch['name']] = watch
    if len(watches) > 256:
        raise ValueError('At most256 watches are supported')
    return list(watches.values())


def _process_start(pid):
    # The process name inside (...) can itself contain spaces and parentheses.
    fields = Path(f'/proc/{pid}/stat').read_text().rpartition(')')[2].split()
    if len(fields) <= 19:
        raise ObserverError('Malformed process stat')
    return int(fields[19])


def _maps(pid):
    result = []
    for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
        fields = line.split(None, 5)
        start, end = (int(value, 16) for value in fields[0].split('-'))
        result.append(dict(start=start, end=end, permissions=fields[1],
                           path=fields[5] if len(fields) > 5 else '', line=line))
    return result


class ReadOnlyObserver:
    """Persistent O_RDONLY /proc handle; no simulator-side code or hooks."""

    def __init__(self, pid, program_bytes, *, proc_pid=None, load_address=PROGRAM_ADDRESS):
        self.pid = int(pid)
        self.proc_pid = int(proc_pid) if proc_pid is not None else self.pid
        self.program = bytes(program_bytes)
        self.load_address = int(load_address) & 0x1fffffff
        if len(self.program) < SIGNATURE_BYTES * 2:
            raise ValueError('Need at least8192 exact program bytes to identify HWRAM')
        if self.load_address != PROGRAM_ADDRESS:
            raise ValueError('This build contract loads the SH2 program at0x06004000')
        # The sandbox's /proc can use host IDs while Popen uses namespace IDs.
        # Only accept the launched direct child, as identified by its own
        # pre-exec launch_identity.py record; never search other process RAM.
        own_proc_pid = int(Path('/proc/self/stat').read_text().split(' ', 1)[0])
        status = {}
        for line in Path(f'/proc/{self.proc_pid}/status').read_text().splitlines():
            key, separator, value = line.partition(':')
            if separator:
                status[key] = value.strip()
        namespace_ids = [int(value) for value in status.get('NSpid', str(self.proc_pid)).split()]
        if int(status.get('PPid', '-1')) != own_proc_pid or namespace_ids[-1] != self.pid:
            raise ObserverError('Process is not the launched child; pass proc_pid from its own launch_identity.py record')
        self.start_ticks = _process_start(self.proc_pid)
        self.exe = os.readlink(f'/proc/{self.proc_pid}/exe')
        self.exe_sha256 = sha256_file(f'/proc/{self.proc_pid}/exe')
        self.cmdline = Path(f'/proc/{self.proc_pid}/cmdline').read_bytes().rstrip(b'\0').split(b'\0')
        self.fd = os.open(f'/proc/{self.proc_pid}/mem', os.O_RDONLY | os.O_CLOEXEC)
        self.hwram_host = None
        self.mapping = None
        self.opened_host_monotonic_ns = time.monotonic_ns()
        self.stats = dict(samples=0, boundary_unstable_samples=0, pread_calls=0,
                          bytes_read=0, observed_clock_changes=0,
                          skipped_clock_values=0, backwards_clock_observations=0)
        self.last_clock = None

    @property
    def located(self):
        return self.hwram_host is not None

    def close(self):
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None

    def __enter__(self):
        return self

    def __exit__(self, *_args):
        self.close()

    def _check_process(self):
        if self.fd is None:
            raise ObserverError('Observer is closed')
        if _process_start(self.proc_pid) != self.start_ticks:
            raise ObserverError('PID identity changed')

    def _pread(self, address, size):
        data = os.pread(self.fd, size, address)
        self.stats['pread_calls'] += 1
        self.stats['bytes_read'] += len(data)
        if len(data) != size:
            raise ObserverError(f'Short process-memory read: {len(data)} of {size}')
        return data

    def locate(self):
        """Find one exact HWRAM image; retry NotReady during ordinary boot."""
        self._check_process()
        if self.located:
            self.verify_signature()
            return self.identity()
        signature = word_swap(self.program[:SIGNATURE_BYTES])
        verification = word_swap(self.program[SIGNATURE_BYTES:SIGNATURE_BYTES * 2])
        candidates = []
        for region in _maps(self.proc_pid):
            if not region['permissions'].startswith('rw'):
                continue
            if region['path'] not in ('', '[heap]'):
                continue
            length = region['end'] - region['start']
            if length > 128 * 1024 * 1024 or length < HWRAM_BYTES:
                continue
            try:
                data = self._pread(region['start'], length)
            except (OSError, ObserverError):
                continue
            at = data.find(signature)
            while at >= 0:
                base = region['start'] + at - (self.load_address - HWRAM_ADDRESS)
                verify_at = at + SIGNATURE_BYTES
                if (base >= region['start'] and base + HWRAM_BYTES <= region['end'] and
                        data[verify_at:verify_at + SIGNATURE_BYTES] == verification):
                    candidates.append((base, region))
                at = data.find(signature, at + 2)
        if not candidates:
            raise NotReady('The exact loaded SH2 HWRAM image is not visible yet')
        if len(candidates) != 1:
            raise ObserverError(f'Ambiguous exact HWRAM images: {len(candidates)}')
        self.hwram_host, self.mapping = candidates[0]
        return self.identity()

    def read_guest(self, address, size):
        """Read arbitrary byte alignment, returning original Saturn byte order."""
        if not self.located:
            raise NotReady('Call locate() before reading watches')
        address = int(address) & 0x1fffffff
        size = int(size)
        offset = address - HWRAM_ADDRESS
        if size < 0 or offset < 0 or offset + size > HWRAM_BYTES:
            raise ValueError('Read is outside the located HWRAM')
        if not size:
            return b''
        begin, end = offset & ~1, (offset + size + 1) & ~1
        raw = self._pread(self.hwram_host + begin, end - begin)
        return word_swap(raw)[offset - begin:offset - begin + size]

    def verify_signature(self):
        self._check_process()
        expected = self.program[:SIGNATURE_BYTES * 2]
        if self.read_guest(self.load_address, len(expected)) != expected:
            raise ObserverError('Loaded instruction signature changed')
        return True

    def identity(self):
        return dict(pid=self.pid, proc_pid=self.proc_pid, process_start_ticks=self.start_ticks,
                    executable=self.exe, executable_sha256=self.exe_sha256,
                    cmdline=[part.decode('utf-8', errors='replace') for part in self.cmdline],
                    program_sha256=hashlib.sha256(self.program).hexdigest(),
                    hwram_guest=HWRAM_ADDRESS, hwram_bytes=HWRAM_BYTES,
                    hwram_host=self.hwram_host, mapping=self.mapping,
                    instruction_match_bytes=SIGNATURE_BYTES * 2,
                    instruction_guest_address=self.load_address,
                    host_byte_order='little-endian uint16; adjacent bytes swapped',
                    process_memory_access='O_RDONLY',
                    opened_host_monotonic_ns=self.opened_host_monotonic_ns,
                    sample_limit='asynchronous reads; boundary guards are not an atomic CPU snapshot')

    def _read_watches(self, watches):
        # Merge nearby watches to reduce /proc syscalls without copying all1MiB.
        groups = []
        for watch in sorted(watches, key=lambda item: item['address']):
            begin, end = watch['address'], watch['address'] + watch['size']
            if groups and begin <= groups[-1][1] + 4096:
                groups[-1][1] = max(groups[-1][1], end)
                groups[-1][2].append(watch)
            else:
                groups.append([begin, end, [watch]])
        values = {}
        for begin, end, items in groups:
            block = self.read_guest(begin, end - begin)
            for watch in items:
                data = block[watch['address'] - begin:watch['address'] - begin + watch['size']]
                values[watch['name']] = int.from_bytes(data, 'big') if len(data) <= 8 else data.hex()
        return values

    def sample(self, watches):
        """Return counters and time/guard metadata; makes no every-field promise."""
        self._check_process()
        before_ns = time.monotonic_ns()
        guards = [watch for watch in watches if watch['name'] in GUARD_NAMES]
        before = self._read_watches(guards)
        values = self._read_watches(watches)
        after = self._read_watches(guards)
        after_ns = time.monotonic_ns()
        stable = bool(guards) and before == after and all(values[name] == value for name, value in before.items())
        self.stats['samples'] += 1
        if not stable:
            self.stats['boundary_unstable_samples'] += 1
        clock = after.get('video_clock')
        if clock is not None:
            if self.last_clock is not None and clock != self.last_clock:
                delta = (clock - self.last_clock) & 0xffffffff
                if delta < 0x80000000:
                    self.stats['observed_clock_changes'] += 1
                    self.stats['skipped_clock_values'] += max(0, delta - 1)
                else:
                    self.stats['backwards_clock_observations'] += 1
            self.last_clock = clock
        values.update(host_monotonic_ns=after_ns,
                      host_elapsed_seconds=(after_ns - self.opened_host_monotonic_ns) / 1e9,
                      sample_begin_ns=before_ns, sample_end_ns=after_ns,
                      sample_duration_ns=after_ns - before_ns,
                      stable_video_clock_before=before.get('video_clock'),
                      stable_video_clock_after=after.get('video_clock'),
                      boundary_stable=stable, boundary_before=before, boundary_after=after)
        return values

    def dump_hwram(self, directory):
        """Save a live read, explicitly not a save state or atomic memory image."""
        self._check_process()
        self.verify_signature()
        start_ns = time.monotonic_ns()
        raw = self._pread(self.hwram_host, HWRAM_BYTES)
        end_ns = time.monotonic_ns()
        directory = Path(directory)
        directory.mkdir(parents=True, exist_ok=True)
        host_path = directory / 'memory_06000000_host.bin'
        guest_path = directory / 'memory_06000000_saturn.bin'
        host_path.write_bytes(raw)
        guest_path.write_bytes(word_swap(raw))
        return dict(host_path=str(host_path), saturn_path=str(guest_path),
                    host_sha256=sha256_file(host_path), saturn_sha256=sha256_file(guest_path),
                    host_begin_ns=start_ns, host_end_ns=end_ns,
                    atomic=False, scope='Read-only live HWRAM dump; not a save state')
