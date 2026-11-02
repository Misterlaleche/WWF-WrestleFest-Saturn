#!/usr/bin/env python3
"""Rebuild the preserved A53 software oracle and render auditable ROM traces.

This host-only tool never links Nuked-OPM into the Saturn firmware. Generated
WAV files are software reference renders, not recordings of arcade hardware.
The immutable A53 source and metrics are included beside this script. Use the
original sound ROM, exact legacy A15 PCM8 pack, and pinned dependency.
"""
from __future__ import annotations

import argparse
import array
import concurrent.futures
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
import wave

OPM_SHA256 = {
    "opm.c": "983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188",
    "opm.h": "186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39",
}
ROM_SHA256 = "f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7"
HERE = Path(__file__).resolve().parent


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "pcm", "index", "opm-dir", "out"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--handoff", type=Path, default=HERE / "oracle_a53")
    p.add_argument("--z80-dir", type=Path, default=HERE.parents[1] / "src/reference_driver")
    p.add_argument("--seconds", type=float, default=30.0)
    p.add_argument("--jobs", type=int, default=2)
    p.add_argument("--cc", default="cc")
    a = p.parse_args()
    if a.seconds < 12 or a.seconds > 600 or a.jobs < 1:
        p.error("At least 12 and at most 600 seconds; jobs must be positive")
    for name in ("handoff", "rom", "pcm", "index", "z80_dir", "opm_dir", "out"):
        setattr(a, name, getattr(a, name).resolve())
    for name, expected in OPM_SHA256.items():
        if sha(a.opm_dir / name) != expected:
            raise ValueError(f"Pinned Nuked-OPM mismatch: {name}")
    if sha(a.rom) != ROM_SHA256 or a.rom.stat().st_size != 65536:
        raise ValueError("Original sound ROM identity mismatch")
    if sha(a.pcm) != "bbb80c6444d9e9e2a5d9d496418b9650171562ef8cf32533d969fb2f4a5e34b9":
        raise ValueError("Exact original A15 signed PCM8 pack required; PCM16 is not interchangeable")
    if sha(a.index) != "18ba394e95a57b5f4688a866f400f6453a63e1e70752eb50cf7418d292560fa5":
        raise ValueError("Exact complete archived A15 PCM8 index required")
    with a.index.open(newline="") as f:
        pcm_index = list(csv.DictReader(f))
    if len(pcm_index) != 114 or not any(int(row["pcm8_bytes"]) > 1000 for row in pcm_index):
        raise ValueError("Need the full 114-entry A15 PCM index; the handoff tests index is a stub")
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "traces").mkdir(exist_ok=True)
    (a.out / "wav").mkdir(exist_ok=True)
    (a.out / "pcm").mkdir(exist_ok=True)
    source = a.handoff / "tools"
    metrics = a.handoff / "evidence/A53-EXACT-12S-RENDER-METRICS.csv"
    with metrics.open(newline="") as f:
        expected = {int(r["command"], 16): r for r in csv.DictReader(f)}
    commands = [
        [a.cc, "-std=c99", "-O3", "-I", str(a.z80_dir),
         str(source / "capture_full_audio_a50.c"), str(a.z80_dir / "z80.c"),
         "-o", str(a.out / "capture_a53")],
        [a.cc, "-std=c99", "-O3", "-I", str(a.opm_dir),
         str(source / "render_combined_a50.c"), str(a.opm_dir / "opm.c"),
         "-lm", "-o", str(a.out / "render_a53")],
    ]
    for command in commands:
        subprocess.run(command, check=True, capture_output=True, text=True)
    inputs = [a.rom, a.pcm, a.index, metrics,
              source / "capture_full_audio_a50.c", source / "render_combined_a50.c",
              a.z80_dir / "z80.c", a.z80_dir / "z80.h",
              a.opm_dir / "opm.c", a.opm_dir / "opm.h"]
    report = {
        "kind": "A53_HOST_SOFTWARE_REFERENCE_NOT_HARDWARE_CAPTURE",
        "opm_commit": "f209e6ed3712032b641d53ce8fb24824eae6adc3",
        "seconds_requested": a.seconds,
        "sample_rate": 44100, "channels": 1, "bits": 16,
        "input_oki_format": "Legacy A15 signed PCM8, initial decoder signal -2; exact archived pack",
        "pcm_format": "Output signed mono PCM16BE",
        "wav_format": "signed mono PCM16LE in WAV; samples preserved",
        "normalization": False,
        "origin_rule": "Full reset/boot synthesized; exported time 0 starts at each trace ORIGIN after two-second boot",
        "input_sha256": {str(path): sha(path) for path in inputs},
        "build_commands": commands,
        "compiler": subprocess.run([a.cc, "--version"], check=True,
                                   capture_output=True, text=True).stdout.splitlines()[0],
        "tracks": [],
    }

    def render(command: int) -> dict:
        started = time.monotonic()
        stem = f"music-{command:02x}"
        trace = a.out / "traces" / (stem + ".csv")
        pcm = a.out / "pcm" / (stem + ".pcm")
        wav = a.out / "wav" / f"ARCADE-REFERENCIA-{command:02d}.wav"
        capture_cmd = [str(a.out / "capture_a53"), str(a.rom), f"{command:x}",
                       str(a.seconds), str(trace)]
        render_cmd = [str(a.out / "render_a53"), str(trace), str(a.pcm),
                      str(a.index), str(pcm)]
        subprocess.run(capture_cmd, check=True, capture_output=True, text=True)
        result = subprocess.run(render_cmd, check=True, capture_output=True, text=True)
        origins = {}
        with trace.open(newline="") as stream:
            for row in csv.DictReader(stream):
                if row["kind"] in ("ORIGIN", "END"):
                    origins[row["kind"]] = int(row["cycle"])
        data = pcm.read_bytes()
        samples = array.array("h", data)
        if sys.byteorder == "little":
            samples.byteswap()
        peak = max(abs(value) for value in samples)
        clipped = sum(value in (-32768, 32767) for value in samples)
        prefix = hashlib.sha256(data[:12 * 44100 * 2]).hexdigest()
        golden = expected[command]["sha256"]
        if sys.byteorder == "big":
            samples.byteswap()
        with wave.open(str(wav), "wb") as sink:
            sink.setnchannels(1)
            sink.setsampwidth(2)
            sink.setframerate(44100)
            sink.writeframes(samples.tobytes())
        return {
            "command": command, "hex": f"0x{command:02X}",
            "samples": len(data) // 2, "seconds": len(data) / 88200,
            "peak": peak, "clipped_samples": clipped,
            "pcm_sha256": hashlib.sha256(data).hexdigest(),
            "prefix_12s_sha256": prefix, "a53_sha256": golden,
            "a53_prefix_match": prefix == golden,
            "trace_sha256": sha(trace), "wav_sha256": sha(wav),
            "trace": str(trace), "wav": str(wav), "pcm": str(pcm),
            "origin_z80_cycle": origins["ORIGIN"],
            "end_z80_cycle": origins["END"],
            "origin_seconds_from_reset": origins["ORIGIN"] / 3579545,
            "commands": [capture_cmd, render_cmd],
            "render_stderr": result.stderr.strip(),
            "elapsed_wall_seconds": time.monotonic() - started,
        }

    with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
        futures = [pool.submit(render, command) for command in range(1, 19)]
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            report["tracks"].append(result)
            report["tracks"].sort(key=lambda r: r["command"])
            (a.out / "reference-report.json").write_text(json.dumps(report, indent=2) + "\n")
            print(f"{result['hex']}: {result['samples']} samples; A53 prefix "
                  f"{'PASS' if result['a53_prefix_match'] else 'FAIL'}", flush=True)
    report["a53_prefix_passed"] = sum(r["a53_prefix_match"] for r in report["tracks"])
    report["complete"] = len(report["tracks"]) == 18 and report["a53_prefix_passed"] == 18
    (a.out / "reference-report.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
