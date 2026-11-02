#!/usr/bin/env python3
"""Render18 original-ROM music windows with the verified board YM clock.

Default profile: YM2151 external3579545Hz /2 pipeline divider, original
initial-zero PCM16 samples at8000Hz. This is a host model, not hardware audio.
Historical A53 sources, PCM8 and prefix tests remain separately reproducible.
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

from prepare_reference_inputs import prepare, resources, verify_bundle
from reproduce_all_references import dependencies, OPM_COMMIT

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def renderer(profile):
    original = HERE / "oracle_a53/tools/render_combined_a50.c"
    text = original.read_text()
    old = "        OPM_Clock(&opm,y,0,0,0);"
    if text.count(old) != 1:
        raise ValueError("Immutable A53 renderer clock statement mismatch")
    text = text.replace(old, "        /* Board clock divider2, before32operator pipeline. */\n"
                            "        if((cyc&1u)==0)OPM_Clock(&opm,y,0,0,0);")
    if profile == "board-pcm16":
        changes = {
            "    int8_t current;": "    int16_t current;",
            "    for(i=0;i<4;i++)if(v[i].playing){\n        int s=(int)v[i].current;":
            "    /* The final decoded sample is held until the next8kHz tick. */\n"
            "    for(i=0;i<4;i++){\n        int s=(int)v[i].current;",
            "        sum += (s*256*oki_num[v[i].volume&15u])/32;":
            "        sum += (s*oki_num[v[i].volume&15u])/32;",
            "    for(i=0;i<4;i++)if(v[i].playing){\n        if(v[i].pos>=v[i].bytes || (uint64_t)v[i].offset+v[i].pos>=pak_sz){":
            "    for(i=0;i<4;i++){\n"
            "        if(!v[i].playing){v[i].current=0;continue;}\n"
            "        if(v[i].pos>=v[i].bytes || (uint64_t)v[i].offset+2u*v[i].pos+1u>=pak_sz){",
            "        v[i].current=(int8_t)pak[v[i].offset+v[i].pos++];":
            "        uint32_t at=v[i].offset+2u*v[i].pos++;\n"
            "        v[i].current=(int16_t)((uint16_t)pak[at]*256u+pak[at+1]);",
        }
        for old, new in changes.items():
            if text.count(old) != 1:
                raise ValueError("Immutable A53 PCM statement mismatch: " + old)
            text = text.replace(old, new)
    return text


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--rom-zip", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--opm-dir", type=Path)
    p.add_argument("--seconds", type=float, default=30)
    p.add_argument("--jobs", type=int, default=2)
    p.add_argument("--cc", default="cc")
    p.add_argument("--profile", choices=("board-pcm16", "divider-only-pcm8"), default="board-pcm16")
    a = p.parse_args()
    if not 1 <= a.seconds <= 600 or a.jobs < 1:
        p.error("Need1..600seconds and positive worker count")
    a.out = a.out.resolve()
    production = HERE.parents[1]
    if a.out == production or production in a.out.parents:
        p.error("Private ROM/audio output must stay outside the source package")
    a.out.mkdir(parents=True, exist_ok=True)
    verify_bundle()
    inputs, deps = a.out / "inputs", a.out / "deps"
    prepare(a.rom_zip.resolve(), inputs)
    dependencies(deps, a.opm_dir.resolve() if a.opm_dir else None)
    generated = resources.generate((inputs / "SOUND.ROM").read_bytes(), (inputs / "SAMPLES.ROM").read_bytes())
    pcm, index = inputs / "AUDPCM.PAK", inputs / "AUDPCM-INDEX.csv"
    if a.profile == "board-pcm16":
        pcm, index = inputs / "PCM16BE.PAK", inputs / "PCM16-INDEX.csv"
        if sha(pcm) != "45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee":
            raise ValueError("Original initial-zero PCM16 pack mismatch")
        with index.open("w", newline="") as stream:
            writer = csv.writer(stream)
            writer.writerow(("bank", "sample_id", "unused", "offset_bytes", "pcm16_frames"))
            for row in generated["samples"]:
                writer.writerow((row["bank"], row["sample_id"], 0, row["offset"], row["frames"]))
    z80 = production / "src/reference_driver"
    capture_source = HERE / "oracle_a53/tools/capture_full_audio_a50.c"
    derived = a.out / "render_board_clock.c"
    derived.write_text(renderer(a.profile))
    commands = [
        [a.cc, "-std=c99", "-O3", "-I", str(z80), str(capture_source), str(z80 / "z80.c"), "-o", str(a.out / "capture_original")],
        [a.cc, "-std=c99", "-O3", "-I", str(deps), str(derived), str(deps / "opm.c"), "-lm", "-o", str(a.out / "render_board_clock")],
    ]
    for command in commands:
        subprocess.run(command, check=True, capture_output=True, text=True)
    for name in ("traces", "pcm", "wav"):
        (a.out / name).mkdir(exist_ok=True)
    report = {
        "kind": "BOARD_CLOCK_HOST_REFERENCE_NOT_PHYSICAL_ARCADE_RECORDING",
        "profile": a.profile, "opm_commit": OPM_COMMIT,
        "external_ym_clock_hz": 3579545, "opm_pipeline_divider": 2, "operator_count": 32,
        "effective_ym_output_hz": 3579545 / 64, "oki_hz": 8000,
        "sample_rate": 44100, "channels": 1, "bits": 16,
        "normalization": False, "time_stretch": False,
        "input_oki_format": ("Original ROM PCM16BE; initial signal/step0; all final decoded samples retained" if a.profile == "board-pcm16" else
                             "Historical A15 PCM8; initial signal−2; original A53 tail behavior preserved for causal diagnostic"),
        "route_gains": {"YM_left_Q15": 10813, "YM_right_Q15": 10813, "OKI_Q15": 21627,
                        "meaning": "nominal MAME board routing0.33/0.33/0.66; not measured analog calibration"},
        "resampling": "Direct sample/hold at44100Hz; no analog reconstruction filter; not an analog board recording",
        "origin_rule": "Unchanged original Z80 trace ORIGIN after two-second reset/boot; synth runs during boot",
        "historical_a53": "Unmodified, separately tested; its doubled YM clock is not an absolute pitch oracle",
        "source_sha256": {str(path): sha(path) for path in [capture_source, derived, z80 / "z80.c", z80 / "z80.h", deps / "opm.c", deps / "opm.h"]},
        "input_sha256": {str(path): sha(path) for path in [a.rom_zip, inputs / "SOUND.ROM", inputs / "SAMPLES.ROM", pcm, index]},
        "build_commands": commands, "tracks": [], "complete": False,
    }

    def render(command):
        started = time.monotonic()
        trace = a.out / "traces" / f"music-{command:02x}.csv"
        raw = a.out / "pcm" / f"music-{command:02x}.pcm"
        prefix = "ARCADE-REFERENCIA" if a.profile == "board-pcm16" else "CLOCK-DIVIDER-ONLY"
        wav = a.out / "wav" / f"{prefix}-{command:02d}.wav"
        run = [[str(a.out / "capture_original"), str(inputs / "SOUND.ROM"), f"{command:x}", str(a.seconds), str(trace)],
               [str(a.out / "render_board_clock"), str(trace), str(pcm), str(index), str(raw)]]
        for item in run:
            subprocess.run(item, check=True, capture_output=True, text=True)
        data = raw.read_bytes()
        samples = array.array("h", data)
        if sys.byteorder == "little":
            samples.byteswap()
        peak = max(abs(v) for v in samples)
        clipped = sum(v in (-32768, 32767) for v in samples)
        if sys.byteorder == "big":
            samples.byteswap()
        with wave.open(str(wav), "wb") as sink:
            sink.setnchannels(1); sink.setsampwidth(2); sink.setframerate(44100)
            sink.writeframes(samples.tobytes())
        with trace.open(newline="") as stream:
            epochs = {row["kind"]: int(row["cycle"]) for row in csv.DictReader(stream) if row["kind"] in ("ORIGIN", "END")}
        return {"command": command, "hex": f"0x{command:02X}", "samples": len(data) // 2,
                "seconds": len(data) / 88200, "peak": peak, "clipped_samples": clipped,
                "origin_z80_cycle": epochs["ORIGIN"], "end_z80_cycle": epochs["END"],
                "trace": str(trace), "pcm": str(raw), "wav": str(wav),
                "trace_sha256": sha(trace), "pcm_sha256": sha(raw), "wav_sha256": sha(wav),
                "commands": run, "wall_seconds": time.monotonic() - started}

    with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
        for result in concurrent.futures.as_completed([pool.submit(render, command) for command in range(1, 19)]):
            item = result.result()
            report["tracks"].append(item); report["tracks"].sort(key=lambda r: r["command"])
            (a.out / "reference-report.json").write_text(json.dumps(report, indent=2) + "\n")
            print(f"{item['hex']}: {item['samples']} samples; {a.profile}", flush=True)
    report["complete"] = len(report["tracks"]) == 18
    (a.out / "reference-report.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["complete"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
