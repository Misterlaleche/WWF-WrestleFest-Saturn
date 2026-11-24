#!/usr/bin/env python3
"""Compare TAG target selection and facing against intact original 68000 code.

The native probe compiles the production $20958 phase and $10BE8 facing
consumer. Expected actor bytes come only from running ROM68K.BIN, never from
a second implementation of target rules. The fixtures isolate target choice:
the legal opponent and their reserve are on opposite sides of the player.
"""
import argparse
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ACTOR_BASE = 0x1C05B0
STRIDE = 0x10C
SLOTS = 9
ACTOR_BYTES = STRIDE * SLOTS
RETURN_PC = 0x70000


def pointer(slot):
    return ACTOR_BASE + slot * STRIDE


def write(actors, slot, offset, value, width=1):
    start = slot * STRIDE + offset
    actors[start:start + width] = value.to_bytes(width, "big")


def read(actors, slot, offset, width=1):
    start = slot * STRIDE + offset
    return int.from_bytes(actors[start:start + width], "big")


@dataclass
class Fixture:
    name: str
    actors: bytes
    source: int = 0
    mode: int = 0
    facing_only: bool = False


def normal_actors(source=0, active=4, mirror=False):
    actors = bytearray(ACTOR_BYTES)
    partner = source ^ 1
    reserve = active ^ 1
    for slot, team, flags, role, x in (
        (source, partner, 3, 0, 0x280),
        (partner, source, 0, 0x40, 0x200),
        (active, reserve, 0x81, 0x80, 0x240 if mirror else 0x2C0),
        (reserve, active, 0x80, 0xC0, 0x2E0 if mirror else 0x200),
    ):
        write(actors, slot, 0, 0x80)
        write(actors, slot, 0x33, flags)
        write(actors, slot, 0x56, role)
        write(actors, slot, 6, x, 2)
        write(actors, slot, 10, 0x150, 2)
        write(actors, slot, 14, 0x140, 2)
        write(actors, slot, 0x86, pointer(team), 4)
        write(actors, slot, 0x7A, pointer(reserve), 4)
        write(actors, slot, 0x82, pointer(active), 4)
    return actors


def fixtures(snapshot=None):
    cases = []
    # Ordinary legal opponent selection is the reported failure. Both TAG
    # teammates, all four human slots and both directions use the same rule.
    for source in range(4):
        for active in (4, 5):
            for mirror in (False, True):
                for moving in (False, True):
                    actors = normal_actors(source, active, mirror)
                    write(actors, source, 0x20, int(moving), 2)
                    cases.append(Fixture(
                        f"normal_p{source + 1}_active{active}_{'left' if mirror else 'right'}_"
                        f"{'walk' if moving else 'idle'}", bytes(actors), source))
    # Branch cases preserve the original special targeting rules when either
    # wrestler is outside, linked, pinning or participating as a reserve.
    for legal in (0, 1):
        for source_outside in (0, 1):
            for reserve_outside in (0, 1):
                for linked in (0, 1):
                    for pin_flags in (0, 1, 2, 3):
                        for state, selector in ((0, 0), (5, 0), (5, 0x1D), (5, 0x61)):
                            actors = normal_actors()
                            write(actors, 0, 0x33, 2 | legal)
                            write(actors, 0, 0x32, source_outside)
                            write(actors, 5, 0x32, reserve_outside)
                            write(actors, 4, 0x33, 0x81 | (linked << 6))
                            write(actors, 4, 0x35, pin_flags)
                            write(actors, 4, 0x20, state, 2)
                            write(actors, 4, 0x60, selector, 2)
                            name = (f"branch_legal{legal}_srcout{source_outside}_reserveout{reserve_outside}_"
                                    f"linked{linked}_pin{pin_flags}_state{state}_sel{selector:02x}")
                            cases.append(Fixture(name, bytes(actors)))
    for source_ring in (0, 1):
        for active_ring in (0, 1):
            for reserve_linked in (0, 1):
                for active in (4, 5):
                    actors = normal_actors(active=active)
                    write(actors, 0, 0x33, 3 | (source_ring << 2))
                    write(actors, active, 0x33, 0x81 | (active_ring << 2))
                    write(actors, active ^ 1, 0x33, 0x80 | (reserve_linked << 6))
                    cases.append(Fixture(
                        f"ring_src{source_ring}_active{active_ring}_reslink{reserve_linked}_slot{active}",
                        bytes(actors)))
    # Pin-distance paths, including ties and 16-bit coordinate boundaries.
    for source_x, active_x, reserve_x in (
        (640, 680, 560), (640, 720, 600), (640, 680, 600),
        (0x7FFF, 0x8000, 0x7FFC), (0, 0xFFFF, 1),
    ):
        actors = normal_actors()
        write(actors, 0, 6, source_x, 2)
        write(actors, 4, 6, active_x, 2)
        write(actors, 5, 6, reserve_x, 2)
        write(actors, 4, 0x33, 0xC1)
        write(actors, 4, 0x35, 2)
        cases.append(Fixture(f"pin_distance_{source_x}_{active_x}_{reserve_x}", bytes(actors)))
    # Verify the consumer independently: with a correct pointer the current
    # facing function already matches the ROM, including lying/held targets.
    for mirror in (False, True):
        for state, substate, selector in ((0, 0, 0), (4, 8, 0), (4, 9, 0),
                                          (5, 0, 0x22), (5, 0, 0x52)):
            for target_facing in (0, 0x80):
                actors = normal_actors(mirror=mirror)
                write(actors, 0, 0x7A, pointer(4), 4)
                write(actors, 4, 0x20, state, 2)
                write(actors, 4, 0x64, substate, 2)
                write(actors, 4, 0x60, selector, 2)
                write(actors, 4, 0x2E, target_facing)
                cases.append(Fixture(
                    f"facing_only_{int(mirror)}_{state}_{substate}_{selector}_{target_facing}",
                    bytes(actors), facing_only=True))
    if snapshot:
        original = snapshot.read_bytes()
        if len(original) != ACTOR_BYTES:
            raise ValueError("Snapshot must contain exactly nine original actor records")
        cases.append(Fixture("legal_frontend_snapshot", original))
        for active_x, reserve_x, name in ((0x2C0, 0x200, "active_right"),
                                         (0x240, 0x2E0, "active_left")):
            actors = bytearray(original)
            write(actors, 0, 6, 0x280, 2)
            write(actors, 4, 6, active_x, 2)
            write(actors, 5, 6, reserve_x, 2)
            cases.append(Fixture("frontend_roles_" + name, bytes(actors)))
    return cases


class Oracle:
    def __init__(self, rom):
        import machine68k as mus
        if len(rom) != 0x80000:
            raise ValueError("ROM68K.BIN must be the complete 512 KiB original ROM")
        self.mus = mus
        self.machine = mus.Machine(mus.CPUType.M68000, 2048)
        self.cpu, self.mem = self.machine.cpu, self.machine.mem
        self.rom = rom

    def entry(self, pc, source):
        self.cpu.w_sp(0x1C3FF0)
        self.mem.w32(0x1C3FF0, RETURN_PC)
        self.cpu.w_reg(self.mus.Register.A0, pointer(source))
        self.cpu.w_pc(pc)
        trace = []
        for _ in range(10000):
            pc = self.cpu.r_pc()
            if pc == RETURN_PC:
                return trace
            if 0x20CCC <= pc < 0x20E00:
                trace.append(pc)
            self.cpu.execute(1)
        raise AssertionError(f"Original routine did not return: PC=${self.cpu.r_pc():06X}")

    def run(self, fixture):
        self.mem.w_block(0, self.rom)
        self.mem.w_block(0x1C0000, bytes(0x4000))
        self.mem.w_block(ACTOR_BASE, fixture.actors)
        self.mem.w8(0x1C0161, fixture.mode)
        self.cpu.w_sr(0x2700)
        for i in range(15):
            self.cpu.w_reg(i, 0)
        trace = []
        if not fixture.facing_only:
            trace = self.entry(0x20958, fixture.source)
        self.entry(0x10BE8, fixture.source)
        return bytes(self.mem.r_block(ACTOR_BASE, ACTOR_BYTES)), trace


def summary(actors, source):
    return {"target": f"{read(actors, source, 0x7A, 4):08X}",
            "secondary": f"{read(actors, source, 0x82, 4):08X}",
            "facing": f"{read(actors, source, 0x2E):02X}"}


def build_probe(directory, core, sanitize):
    # Copy only the test runner to adapt its include to the selected baseline.
    source = (ROOT / "tests/test_tag_facing_probe.c").read_text()
    source = source.replace('"../core/src/wf_actor_prepass.c"',
                            '"' + str(core / "src/wf_actor_prepass.c") + '"')
    cfile, binary = directory / "probe.c", directory / "probe"
    cfile.write_text(source)
    flags = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
             "-ffunction-sections", "-fdata-sections"]
    if sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    subprocess.run([os.environ.get("CC", "cc"), *flags, "-I" + str(core / "include"),
                    str(cfile), str(core / "src/wf_actor.c"),
                    str(core / "src/wf_target_relation.c"),
                    str(core / "src/wf_r17_m68k_shadow.c"),
                    "-Wl,--gc-sections", "-o", str(binary)], check=True)
    return binary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROOT / "cd/ROM68K.BIN")
    parser.add_argument("--core", type=Path, default=ROOT / "core")
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--summary", type=Path)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    rom = args.rom.read_bytes()
    oracle = Oracle(rom)
    cases = fixtures(args.snapshot)
    native_input = b"".join(struct.pack("BBBB", case.mode, case.source, case.facing_only, 0)
                            + case.actors for case in cases)
    with tempfile.TemporaryDirectory(prefix="wf-tag-facing-") as temporary:
        binary = build_probe(Path(temporary), args.core.resolve(), args.sanitize)
        output = subprocess.run([str(binary)], input=native_input, capture_output=True)
        if output.returncode:
            raise RuntimeError(f"Native target/facing probe exited {output.returncode}:\n"
                               + output.stderr.decode())
    if output.stderr:
        raise AssertionError(output.stderr.decode())
    record_size = 2 + ACTOR_BYTES
    if len(output.stdout) != record_size * len(cases):
        raise AssertionError("Incomplete native actor output")
    failures, first_cases = [], []
    for index, case in enumerate(cases):
        record = output.stdout[index * record_size:(index + 1) * record_size]
        if record[:2] != b"\0\0":
            raise AssertionError((case.name, "native routine blocked", list(record[:2])))
        actual, (expected, trace) = record[2:], oracle.run(case)
        differences = [offset for offset, (a, b) in enumerate(zip(actual, expected)) if a != b]
        item = {"case": case.name, "native": summary(actual, case.source),
                "rom": summary(expected, case.source)}
        if differences:
            item["byte_differences"] = [f"slot{offset // STRIDE}+{offset % STRIDE:02X}"
                                         for offset in differences]
            item["rom_target_path"] = [f"{pc:06X}" for pc in trace]
            failures.append(item)
        if index < 4 or case.name.startswith("frontend_") or case.name == "legal_frontend_snapshot":
            first_cases.append(item)
    result = {"fixtures": len(cases), "failed": len(failures),
              "passed": len(cases) - len(failures),
              "facing_only_fixtures": sum(case.facing_only for case in cases),
              "facing_only_failed": sum(item["case"].startswith("facing_only_") for item in failures),
              "rom_sha256": hashlib.sha256(rom).hexdigest(),
              "core": str(args.core), "examples": first_cases, "failures": failures,
              "scope": "Actor bytes after original $20958 and $10BE8 versus native target phase and facing consumer; no full-frame or hardware claim"}
    if args.summary:
        args.summary.parent.mkdir(parents=True, exist_ok=True)
        args.summary.write_text(json.dumps(result, indent=2) + "\n")
    for failure in failures[:4]:
        print(f"FAIL {failure['case']}: native={failure['native']} ROM={failure['rom']}")
    print(f"{'FAIL' if failures else 'PASS'} TAG target/facing ROM parity: "
          f"{result['passed']}/{len(cases)} fixtures; "
          f"facing-only mismatches={result['facing_only_failed']}")
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
