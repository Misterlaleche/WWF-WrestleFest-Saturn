#!/usr/bin/env python3
"""Host-only envelope quantization using pinned OPM and selected SCSP equations.

This is a model calibration, not physical hardware validation. It keeps the
original rate values and selects the nearest available native envelope rate.
"""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
OPM_SHA = {"opm.c": "983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188",
           "opm.h": "186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39"}
SCSP_SHA = "0a1e00647022c86c33edfa1601ae7a9fddde45884e34bf4a92015442e04f9906"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def scsp_time(rate, attack):
    # Mathematical transcription of Mednafen 1.32.1 RunEG, scsp.inc:1006,
    # key-on at1474 and amplitude at1637. KRS15 disables native key scale.
    # Deterministic sample-counter phase1, starting attack at0x280 or decay0.
    level = 0x280 if attack else 0
    wait_bits = (24 - min(24, rate)) >> 1
    odd = int(rate < 24 and rate & 1)
    maximum = max(24, min(30, rate))
    for frame in range(1, 44100 * 40):
        toggle = (frame ^ (frame - 1)) & (frame ^ 1)
        if ((toggle | (toggle >> odd)) >> (wait_bits + odd)) & 1:
            shift = ((32 - maximum) >> 1) + (maximum & 1 & (frame >> (wait_bits + 1)))
            increment = (~level if attack else 16) >> shift
            level = max(0, min(1023, level + increment))
        # Relative to native maximum, whose127/128 multiplier is constant.
        amplitude = (127 - (level & 63)) / 127 / (1 << (level >> 6))
        if (attack and amplitude >= .9) or (not attack and level >= 128):
            return frame / 44100
    raise ValueError("Native envelope threshold not reached")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--opm-dir", type=Path, required=True)
    p.add_argument("--scsp-source", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--check-header", type=Path)
    p.add_argument("--write-header", type=Path)
    p.add_argument("--cc", default="cc")
    a = p.parse_args()
    for name, expected in OPM_SHA.items():
        if digest(a.opm_dir / name) != expected:
            raise ValueError("Pinned dependency mismatch: " + name)
    if digest(a.scsp_source) != SCSP_SHA:
        raise ValueError("Need selected, unchanged Mednafen1.32.1 SCSP source")
    a.out.mkdir(parents=True, exist_ok=True)
    exe = a.out / "measure_opm_envelopes"
    command = [a.cc, "-std=c99", "-O3", "-I", str(a.opm_dir),
               str(HERE / "measure_opm_envelopes.c"), "-o", str(exe)]
    subprocess.run(command, check=True)
    result = subprocess.run([str(exe)], check=True, capture_output=True, text=True)
    (a.out / "opm-envelope-core-clocks.csv").write_text(result.stdout)
    rows = list(csv.DictReader(result.stdout.splitlines()))
    native = {kind: [None] + [scsp_time(rate, kind == "attack") for rate in range(1, 32)]
              for kind in ("attack", "decay")}
    maps = {kind: [0, 0] for kind in ("attack", "decay")}
    measurements = []
    for row in rows:
        effective = int(row["effective"])
        item = {"effective_rate": effective}
        for kind in ("attack", "decay"):
            # OPM pipeline runs at half the3,579,545Hz external board clock.
            target = int(row[kind + "_core_clocks"]) * 2 / 3579545
            chosen = min(range(1, 32), key=lambda r: abs(math.log(native[kind][r] / target)))
            maps[kind].append(chosen)
            item[kind] = {"reference_ms": target * 1000, "native_rate": chosen,
                          "native_ms": native[kind][chosen] * 1000,
                          "relative_error": (native[kind][chosen] - target) / target}
        measurements.append(item)
    report = {"kind": "HOST_ENVELOPE_RATE_QUANTIZATION_NOT_PHYSICAL_VALIDATION",
              "opm_clock": "3579545 external Hz; pipeline divider2;32operators",
              "opm_sha256": OPM_SHA, "scsp_source_sha256": SCSP_SHA,
              "model": "selected Mednafen1.32.1 RunEG equations; KRS15;44100Hz",
              "criterion": "minimum absolute log time ratio; attack90percent of maximum amplitude; decay128attenuation units",
              "phase": "OPM zeroed envelope clock state; SCSP first counter1; real attack phase varies",
              "limitations": ["Finite native rates cannot exactly match every original rate or full envelope curve",
                              "KRS15 native maximum attack is slower than OPM instant attack62/63",
                              "No physical Saturn or arcade measurement"],
              "build_command": command, "attack_map": maps["attack"], "decay_map": maps["decay"],
              "native_times_seconds": native, "measurements": measurements}
    text = """/* Board-clock OPM / selected Mednafen1.32.1 envelope-rate quantization.
 * Generated by tools/reference/calibrate_native_envelopes.py.
 * Attack90percent; decay128attenuation units. Model fit, not hardware proof. */
#if defined(__GNUC__)
#define WF_EG_TABLE __attribute__((section(".wf_fm_tables")))
#else
#define WF_EG_TABLE
#endif
"""
    for kind in ("attack", "decay"):
        text += f"static const uint8_t wf_ym_{kind}_rate[64] WF_EG_TABLE={{\n "
        text += ",".join(map(str, maps[kind])) + "\n};\n"
    text += "#undef WF_EG_TABLE\n"
    (a.out / "wf_ym_envelope.candidate.h").write_text(text)
    report["candidate_header_sha256"] = hashlib.sha256(text.encode()).hexdigest()
    failed = False
    if a.check_header:
        current = a.check_header.read_text()
        differences = []
        for kind in ("attack", "decay"):
            match = re.search(r"wf_ym_" + kind + r"_rate\[64\].*?\{(.*?)\}", current, re.S)
            actual = list(map(int, re.findall(r"\d+", match.group(1)))) if match else []
            differences += [{"kind": kind, "effective_rate": i, "actual": actual[i] if i < len(actual) else None,
                             "expected_nearest_model_rate": expected}
                            for i, expected in enumerate(maps[kind]) if i >= len(actual) or actual[i] != expected]
        report["header_check"] = {"passed": not differences, "differences": differences}
        failed = bool(differences)
    if a.write_header:
        a.write_header.write_text(text)
    (a.out / "envelope-calibration.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"attack_map": maps["attack"], "decay_map": maps["decay"],
                      "check_passed": not failed}, indent=2))
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
