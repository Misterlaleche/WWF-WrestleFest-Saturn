"""Offline audio oracle for the fixtures in test_actions_audio.c.

Runs the intact 68000 action dispatcher and observes the real $2052 bus
write at $206C. No sound routine, helper, RNG or action callback is stubbed.
The expected command stream comes from the native test, not a second table
of action sound rules. machine68k is a host test dependency only.
"""
from dataclasses import dataclass, field
from pathlib import Path
import argparse
import hashlib
import json
import re

ACTOR_BASE = 0x1C05B0
ACTOR_STRIDE = 0x10C
ACTOR_SLOTS = 9
RETURN_PC = 0x70000


@dataclass
class ActionFixture:
    selector: int
    frame: int
    timer: int
    flags: int
    contact: int
    credited: int = 1
    mode: int = 1
    dip: int = 0
    rng_seed: int = 1085
    frame_counter: int = 0
    # Exact extra bus writes: (address, width in bytes, value).
    patches: tuple = field(default_factory=tuple)
    d: tuple = field(default_factory=lambda: tuple(0x12340000 + i for i in range(8)))


class ActionAudioOracle:
    def __init__(self, rom):
        import machine68k as mus
        self.mus = mus
        self.machine = mus.Machine(mus.CPUType.M68000, 2048)
        self.cpu, self.mem = self.machine.cpu, self.machine.mem
        if len(rom) != 0x80000:
            raise ValueError("ROM68K.BIN must be the complete 512 KiB original ROM")
        self.rom = bytes(rom)

    def load(self, fixture):
        mem, cpu = self.mem, self.cpu
        # Reapply ROM as well: a test fixture must never inherit prior writes.
        mem.w_block(0, self.rom)
        for address, size in ((0x1C0000, 0x4000), (0x80000, 0x10000),
                              (0xC0000, 0x4000), (0x180000, 0x9000),
                              (0x100000, 0x20), (0x140000, 0x40)):
            mem.w_block(address, bytes(size))
        cpu.w_sr(0x2700)
        for i in range(15):
            cpu.w_reg(i, fixture.d[i] if i < 8 else 0)
        cpu.w_sp(0x1C3FF0)
        mem.w32(0x1C3FF0, RETURN_PC)
        cpu.w_reg(self.mus.Register.A0, ACTOR_BASE)
        mem.w16(0x1C007C, fixture.credited)
        mem.w8(0x1C0161, fixture.mode)
        mem.w16(0x1C0066, fixture.dip)
        mem.w32(0x1C005C, fixture.rng_seed)
        mem.w32(0x1C0080, fixture.frame_counter)
        # Same zeroed actors, pointers and fields as the native C setup().
        for i in range(ACTOR_SLOTS):
            actor = ACTOR_BASE + i * ACTOR_STRIDE
            mem.w8(actor, 0x80)
            for offset, value in ((0x20, 0), (6, 0x280), (10, 0x160),
                                  (14, 0x140), (0x66, 50), (0x72, 100)):
                mem.w16(actor + offset, value)
            for offset, slot in ((0x26, (i + 1) % ACTOR_SLOTS),
                                 (0x86, (i + 2) % ACTOR_SLOTS),
                                 (0x7A, (i + 1) % ACTOR_SLOTS), (0x92, 1)):
                mem.w32(actor + offset, ACTOR_BASE + slot * ACTOR_STRIDE)
        for offset, value in ((0x20, 5), (0x1C, 0x8005),
                              (0x60, fixture.selector), (0x24, fixture.frame),
                              (0x22, fixture.timer), (0x36, fixture.contact)):
            mem.w16(ACTOR_BASE + offset, value)
        mem.w8(ACTOR_BASE + 0x33, fixture.flags)
        for address, width, value in fixture.patches:
            if width not in (1, 2, 4):
                raise ValueError("fixture patch width must be 1, 2 or 4")
            getattr(mem, f"w{width * 8}")(address, value)
        cpu.w_pc(0x1C03E)

    def run(self, fixture, instruction_limit=100000):
        self.load(fixture)
        commands = []
        for instruction in range(instruction_limit):
            pc = self.cpu.r_pc()
            if pc == RETURN_PC:
                return commands
            if pc == 0x206C:
                commands.append(self.cpu.r_reg(self.mus.Register.D0) & 0xFFFF)
            self.cpu.execute(1)
        raise AssertionError(("original action did not return", fixture,
                              hex(self.cpu.r_pc()), commands))


def parse_native_trace(trace):
    """Read the simple or patched fixture emitted by test_actions_audio.c."""
    parts = trace.split()
    if len(parts) < 7 or parts[0] != "TRACE":
        raise ValueError(f"invalid native action trace: {trace}")
    values = [int(value, 0) for value in parts[2:]]
    if parts[1] == "action":
        return ActionFixture(*values[:5]), values[5:]
    if parts[1] != "patch" or len(values) < 9:
        raise ValueError(f"invalid native action trace: {trace}")
    patch_count = values[8]
    end = 9 + patch_count * 3
    if patch_count < 0 or len(values) < end:
        raise ValueError(f"incomplete native fixture patches: {trace}")
    patches = tuple(tuple(values[i:i + 3]) for i in range(9, end, 3))
    return ActionFixture(*values[:8], patches=patches), values[end:]


def verify_native_traces(rom, traces):
    """Compare TRACE action/patch records with unchanged 68000 instructions."""
    oracle = ActionAudioOracle(rom)
    count = 0
    command_count = 0
    audible_fixtures = 0
    selectors, modes, credits, dips, commands = set(), set(), set(), set(), set()
    stream = hashlib.sha256()
    for trace in traces:
        fixture, expected = parse_native_trace(trace)
        observed = oracle.run(fixture)
        if observed != expected:
            raise AssertionError(("native/original ROM action audio mismatch",
                                  fixture, "native", expected, "ROM", observed))
        count += 1
        command_count += len(expected)
        audible_fixtures += bool(expected)
        selectors.add(fixture.selector & 0xFF)
        modes.add(fixture.mode)
        credits.add(fixture.credited)
        dips.add(fixture.dip)
        commands.update(expected)
        stream.update((trace.rstrip() + "\n").encode())
    if not count:
        raise AssertionError("no native action traces supplied")
    print(f"PASS: {count} action traces match original 68000 sound writes at $206C")
    return {
        "fixture_count": count,
        "fixtures_with_audio": audible_fixtures,
        "command_count": command_count,
        "selector_count": len(selectors),
        "selectors": [f"{value:02X}" for value in sorted(selectors)],
        "mode_bytes": sorted(modes),
        "credited_words": sorted(credits),
        "dip_words": sorted(dips),
        "command_words": [f"{value:04X}" for value in sorted(commands)],
        "native_trace_sha256": stream.hexdigest(),
        "rom_sha256": hashlib.sha256(rom).hexdigest(),
        "scope": "Audio command sequences from unchanged 68000 instructions; no claim of full gameplay/RNG equivalence or physical Saturn validation",
    }


def verify_c_expectations(rom, source):
    """Check literal expect() fixtures before the native runner emits traces."""
    oracle = ActionAudioOracle(rom)
    number = r"(?:0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*"
    cases = re.findall(r"\bexpect\(\s*(" + number + r"(?:\s*,\s*" + number + r"){7})\s*\);", source)
    failures = []
    for case in cases:
        values = [int(re.sub(r"[uUlL]+$", "", value.strip()), 0)
                  for value in case.split(",")]
        fixture = ActionFixture(*values[:5])
        expected = values[6:6 + values[5]]
        observed = oracle.run(fixture)
        if expected != observed:
            failures.append((fixture, expected, observed))
    if not cases:
        raise AssertionError("no literal expect() cases found")
    if failures:
        for fixture, expected, observed in failures:
            print("FAIL", fixture, "C expected", expected, "ROM", observed)
        raise AssertionError(f"{len(failures)} of {len(cases)} action expectations disagree with ROM")
    print(f"PASS: {len(cases)} literal action expectations match original 68000 writes at $206C")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=root / "cd/ROM68K.BIN")
    parser.add_argument("--source", type=Path, default=root / "tests/test_actions_audio.c")
    parser.add_argument("--traces", type=Path, help="native test output produced with --traces")
    parser.add_argument("--summary", type=Path, help="write compact JSON results with --traces")
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    if args.traces:
        lines = args.traces.read_text().splitlines()
        result = verify_native_traces(rom, [line for line in lines if line.startswith("TRACE ")])
        for line in lines:
            match = re.fullmatch(r"PASS native action matrix: (\d+) fixtures, (\d+) existing blocked dependencies", line)
            if match:
                result["native_matrix_fixtures"] = int(match[1])
                result["native_matrix_blocked"] = int(match[2])
        if args.summary:
            args.summary.write_text(json.dumps(result, indent=2) + "\n")
    else:
        if args.summary:
            parser.error("--summary requires --traces")
        verify_c_expectations(rom, args.source.read_text())
