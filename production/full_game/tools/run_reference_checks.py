#!/usr/bin/env python3
"""Run original ROM/decoder checks in a new scratch directory, never in sources.

Requires a host C compiler, liblz4, and Unicorn 2.1.4 in this Python environment.
Does not execute Saturn binaries or measure hardware performance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--rom", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    rom, out = args.rom.resolve(), args.out.resolve()
    if not rom.is_file():
        raise SystemExit("Missing external ROM ZIP")
    if out.exists():
        raise SystemExit("Choose a new output directory; existing evidence is preserved")
    # Dependencies must be present before staging any sources.
    import unicorn
    if unicorn.__version__ != "2.1.4":
        raise SystemExit("Use pinned Unicorn 2.1.4")
    out.mkdir(parents=True)
    source_hashes = {}
    commands = []

    def copy(source, target):
        data = source.read_bytes()
        source_hashes[str(source.relative_to(ROOT))] = hashlib.sha256(data).hexdigest()
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)

    def run(command, cwd, label):
        with (out / (label + ".log")).open("w") as log:
            result = subprocess.run(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT, timeout=300)
        commands.append({"command": command, "cwd": str(cwd), "returncode": result.returncode})
        (out / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if result.returncode:
            raise SystemExit("Reference check failed; see " + str(out / (label + ".log")))
        print("PASS", label, flush=True)

    presenter = ROOT / "Escena Presentadora/source/research_rom"
    for source in presenter.iterdir():
        if source.suffix in (".py", ".c", ".h"):
            copy(source, out / "presenter" / source.name)
    run([sys.executable, "test_scene_against_rom.py", str(rom), "--wide"], out / "presenter", "presenter-rom")

    royal = ROOT / "modo Royal/wp1980/work/hwfix01/native"
    for stem in ("wp1980_inflate", "wp1980_inflate_dynamic", "wp1980_lz4"):
        copy(royal / "src" / (stem + ".c"), out / "royal/src" / (stem + ".c"))
        copy(royal / "include" / (stem + ".h"), out / "royal/include" / (stem + ".h"))
        copy(royal / "tests/r18" / (stem + "_test.py"), out / "royal/tests/r18" / (stem + "_test.py"))
    copy(royal / "tests/r18/wp1980_inflate_dynamic_runner.c", out / "royal/tests/r18/wp1980_inflate_dynamic_runner.c")
    for name in ("wp1980_inflate", "wp1980_lz4"):
        run([sys.executable, "tests/r18/" + name + "_test.py"], out / "royal", name)
    run([sys.executable, "tests/r18/wp1980_inflate_dynamic_test.py", "--sanitize", "--output", "audit/dynamic"], out / "royal", "dynamic-sanitized")
    for path, digest in source_hashes.items():
        if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != digest:
            raise SystemExit("Original source changed: " + path)
    presenter_result = json.loads((out / "presenter/native_differential_results.json").read_text())
    report = {"pass": True, "source_hashes": source_hashes,
              "presenter_cases": len(presenter_result["cases"]),
              "presenter_updates": sum(case["ticks"] for case in presenter_result["cases"]),
              "rom_sha1": presenter_result["rom_sha1"],
              "decoder_original_atlas_blocks": 0,
              "saturn_executed": False, "audio_played": False}
    (out / "summary.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k:v for k,v in report.items() if k != "source_hashes"}, indent=2))

if __name__ == "__main__":
    main()
