#!/usr/bin/env python3
"""Run independent pinned-phase and full original-ROM trace comparison gates."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "traces", "opm", "out"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--production", type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument("--cc", default="cc")
    a = p.parse_args()
    for name in ("rom", "traces", "opm", "out", "production"):
        setattr(a, name, getattr(a, name).resolve())
    expected = {"opm.c": "983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188",
                "opm.h": "186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39"}
    for name, digest in expected.items():
        if sha(a.opm / name) != digest:
            raise ValueError("Pinned A53 dependency mismatch: " + name)
    if sha(a.rom) != "f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7":
        raise ValueError("Original sound ROM mismatch")
    traces = sorted(a.traces.glob("music-*.csv"))
    if len(traces) != 18:
        raise ValueError("Need all 18 independent A53 traces")
    a.out.mkdir(parents=True, exist_ok=True)
    src = a.production / "src"
    tests = a.production / "tests"
    native_sources = [src / "wf_scsp_ym.c", src / "wf_scsp_ym.h", src / "wf_scsp_sfx.h",
                      src / "wf_music_seq.c", src / "wf_music_seq.h",
                      *src.glob("wf_z80_*.inc"), *src.glob("generated/*.h"),
                      *src.glob("reference_driver/z80*.c"), src / "reference_driver/z80.h",
                      tests / "test_ym_native.c", tests / "test_seq_a53.c"]
    before = {str(path): sha(path) for path in native_sources}
    common = [a.cc, "-std=c99", "-O2", "-I", str(src)]
    # Match the selected native profile without changing the independent oracle.
    sequence_object = a.out / "candidate-sequence.o"
    sequence_build = [a.cc, "-std=c99", "-Os", "-I", str(src),
                      "-DWF_MUSIC_NATIVE_CONTROL=1", "-c", str(src / "wf_music_seq.c"),
                      "-o", str(sequence_object)]
    cases = [
        ("ym_phase_and_bridge", common + ["-DWF_TEST_PINNED_OPM", "-I", str(a.opm),
         str(tests / "test_ym_native.c"), str(src / "wf_scsp_ym.c"), "-lm", "-o",
         str(a.out / "test_ym_pinned")], [str(a.out / "test_ym_pinned"), *map(str, traces)]),
        ("serviced_native_sequence_vs_full_z80_a53", common + ["-DWF_MUSIC_NATIVE_CONTROL=1",
         "-I", str(src / "reference_driver"), str(tests / "test_seq_a53.c"),
         str(sequence_object), str(src / "reference_driver/z80-cold.c"),
         str(src / "reference_driver/z80-native.c"), "-o", str(a.out / "test_seq_a53")],
         [str(a.out / "test_seq_a53"), str(a.rom), *map(str, traces)]),
    ]
    report = {"kind": "HOST_PHASE_AND_REGISTER_LOGIC_NOT_NATIVE_WAVEFORM_VALIDATION",
              "source_sha256": before,
              "input_sha256": {str(path): sha(path) for path in [a.rom, *traces]},
              "sequence_build_command": sequence_build, "tests": []}
    subprocess.run(sequence_build, check=True, capture_output=True, text=True)
    for name, build, run in cases:
        subprocess.run(build, check=True, capture_output=True, text=True)
        result = subprocess.run(run, capture_output=True, text=True)
        report["tests"].append({"name": name, "build_command": build, "run_command": run,
                                "exit_code": result.returncode, "stdout": result.stdout, "stderr": result.stderr})
        if result.returncode:
            print(result.stdout + result.stderr)
            break
        print(name + ": PASS")
    after = {str(path): sha(path) for path in native_sources}
    report["source_stable_during_tests"] = before == after
    report["passed"] = len(report["tests"]) == len(cases) and all(t["exit_code"] == 0 for t in report["tests"]) and before == after
    (a.out / "ym-and-sequence-validation.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
