#!/usr/bin/env python3
"""Reproduce TAG ring-exit F560/26E66 and compare with the intact arcade ROM.

The fixture enters the original legal raw68 landing branch. It executes the
production action dispatcher followed by F98C with the real scene transition.
The oracle executes both original owners, including map/palette/audio routines;
only the external VBlank input pin changes at their hardware wait boundaries.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def build(work, transition_source=None):
    flags = ["-std=c11", "-O2", "-g", "-Wall", "-Wextra", "-Werror",
             "-ffunction-sections", "-fdata-sections", "-I" + str(ROOT / "core/include"),
             "-I" + str(ROOT / "core/src")]
    sources = [ROOT / p for p in dict.fromkeys(re.findall(
        r"core/(?:src|generated)/\w+\.c", (ROOT / "core_sources.mk").read_text()))]
    if transition_source:
        sources = [transition_source.resolve() if p.name == "wf_r18_transition.c" else p
                   for p in sources]
    sources.append(ROOT / "tests/test_tag_ringout.c")
    latest = max(p.stat().st_mtime for p in (ROOT / "core/include").glob("*.h"))
    latest = max(latest, max(p.stat().st_mtime for p in (ROOT / "core/src").glob("*.inc")))
    cc = os.environ.get("CC", "cc")

    def compile_one(source):
        obj = work / (source.stem + ".o")
        if source.name == "wf_r18_transition.c" or not obj.exists() or obj.stat().st_mtime < max(latest, source.stat().st_mtime):
            subprocess.run([cc, *flags, "-c", str(source), "-o", str(obj)], check=True)
        return str(obj)

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        objects = list(pool.map(compile_one, sources))
    binary = work / "test_tag_ringout"
    subprocess.run([cc, "-Wl,--gc-sections", *objects, "-o", str(binary)], check=True)
    return binary


def rom_oracle(rom, folder, compare):
    import machine68k as mus
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    mem, cpu = machine.mem, machine.cpu
    mem.w_block(0, rom)
    mem.w_block(0x1C0000, (folder / "before_action.bin").read_bytes())
    for reg in range(15):
        cpu.w_reg(reg, 0x12340000 + reg if reg < 8 else 0)
    cpu.w_sr(0x2700)
    cpu.w_reg(mus.Register.A0, 0x1C05B0)
    audio, owners = [], []

    def run(entry):
        cpu.w_sp(0x1C3FF0)
        mem.w32(0x1C3FF0, 0x70000)
        cpu.w_pc(entry)
        for count in range(100000):
            pc = cpu.r_pc()
            if pc == 0x70000:
                return count
            if pc in (0xFB5A, 0xFC80, 0x2A0A):
                mem.w8(0x140026, 4)
            elif pc in (0xFB8A, 0xFCB0, 0x2A88):
                mem.w8(0x140026, 0)
            if pc in (0x26E66, 0x2A06, 0xC1EA):
                owners.append(f"{pc:05X}")
            if pc == 0x206C:
                audio.append(cpu.r_reg(mus.Register.D0) & 0xFFFF)
            cpu.execute(1)
        raise AssertionError(f"original {entry:05X} did not return; PC={cpu.r_pc():05X}")

    action_steps = run(0x1C03E)
    expected_action = (folder / "before_f98c.bin").read_bytes()
    assert bytes(mem.r_block(0x1C0000, 0x2000)) == expected_action[:0x2000], "raw68 state differs from ROM"
    transition_steps = run(0xF98C)
    outputs = {"after_f98c.bin": bytes(mem.r_block(0x1C0000, 0x4000)),
               "background.bin": bytes(mem.r_block(0x80000, 0x4000)),
               "fg_palette.bin": bytes(mem.r_block(0x188000, 0x800)),
               "bg_palette.bin": bytes(mem.r_block(0x186000, 0x800)),
               "palettes.bin": bytes(mem.r_block(0x180000, 0x4000)),
               "hardware.bin": bytes(mem.r_block(0x100000, 8)) + bytes([mem.r16(0x140010) & 255]),
               "registers.bin": b"".join(cpu.r_reg(reg).to_bytes(4, "big") for reg in range(8))}
    compared = []
    for name, data in outputs.items():
        (folder / ("rom_" + name)).write_bytes(data)
        if compare:
            native = (folder / name).read_bytes()
            # The host has no emulated stack; compare all original live state
            # below0x2000, including actors, referee, globals and world cache.
            n = 0x2000 if name == "after_f98c.bin" else len(data)
            if native[:n] != data[:n]:
                diff = [(f"{i:04X}", a, b) for i, (a, b) in enumerate(zip(native[:n], data[:n])) if a != b]
                raise AssertionError((str(folder), name, diff[:20]))
            compared.append({"name": name, "bytes": n, "sha256": hashlib.sha256(data[:n]).hexdigest()})
    assert owners == ["26E66", "02A06", "0C1EA"]
    assert audio == [0x3180]
    assert mem.r32(0x1C005C) == 0x1234ABCD and mem.r32(0x1C0080) == 2608
    result = {"world": mem.r16(0x1C007E), "priority": mem.r16(0x140010),
              "mode": mem.r8(0x1C0161), "position": [mem.r16(0x1C1804), mem.r16(0x1C1806)],
              "owners": list(owners), "audio": list(audio), "action_steps": action_steps,
              "transition_steps": transition_steps, "rng": mem.r32(0x1C005C),
              "tick": mem.r32(0x1C0080), "compared": compared}
    if compare:
        # Retain the completed original exit. Admit only raw69's normal climb
        # entry fields, exactly as the native fixture does; no transition flag,
        # other actor, world, palette, map, RNG or live register is replaced.
        actor = 0x1C05B0
        facing = mem.r8(actor + 0x2E) >> 7
        for offset, value in ((0x20, 5), (0x1C, 5), (0x60, 0x69), (0x24, 0), (0x22, 0),
                              (6, 0x42C if facing else 0x210), (10, 0x120), (14, 0x100), (0x36, 0x0400)):
            mem.w16(actor + offset, value)
        mem.w8(actor + 0x32, mem.r8(actor + 0x32) & ~0x20)
        assert bytes(mem.r_block(0x1C0000, 0x2000)) == (folder / "before_return_action.bin").read_bytes()[:0x2000]
        callbacks = 0
        while not mem.r8(0x1C1678) & 0x80:
            assert callbacks < 128, "original raw69 never requested reentry"
            cpu.w_reg(mus.Register.A0, actor)
            run(0x1C03E)
            callbacks += 1
        assert mem.r8(0x1C1678) == 0xC0
        native_return = (folder / "before_return_f98c.bin").read_bytes()[:0x2000]
        original_return = bytes(mem.r_block(0x1C0000, 0x2000))
        if native_return != original_return:
            diff = [(f"{i:04X}", a, b) for i, (a, b) in enumerate(zip(native_return, original_return)) if a != b]
            raise AssertionError(("raw69 state differs from ROM", callbacks, diff[:20]))
        return_steps = run(0xF98C)
        return_outputs = {"after_return_f98c.bin": bytes(mem.r_block(0x1C0000, 0x4000)),
                          "return_background.bin": bytes(mem.r_block(0x80000, 0x4000)),
                          "return_fg_palette.bin": bytes(mem.r_block(0x188000, 0x800)),
                          "return_bg_palette.bin": bytes(mem.r_block(0x186000, 0x800)),
                          "return_palettes.bin": bytes(mem.r_block(0x180000, 0x4000)),
                          "return_hardware.bin": bytes(mem.r_block(0x100000, 8)) + bytes([mem.r16(0x140010) & 255]),
                          "return_registers.bin": b"".join(cpu.r_reg(reg).to_bytes(4, "big") for reg in range(8))}
        return_compared = []
        for name, data in return_outputs.items():
            (folder / ("rom_" + name)).write_bytes(data)
            native = (folder / name).read_bytes()
            n = 0x2000 if name == "after_return_f98c.bin" else len(data)
            if native[:n] != data[:n]:
                diff = [(f"{i:04X}", a, b) for i, (a, b) in enumerate(zip(native[:n], data[:n])) if a != b]
                raise AssertionError((str(folder), name, diff[:20]))
            return_compared.append({"name": name, "bytes": n, "sha256": hashlib.sha256(data[:n]).hexdigest()})
        assert owners == ["26E66", "02A06", "0C1EA"] * 2
        assert audio == [0x3180, 0x3120]
        assert mem.r32(0x1C005C) == 0x1234ABCD and mem.r32(0x1C0080) == 2608
        result["reentry"] = {"world": mem.r16(0x1C007E), "mode": mem.r8(0x1C0161),
                             "raw69_callbacks": callbacks, "transition_steps": return_steps,
                             "position": [mem.r16(0x1C1804), mem.r16(0x1C1806)], "audio": list(audio),
                             "compared": return_compared}
    (folder / "rom.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", type=Path, default=ROOT / "source_assets")
    parser.add_argument("--world6", type=Path, default=ROOT / "source_assets/R18MAP6.BIN")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build/b12-tag-ringout")
    parser.add_argument("--evidence-dir", type=Path, default=ROOT / "evidence/boot12/ringout/oracle")
    parser.add_argument("--transition-source", type=Path)
    parser.add_argument("--progressions", default="0,1,2,3,4,5,6,7,8,9")
    parser.add_argument("--rom-oracle", action="store_true")
    args = parser.parse_args()
    args.build_dir.mkdir(parents=True, exist_ok=True)
    args.evidence_dir.mkdir(parents=True, exist_ok=True)
    binary = build(args.build_dir.resolve(), args.transition_source)
    rom = (args.assets / "ROM68K.BIN").read_bytes()
    assert len(rom) == 0x80000
    assert rom[0xFA00:0xFA0A] == bytes([2, 6, 0, 6, 2, 6, 2, 2, 6, 2])
    assert rom[0xFA0A:0xFA14] == bytes([0, 5, 0, 5, 0, 5, 0, 0, 5, 0])
    failures, cases = 0, []
    for progression in map(int, args.progressions.split(",")):
        for facing in range(2):
            folder = args.evidence_dir.resolve() / f"progression{progression}-facing{facing}"
            folder.mkdir(parents=True, exist_ok=True)
            run = subprocess.run([str(binary), str(args.assets.resolve()), str(args.world6.resolve()),
                                  str(folder), str(progression), str(facing)], capture_output=True, text=True)
            (folder / "native.log").write_text(run.stdout + run.stderr)
            print(run.stdout, end="")
            if run.stderr:
                print(run.stderr, end="")
            failures += run.returncode != 0
            case = {"progression": progression, "facing": facing, "native_exit": run.returncode}
            if args.rom_oracle and (folder / "before_action.bin").exists():
                case["oracle"] = rom_oracle(rom, folder, run.returncode == 0)
            cases.append(case)
    result = {"rom_sha256": hashlib.sha256(rom).hexdigest(), "rom_table_FA00": rom[0xFA00:0xFA0A].hex(),
              "rom_table_FA0A": rom[0xFA0A:0xFA14].hex(), "failures": failures, "cases": cases}
    (args.evidence_dir / "summary.json").write_text(json.dumps(result, indent=2) + "\n")
    if failures:
        raise SystemExit(1)
    print(f"PASS {len(cases)} TAG ring-exit/reentry chains" + (" against intact original ROM" if args.rom_oracle else ""))


if __name__ == "__main__":
    main()
