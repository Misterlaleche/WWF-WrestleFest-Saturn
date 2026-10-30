#!/usr/bin/env python3
"""Measure a continuous Royal match from read-only, per-field Beetle telemetry.

The video refresh, the game's completed draws, and host throughput are separate.
Only the first observation of each completed draw is a timing milestone. Boot
and the postmatch presentation are excluded, but the first gameplay interval
is retained. Runs containing a reset require separate segmentation.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def analyze(capture, arcade_reference=57.444852941234):
    capture = Path(capture)
    report = json.loads((capture / "report.json").read_text())
    provenance = json.loads((capture / "provenance.json").read_text())
    assert report["status"] == "PASS", "Capture observation/identity gate failed"
    assert provenance["disc_unchanged"], "Disc changed during capture"
    assert provenance["elf"]["a_bin_matches"], "ELF/disc identity failed"
    assert not report["unmapped_watches"], "Some requested observations are unavailable"
    assert report["video_callbacks"] == report["frames"]
    assert report["duplicate_callbacks"] == 0
    fps = report["video_rate"]
    milestone = {}
    last = None
    previous_draw = 0
    fatal_max = {"fatal_owner": 0, "fatal_dependency": 0}
    scalar_keys = ("emulator_frame", "wp1980_draws", "wp1980_vblanks",
                   "game_frame", "window_misses", "window_sectors",
                   "fatal_owner", "fatal_dependency", "finished_frame",
                   "finished_presentation", "wp1980_started")
    with (capture / "telemetry.csv").open() as stream:
        for raw in csv.DictReader(stream):
            last = {key: int(raw[key]) for key in scalar_keys}
            for key in fatal_max:
                fatal_max[key] = max(fatal_max[key], last[key])
            draw = last["wp1980_draws"]
            assert draw >= previous_draw, "Reset detected: segment the run before measuring"
            previous_draw = draw
            if last["wp1980_started"] and draw:
                milestone.setdefault(draw, last)
    assert last is not None and milestone, "No live gameplay observed"
    count = max(milestone)
    assert set(milestone) == set(range(1, count + 1)), "A completed draw was not sampled"
    assert not any(fatal_max.values()), "Native fatal condition observed"
    first = milestone[1]
    end = milestone[count]

    def window(start, stop):
        a, b = milestone[start], milestone[stop]
        fields = b["emulator_frame"] - a["emulator_frame"]
        counter_fields = b["wp1980_vblanks"] - a["wp1980_vblanks"]
        assert fields == counter_fields, "Native VBlank counter differs from emulator fields"
        assert fields > 0
        return {"first_draw": start, "last_draw": stop,
                "intervals": stop - start, "vblanks": fields,
                "emulated_seconds": fields / fps,
                "updates_per_second": (stop - start) * fps / fields,
                "cd_windows": b["window_misses"] - a["window_misses"]}

    initial = [window(1, stop) for stop in (100, 500, 1000) if stop <= count]
    contiguous = [window(start, min(start + 499, count))
                  for start in range(1, count, 499)]
    sliding = []
    for intervals in (59, 99, 499, 999):
        if count <= intervals:
            continue
        worst = None
        below = 0
        for start in range(1, count - intervals + 1):
            measured = window(start, start + intervals)
            below += measured["updates_per_second"] < arcade_reference
            if worst is None or measured["updates_per_second"] < worst["updates_per_second"]:
                worst = measured
        sliding.append({"intervals_per_window": intervals,
                        "windows_examined": count - intervals,
                        "windows_below_reference": below, "minimum": worst})
    gaps = {}
    for draw in range(2, count + 1):
        gap = milestone[draw]["emulator_frame"] - milestone[draw - 1]["emulator_frame"]
        gaps[str(gap)] = gaps.get(str(gap), 0) + 1

    return {
        "status": "PASS",
        "scope": "Measured emulator cadence; not a physical Saturn or PCB measurement",
        "capture": str(capture),
        "cartridge": report["cartridge"],
        "core_video_hz": fps,
        "arcade_reference_hz": arcade_reference,
        "arcade_reference_qualification": "MAME0.264 original-ROM driver; PCB totals marked guessed",
        "formula": "completed native draw intervals * core_video_hz / elapsed emulator fields",
        "boot_first_gameplay_field": first["emulator_frame"],
        "boot_seconds": first["emulator_frame"] / fps,
        "native_updates": count,
        "complete_match": last["finished_frame"] == count and last["finished_presentation"] == 2,
        "finished_frame": last["finished_frame"],
        "finished_presentation": last["finished_presentation"],
        "whole_match": window(1, count),
        "initial_windows": initial,
        "contiguous_windows": contiguous,
        "sliding_windows": sliding,
        "draw_interval_field_histogram": gaps,
        "combat_cd_windows": end["window_misses"] - first["window_misses"],
        "postmatch_cd_windows": last["window_misses"] - end["window_misses"],
        "fatal_max": fatal_max,
        "elf_sha256": provenance["elf"]["sha256"],
        "disc_files": provenance["disc_files"],
        "input_hashes": {name: sha256(capture / name) for name in
                         ("telemetry.csv", "report.json", "provenance.json")},
        "hardware_test": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = analyze(args.capture)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({key: result[key] for key in
                     ("status", "cartridge", "native_updates", "complete_match",
                      "whole_match", "initial_windows", "sliding_windows",
                      "fatal_max")}, indent=2))


if __name__ == "__main__":
    main()
