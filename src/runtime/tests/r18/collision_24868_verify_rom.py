"""Exact Royal frame3124 source5/target4 equal-facing pair versus 68000 ROM."""
import ctypes
import json
import os
import re
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path
import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
REPORT = ROOT.parents[1] / "validation/local-20261025/royal-stage9ad-3300/report.json"
ROM = (ROOT / "audit/runtime18-cd/ROM68K.BIN").read_bytes()
evidence = json.loads(REPORT.read_text())
assert evidence["fatal_owner"] == 0x24062
assert evidence["fatal_dependency"] == 0x246ea
assert evidence["fatal_frame"] == 3124
assert evidence["wf_collision_blocked_probe"][:6] == [5, 4, 17, 9, 8, 1]
actors = b"".join(bytes.fromhex(x) for x in evidence["actors_at_block"])
assert len(actors) == 9 * 0x10c
mode = (evidence["fatal_diagnostic"][7] >> 16) & 255
regs = evidence["shadow_d_at_block"]
core = re.search(r"^CORE=(.*)$", (ROOT / "Makefile").read_text(), re.M)[1].split()

with tempfile.TemporaryDirectory(prefix="wf-collision24868-", ignore_cleanup_errors=os.name == "nt") as tmp:
    frozen = Path(tmp)
    for folder in ("src", "include", "generated"):
        shutil.copytree(ROOT / folder, frozen / folder)
    bridge = "tests/r18/collision_24868_oracle_bridge.c"
    target = frozen / bridge
    target.parent.mkdir(parents=True)
    shutil.copy2(ROOT / bridge, target)
    library = frozen / "collision24868.so"
    subprocess.run(["cc", "-shared", "-fPIC", "-std=c11", "-O2", "-Wall", "-Wextra",
                    "-Werror", "-Iinclude", *core, bridge, "-o", str(library)],
                   cwd=frozen, check=True)
    native = ctypes.CDLL(str(library))
    native.collision_24868_oracle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint32)]
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    cpu, mem = machine.cpu, machine.mem
    cases = 0
    for face in (0, 1, 0x80, 0xff):
        for timer in (0, 3):
            m = bytearray(0x200000)
            m[:len(ROM)] = ROM
            m[0x1c05b0:0x1c05b0 + len(actors)] = actors
            m[0x1c005c:0x1c0060] = evidence["fatal_diagnostic"][4].to_bytes(4, "big")
            m[0x1c0080:0x1c0084] = evidence["fatal_frame"].to_bytes(4, "big")
            m[0x1c0161] = mode
            source = 0x1c05b0 + 5 * 0x10c
            receiver = 0x1c05b0 + 4 * 0x10c
            m[source + 0x2e] = m[receiver + 0x2e] = face
            m[receiver + 0x52:receiver + 0x54] = timer.to_bytes(2, "big")
            mem.w_block(0, bytes(m))
            cpu.w_sp(0x1d0000)
            mem.w32(0x1d0000, 0x70000)
            cpu.w_pc(0x2435a)
            for i, value in enumerate(regs): cpu.w_reg(i, value)
            for reg, value in ((8, source), (9, receiver),
                               (10, 0x24ef6 + 17 * 7), (11, 0x24ef6 + 9 * 7)):
                cpu.w_reg(reg, value)
            for _ in range(4000):
                if cpu.r_pc() == 0x70000: break
                cpu.execute(1)
            assert cpu.r_pc() == 0x70000, (face, timer, hex(cpu.r_pc()))
            expected = 0 if cpu.r_sr() & 1 else 1
            image = ctypes.create_string_buffer(bytes(m))
            d = (ctypes.c_uint32 * 8)(*regs)
            actual = native.collision_24868_oracle(image, d)
            assert actual == expected, (face, timer, "result", actual, expected)
            for address, length in ((0x1c0000, 0x2000), (0xc0000, 0x4000)):
                assert image.raw[address:address + length] == bytes(mem.r_block(address, length)), (
                    face, timer, "RAM/FG", address)
            assert list(d) == [cpu.r_reg(i) for i in range(8)], (face, timer, "D0-D7")
            cases += 1
    machine.cleanup()
print(f"PASS {cases} collision24868 equal-facing cases; RAM, FG, D0-D7")
