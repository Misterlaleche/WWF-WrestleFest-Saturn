#!/usr/bin/env python3
"""Prove causal RED/GREEN for bounded early service using local source oracles.

The frozen pre-early-service core and scheduler produce the expected RED cases.
The candidate must pass the quota, fragmented-grant and serviced R01 tests.
Input ROM stays external; no private capture paths or embedded ROM are needed.
This is a host semantic proof, not a native-performance acceptance test.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

PACKAGE = Path(__file__).resolve().parents[2]
ROM_SHA256 = "f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7"
PINNED = {
    "tests/oracle/z80_native_pre_early_service.c": "24b844357544e12cfa7b1a09f7d368d895a918b00c129815438827102b1ea0fa",
    "tests/oracle/wf_music_seq_pre_early_service.c": "bdcff61e26282376c6b2e3b2f7921ba389a6b938534e461e197995afef4876a7",
    "tests/oracle/wf_music_seq_r01.c": "5432de051afdae58f5cde12e866fa1bd7241dd98b07a588693a750fc50f10200",
}
PINNED_SUPPORT = {
    "src/reference_driver/z80.c": "d82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698",
    "src/reference_driver/z80-cold.c": "a03ba08502f60da762e712ae7eb52df6e401b09e781fbf64f7bec930b16c24fe",
    "src/wf_music_seq.h": "b02489430f5fb5eb97712c2a56d1636a4159358930cdcbf6244df0cbadf92471",
}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, default=PACKAGE,
                        help="Package containing these tests and frozen oracles")
    parser.add_argument("--candidate-source", type=Path,
                        help="Candidate package; defaults to --package")
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--score-seconds", type=int, default=15)
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()
    package = args.package.resolve()
    candidate = (args.candidate_source or package).resolve()
    rom, out = args.rom.resolve(), args.out.resolve()
    if out.exists() or any(out.is_relative_to(root) for root in (package, candidate)):
        parser.error("--out must be a new directory outside both source packages")
    if not 1 <= args.score_seconds <= 1200:
        parser.error("--score-seconds must be between 1 and 1200")
    if sha(rom) != ROM_SHA256:
        parser.error("Original SOUND.ROM identity differs")
    for relative, expected in PINNED.items():
        if sha(package / relative) != expected:
            parser.error("Frozen oracle identity differs: " + relative)
    for relative, expected in PINNED_SUPPORT.items():
        if sha(candidate / relative) != expected:
            parser.error("Pinned supporting source differs: " + relative)
    test_names = ("quota", "entries", "r01")
    sources = {"oracle:" + path: package / path for path in PINNED}
    sources.update({"candidate:" + path: candidate / path for path in PINNED_SUPPORT})
    for path in ("src/reference_driver/z80-native.c", "src/wf_music_seq.c", "src/reference_driver/z80.h"):
        sources["candidate:" + path] = candidate / path
    for name in test_names:
        path = "tests/test_sh2_early_service_" + name + ".c"
        sources["test:" + path] = package / path
    sources["test:service-audit"] = package / "tests/support/sh2_service_audit.c"
    before = {name: sha(path) for name, path in sources.items()}
    out.mkdir()
    report = {
        "scope": "Causal RED/GREEN and serviced R01 host proof; native acceptance is separate.",
        "rom_sha256": ROM_SHA256, "source_inputs": before, "score_seconds": args.score_seconds,
        "commands": [], "red_observed": {}, "green_passed": False,
    }

    def save() -> None:
        (out / "early-service-proof.json").write_text(json.dumps(report, indent=2) + "\n")

    def run(name: str, command: list, expected: int = 0) -> subprocess.CompletedProcess:
        command = [str(item) for item in command]
        started = time.monotonic()
        result = subprocess.run(command, capture_output=True, text=True)
        log = out / (name + ".log")
        log.write_text(result.stdout + result.stderr)
        report["commands"].append({"name": name, "command": command, "returncode": result.returncode,
                                   "seconds": time.monotonic() - started, "log_sha256": sha(log)})
        save()
        if result.returncode != expected:
            raise RuntimeError(name + " failed; see " + str(log))
        return result

    flags = [args.cc, "-O2", "-std=c11", "-I" + str(candidate / "src"),
             "-I" + str(candidate / "src/reference_driver")]
    ref_names = ["-D" + name + "=wf_ref_" + name for name in
                 ("z80_init", "z80_step", "z80_gen_nmi", "z80_gen_int", "z80_debug_output")]
    common = []
    for name, source, extra in (
        ("original", candidate / "src/reference_driver/z80.c", ref_names),
        ("r01", package / "tests/oracle/wf_music_seq_r01.c", [*ref_names,
            "-Dwf_music_seq_init=wf_ref_init", "-Dwf_music_seq_send=wf_ref_send", "-Dwf_music_seq_run=wf_ref_run"]),
        ("cold", candidate / "src/reference_driver/z80-cold.c", ["-Dz80_step=wf_audit_cold_step"]),
    ):
        obj = out / (name + ".o")
        run("compile-" + name, [*flags, *extra, "-c", source, "-o", obj])
        common.append(obj)
    for stage in ("red", "green"):
        core = (package / "tests/oracle/z80_native_pre_early_service.c" if stage == "red" else
                candidate / "src/reference_driver/z80-native.c")
        seq = (package / "tests/oracle/wf_music_seq_pre_early_service.c" if stage == "red" else
               candidate / "src/wf_music_seq.c")
        core_obj, seq_obj = out / (stage + "-native.o"), out / (stage + "-seq.o")
        run("compile-" + stage + "-core", [*flags, "-Dwf_z80_control_batch=wf_audit_batch_core", "-c", core, "-o", core_obj])
        run("compile-" + stage + "-seq", [*flags, "-DWF_MUSIC_NATIVE_CONTROL=1", "-Os", "-c", seq, "-o", seq_obj])
        for name in test_names:
            binary = out / (stage + "-" + name)
            quota_flag = ["-DWF_EARLY_QUOTA_TEST=1"] if stage == "green" else []
            run("link-" + stage + "-" + name, [*flags, "-Wall", "-Wextra", "-Werror",
                "-DWF_MUSIC_NATIVE_CONTROL=1", *quota_flag,
                package / ("tests/test_sh2_early_service_" + name + ".c"),
                package / "tests/support/sh2_service_audit.c", *common, core_obj, seq_obj, "-o", binary])
            command = [binary, rom]
            if name == "r01":
                command.append(args.score_seconds)
            causal_red = stage == "red" and name != "r01"
            result = run("test-" + stage + "-" + name, command, 2 if causal_red else 0)
            if causal_red:
                marker, count = ("RED 08C1 grant", 38) if name == "quota" else ("RED fragmented grant", 11)
                if result.stderr.count(marker) != count:
                    raise ValueError("Unexpected RED failure: " + name)
                report["red_observed"][name] = count
            elif name == "quota" and "max_actual=106 max_quota=106" not in result.stdout:
                raise ValueError("Quota bound proof absent")
            if name == "r01" and "max_between_callbacks=512 max_grant=512" not in result.stdout:
                raise ValueError("Service-bound audit absent")
    after = {name: sha(path) for name, path in sources.items()}
    if after != before:
        raise ValueError("A source input changed during the proof")
    report.update({"source_stable": True, "green_passed": True, "completed": True})
    save()
    print(json.dumps({"proof": str(out / "early-service-proof.json"), "green_passed": True,
                      "red_observed": report["red_observed"], "source_stable": True}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
