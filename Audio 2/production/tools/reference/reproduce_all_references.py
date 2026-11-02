#!/usr/bin/env python3
"""Reproduce 18 music references, 98 model SFX, and independent host gates.

Only the user's original ROM zip and a host C compiler/Python are required.
Nuked-OPM is downloaded at the immutable A53 pin, unless --opm-dir is supplied.
All private ROM data, audio, traces, fixtures and binaries go under --out.
This host-only workflow does not add software FM synthesis to Saturn firmware.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import urllib.request

from prepare_reference_inputs import prepare

HERE = Path(__file__).resolve().parent
OPM_COMMIT = "f209e6ed3712032b641d53ce8fb24824eae6adc3"
OPM_SHA256 = {
    "opm.c": "983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188",
    "opm.h": "186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39",
}


def dependencies(destination: Path, existing: Path | None) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    for name, expected in OPM_SHA256.items():
        if existing is not None:
            data = (existing / name).read_bytes()
        elif (destination / name).exists():
            data = (destination / name).read_bytes()
        else:
            url = f"https://raw.githubusercontent.com/nukeykt/Nuked-OPM/{OPM_COMMIT}/{name}"
            with urllib.request.urlopen(url, timeout=30) as response:
                data = response.read()
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError("Pinned Nuked-OPM dependency mismatch: " + name)
        (destination / name).write_bytes(data)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-zip", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--opm-dir", type=Path, help="Use already downloaded, hash-verified pinned sources")
    parser.add_argument("--seconds", type=float, default=30)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--prepare-only", action="store_true", help="Recreate and verify private inputs without downloading or rendering")
    args = parser.parse_args()
    if not 12 <= args.seconds <= 600 or args.jobs < 1:
        parser.error("Need 12..600 seconds and at least one worker")
    out = args.out.resolve()
    production = HERE.parents[1]
    if out == production or production in out.parents:
        parser.error("Choose --out outside the production source package for private ROM/audio outputs")
    inputs = out / "inputs"
    prepare(args.rom_zip.resolve(), inputs)
    print("All private inputs reconstructed; complete A15 PCM8, PCM16 and model C hashes match", flush=True)
    if args.prepare_only:
        return 0
    deps = out / "deps"
    dependencies(deps, args.opm_dir.resolve() if args.opm_dir else None)
    stages = [
        ("music_reference", "render_reference.py", [
            "--rom", inputs / "SOUND.ROM", "--pcm", inputs / "AUDPCM.PAK",
            "--index", inputs / "AUDPCM-INDEX.csv", "--opm-dir", deps,
            "--out", out / "music", "--seconds", args.seconds, "--jobs", args.jobs]),
        ("ym_and_sequence", "verify_ym_and_seq.py", [
            "--rom", inputs / "SOUND.ROM", "--traces", out / "music/traces",
            "--opm", deps, "--out", out / "logic"]),
        ("ym_register_usage", "audit_ym_usage.py", [
            "--traces", out / "music/traces", "--out", out / "ym-register-usage.json"]),
        ("sfx_native", "verify_sfx_native.py", [
            "--rom", inputs / "SAMPLES.ROM", "--reference", inputs / "a15_model",
            "--pack", inputs / "SFX.DAT", "--out", out / "sfx-validation"]),
        ("sfx_model_reference", "render_sfx_reference.py", [
            "--golden", out / "sfx-validation/WF_OKI_PCM16BE.PAK",
            "--fixtures", out / "sfx-validation", "--reference", inputs / "a15_model",
            "--out", out / "sfx"]),
    ]
    report = {
        "kind": "HOST_REFERENCE_AND_LOGIC_VALIDATION_NOT_NATIVE_AUDIO_ACCEPTANCE",
        "opm_commit": OPM_COMMIT, "seconds_per_music": args.seconds,
        "external_handoff_required": False, "stages": [], "complete": False,
    }
    for name, script, parameters in stages:
        command = [sys.executable, str(HERE / script), *map(str, parameters)]
        if name != "ym_register_usage":
            command += ["--cc", args.cc]
        print("Running " + name, flush=True)
        result = subprocess.run(command)
        report["stages"].append({"name": name, "command": command, "exit_code": result.returncode})
        (out / "reproduction-report.json").write_text(json.dumps(report, indent=2) + "\n")
        if result.returncode:
            return result.returncode
    report["complete"] = True
    (out / "reproduction-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Complete: 18 A53 music prefixes, 98 model SFX, pinned YM/sequence and full-PCM gates passed", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
