#!/usr/bin/env python3
"""Run production action callbacks with only the audio device captured.

--rom-oracle requires machine68k and executes the unmodified arcade ROM.
--state-baseline compares every typed gameplay field and live D register with
an older actions translation while keeping all other production core identical.
"""
import argparse
import concurrent.futures
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--actions-source", type=Path)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--rom-oracle", action="store_true")
    parser.add_argument("--state-baseline", type=Path,
                        help="frozen actions source used to compare all gameplay state")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="wf-actions-audio-") as temporary:
        work = (args.build_dir or Path(temporary)).resolve()
        work.mkdir(parents=True, exist_ok=True)
        flags = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                 "-ffunction-sections", "-fdata-sections"]
        flags += ["-I" + str(ROOT / p) for p in ("core/include", "core/src")]
        sources = [ROOT / p for p in dict.fromkeys(re.findall(
            r"core/(?:src|generated)/\w+\.c", (ROOT / "core_sources.mk").read_text()))]
        if args.actions_source:
            sources = [args.actions_source.resolve() if p.name == "wf_r17_actions_r11.c" else p for p in sources]
        sources += [ROOT / "tests/test_actions_audio.c"]
        headers = max(p.stat().st_mtime for p in (ROOT / "core/include").glob("*.h"))

        def compile_source(source):
            obj = work / (source.stem + ".o")
            # The actions translation includes local .inc files and may be
            # replaced by a frozen source with an older timestamp.
            if source.name == "wf_r17_actions_r11.c" or not obj.exists() or obj.stat().st_mtime < max(source.stat().st_mtime, headers):
                subprocess.run([os.environ.get("CC", "cc"), *flags, "-c", str(source), "-o", str(obj)], check=True)
            return str(obj)

        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            objects = list(pool.map(compile_source, sources))
        binary = work / "test"
        subprocess.run([os.environ.get("CC", "cc"), "-Wl,--gc-sections", *objects, "-o", str(binary)], check=True)
        result = subprocess.run([str(binary), *(["--traces"] if args.rom_oracle else [])],
                                capture_output=True, text=True)
        for line in result.stdout.splitlines():
            if not line.startswith("TRACE "):
                print(line)
        if result.stderr:
            print(result.stderr, end="")
        result.check_returncode()
        if args.rom_oracle:
            from actions_audio_rom_oracle import verify_native_traces
            verify_native_traces((ROOT / "cd/ROM68K.BIN").read_bytes(),
                                 [line for line in result.stdout.splitlines() if line.startswith("TRACE ")])
        if args.state_baseline:
            old_object = work / "actions_baseline.o"
            subprocess.run([os.environ.get("CC", "cc"), *flags, "-c",
                            str(args.state_baseline.resolve()), "-o", str(old_object)], check=True)
            old_binary = work / "test_baseline"
            old_objects = [str(old_object) if Path(obj).name == "wf_r17_actions_r11.o" else obj
                           for obj in objects]
            subprocess.run([os.environ.get("CC", "cc"), "-Wl,--gc-sections", *old_objects,
                            "-o", str(old_binary)], check=True)
            current = subprocess.run([str(binary), "--state"], capture_output=True, text=True, check=True).stdout
            baseline = subprocess.run([str(old_binary), "--state"], capture_output=True, text=True, check=True).stdout
            if current != baseline:
                differences = [(a, b) for a, b in zip(current.splitlines(), baseline.splitlines()) if a != b]
                raise AssertionError(("gameplay state changed", differences[:20]))
            print(f"PASS: {len(current.splitlines())} fixtures preserve frozen gameplay state, RNG, D registers and callback status")


if __name__ == "__main__":
    main()
