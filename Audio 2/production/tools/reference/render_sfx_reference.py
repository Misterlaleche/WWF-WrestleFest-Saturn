#!/usr/bin/env python3
"""Export all 98 A15 model SFX commands, with full events/tails and 250ms silence.

This is a model-level 44.1kHz reference using exact audited 8kHz original-ROM
samples, integer OKI levels, and the documented R04 21627/32768 routing gain.
It is not an A53 YM render, a native SCSP capture, or a physical arcade capture.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import wave


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("golden", "fixtures", "reference", "out"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--cc", default="cc")
    a = p.parse_args()
    for name in ("golden", "fixtures", "reference", "out"):
        setattr(a, name, getattr(a, name).resolve())
    if sha(a.golden) != "45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee":
        raise ValueError("Full audited original-ROM PCM pack required")
    model = a.reference / "wf_audio_sfx.c"
    if sha(model) != "7129466a7d688fcfd2600413b91d4951d80c16de1f411489bbfbfa66fdbc5e6b":
        raise ValueError("Preserved independent A15/A16 command model required")
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "wav").mkdir(exist_ok=True)
    (a.out / "events").mkdir(exist_ok=True)
    source = Path(__file__).with_suffix(".c")
    executable = a.out / "render_sfx_model"
    build = [a.cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror", "-I", str(a.reference),
             "-I", str(a.fixtures), str(source), str(model), "-o", str(executable)]
    subprocess.run(build, check=True, capture_output=True, text=True)
    report = {
        "kind": "A15_COMMAND_MODEL_REFERENCE_NOT_A53_YM_NOT_HARDWARE_CAPTURE",
        "sample_rate": 44100, "channels": 1, "bits": 16,
        "source_sample_rate": 8000, "sample_method": "zero-order hold at exactly 8000 Hz",
        "command_origin": "instantaneous model acceptance at time 0; no invented Z80 command latency",
        "timer_a": "3579545/(64*253) Hz; rational event ordering; timer first on exact ties",
        "oki_volume_numerators_over_32": [32,22,16,11,8,6,4,3,2,0,0,0,0,0,0,0],
        "route_gain": "21627/32768, inherited R04 mix calibration; not original-ROM data",
        "normalization": False,
        "end_rule": "all command events complete and physical PCM tails end, then 11025 silent frames",
        "input_sha256": {str(path): sha(path) for path in [a.golden, model, source, a.fixtures / "sfx_fixture.h"]},
        "build_command": build, "commands": [],
    }
    for command in range(0x20, 0x82):
        stem = f"ARCADE-SFX-MODELO-0x{command:02X}"
        raw = a.out / (stem + ".pcm")
        events = a.out / "events" / (stem + ".csv")
        wav = a.out / "wav" / (stem + ".wav")
        result = subprocess.run([str(executable), str(a.golden), hex(command), str(raw), str(events)],
                                check=True, capture_output=True, text=True)
        item = json.loads(result.stdout)
        data = raw.read_bytes()
        if len(data) != item["frames"] * 2:
            raise ValueError("PCM byte count mismatch")
        with wave.open(str(wav), "wb") as sink:
            sink.setnchannels(1); sink.setsampwidth(2); sink.setframerate(44100)
            sink.writeframes(data)
        item.update(wav=str(wav), wav_sha256=sha(wav), pcm_sha256=hashlib.sha256(data).hexdigest(),
                    events=str(events), events_sha256=sha(events), seconds=item["frames"] / 44100)
        raw.unlink()
        report["commands"].append(item)
    report["complete"] = len(report["commands"]) == 98
    (a.out / "reference-sfx-report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("98 full A15 model SFX WAVs generated; no amplitude normalization")
    print("Total frames:", sum(r["frames"] for r in report["commands"]))


if __name__ == "__main__":
    main()
