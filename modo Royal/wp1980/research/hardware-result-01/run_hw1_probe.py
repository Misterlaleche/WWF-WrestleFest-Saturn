#!/usr/bin/env python3
"""Reproduce the HW1 result probes using explicit roots and preserved fixtures.

All outputs go into a new directory. This runner never reads a temporary
emulator capture, invokes an emulator, or builds a Saturn release image.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROM_SHA = "265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306"
INIT_SHA = "40c53836ca6591f9bdc3cfcc879fda054ffc43f0d168ac02733b93d0d83cefbb"
REPORT_SHA = "10f6dc6cf94d374d568145d467b6577047e4382a7c9002491fc6a4665cf4fbd2"
ACTORS_SHA = "656f7934c1c1b1cbac5afef25e4d17c8503c050c011effc734a0e00e1fb6f884"
RESULT_SOURCES = (
    "wf_r18_present.c", "wf_r18_result_screen.c", "wf_r18_frontend_palette.c",
    "wf_r18_video.c", "wf_r17_m68k_shadow.c", "wf_r18_metasprite.c",
    "wf_saturn_yaul_backend.c", "wf_saturn_memory.c", "wf_r18_hud.c",
    "wf_r18_runtime_handoff.c",
)
MUSASHI_SOURCES = ("m68kcpu.c", "m68kops.c", "m68kopac.c", "m68kopdm.c", "m68kopnz.c")


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def fixture(original_root):
    manifest = json.loads((HERE / "fixtures/portable-fixture.json").read_text())
    report = HERE / manifest["report"]
    actors = HERE / manifest["actors"]
    require(sha(report) == REPORT_SHA == manifest["report_sha256"], "Terminal report hash mismatch")
    require(sha(actors) == ACTORS_SHA == manifest["actors_sha256"], "Terminal actor hash mismatch")
    last = json.loads(report.read_text())["last_sample"]
    report_actors = b"".join(bytes.fromhex(last[f"actor{i}"]) for i in range(9))
    require(report_actors == actors.read_bytes() and len(report_actors) == 9 * 0x10C,
            "Preserved actor bytes do not match the terminal report")
    require(last["game_frame"] == 8474 and last["rng_state"] == 0xB8A86C74,
            "Terminal frame or RNG mismatch")
    rom = original_root / "native/audit/wp1980-release-cd/ROM68K.BIN"
    initial = original_root / "native/audit/wp1980-release-cd/ROYINIT.BIN"
    require(sha(rom) == ROM_SHA, "Original ROM hash mismatch")
    require(sha(initial) == INIT_SHA, "Original ROYINIT hash mismatch")
    return rom, initial, actors, report, manifest


def hashes(paths):
    return {str(path): sha(path) for path in paths}


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    commands = p.add_subparsers(dest="kind", required=True)
    for kind in ("result", "provenance", "oracle", "check"):
        sub = commands.add_parser(kind)
        sub.add_argument("--original-root", type=Path, required=True,
                         help="Project root containing original native/audit and reference/musashi")
        sub.add_argument("--out", type=Path, required=True, help="New output directory; must not exist")
        sub.add_argument("--cc", default="cc", help="Host C compiler executable")
        if kind != "oracle":
            sub.add_argument("--source-root", type=Path, required=True,
                             help="Project root containing the native/src and native/include to test")
        if kind == "result":
            sub.add_argument("--variant", choices=("base", "cart"), required=True)
            sub.add_argument("--capacity", type=int, required=True)
            sub.add_argument("--hot-header", type=int, default=4096)
            sub.add_argument("--mode", choices=("baseline", "deny", "churn"), required=True)
            sub.add_argument("--reference", type=Path, help="Native snapshot made by the same host/compiler")
            sub.add_argument("--board-reference", type=Path, help="Portable original-68000 board projection")
    return p


def run_one(args):
    original = args.original_root.resolve(strict=True)
    rom, initial, actors, report, manifest = fixture(original)
    out = args.out.resolve()
    require(not out.exists(), f"Refusing to overwrite existing output: {out}")
    if args.kind == "result" and args.mode != "baseline":
        require(args.reference is not None, "Denied-read modes require --reference")
    reference = args.reference.resolve(strict=True) if getattr(args, "reference", None) else None
    board_reference = args.board_reference.resolve(strict=True) if getattr(args, "board_reference", None) else None
    source_root = args.source_root.resolve(strict=True) if hasattr(args, "source_root") else original
    native = source_root / "native"
    if args.kind == "oracle":
        probe = HERE / "original_result_oracle.c"
        includes = original / "reference/musashi"
        sources = [includes / name for name in MUSASHI_SOURCES]
        flags = ["-O2", f"-I{includes}"]
    else:
        probe = HERE / ("result_residency_probe.c" if args.kind == "result" else "palette_provenance_probe.c")
        includes = native / "include"
        names = RESULT_SOURCES if args.kind == "result" else ("wf_r18_frontend_palette.c", "wf_r17_m68k_shadow.c")
        sources = [native / "src" / name for name in names]
        flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-g", f"-I{includes}"]
        if args.kind == "result":
            flags += ["-ffunction-sections", "-fdata-sections",
                      f"-DWP1980_CART={int(args.variant == 'cart')}",
                      f"-DWF_R18_ROM_EXTRA_CAPACITY={args.capacity}u",
                      f"-DWP1980_HOT_HEADER_BYTES={args.hot_header}u"]
    header_files = sorted(includes.rglob("*.h"))
    require(header_files, f"No source headers found: {includes}")
    input_files = sources + header_files + [probe, rom, initial, actors, report]
    before = hashes(input_files)
    out.mkdir(parents=True)
    local_probe = out / "probe-source.c"
    local_probe.write_bytes(probe.read_bytes())
    local_actors = out / "terminal-actors.bin"
    local_actors.write_bytes(actors.read_bytes())
    binary = out / f"{args.kind}-probe"
    command = [args.cc, *flags, str(local_probe), *map(str, sources)]
    if args.kind == "result":
        command.append("-Wl,--gc-sections")
    command += ["-o", str(binary)]
    write_json(out / "build-command.json", command)
    (out / "build-command.txt").write_text(shlex.join(command) + "\n")
    built = subprocess.run(command, text=True, capture_output=True)
    (out / "build.log").write_text(built.stdout + built.stderr)
    evidence = {"kind": args.kind, "original_root": str(original), "source_root": str(source_root),
                "wrapper_sha256": sha(__file__), "build_exit_code": built.returncode,
                "input_sha256": before, "fixture_scope": manifest["scope"]}
    if built.returncode:
        write_json(out / "evidence.json", evidence)
        sys.stderr.write(built.stdout + built.stderr)
        return built.returncode
    if args.kind == "result":
        snapshot = out / "result-state.bin"
        run = [str(binary), str(rom), str(initial), str(local_actors), args.mode, str(snapshot)]
        if reference:
            run.append(str(reference))
    elif args.kind == "provenance":
        run = [str(binary), str(rom)]
    else:
        run = [str(binary), str(rom), str(initial), str(local_actors), str(out / "original-board-state.bin")]
    write_json(out / "run-command.json", run)
    (out / "run-command.txt").write_text(shlex.join(run) + "\n")
    executed = subprocess.run(run, text=True, capture_output=True)
    (out / "stdout.jsonl").write_text(executed.stdout)
    (out / "stderr.txt").write_text(executed.stderr)
    metrics = [json.loads(line) for line in executed.stdout.splitlines() if line.strip()]
    stable = before == hashes(input_files)
    evidence.update(run_exit_code=executed.returncode, metrics=metrics,
                    inputs_unchanged_during_run=stable, binary_sha256=sha(binary))
    exit_code = executed.returncode
    if not stable:
        exit_code = 2
    if args.kind == "result":
        evidence["snapshot_sha256"] = sha(snapshot)
        evidence["snapshot_bytes"] = snapshot.stat().st_size
        if reference:
            evidence["native_reference_sha256"] = sha(reference)
            evidence["native_reference_equal"] = snapshot.read_bytes() == reference.read_bytes()
        if board_reference:
            board = out / "result-state.bin.board.bin"
            equal = board.read_bytes() == board_reference.read_bytes()
            evidence["board_comparison"] = {"exact": equal, "bytes": board.stat().st_size,
                                            "native_sha256": sha(board), "original_sha256": sha(board_reference)}
            if not equal:
                exit_code = 1
    evidence["exit_code"] = exit_code
    write_json(out / "evidence.json", evidence)
    sys.stdout.write(executed.stdout)
    sys.stderr.write(executed.stderr)
    return exit_code


def check(args):
    """Run each documented contract once, preserving genuine RED exit codes."""
    out = args.out.resolve()
    require(not out.exists(), f"Refusing to overwrite existing output: {out}")
    original = args.original_root.resolve(strict=True)
    fixed = args.source_root.resolve(strict=True)
    fixture(original)
    out.mkdir(parents=True)
    cases = []

    def invoke(name, kind, source=None, extra=(), expected=0):
        command = [sys.executable, str(Path(__file__).resolve()), kind,
                   "--original-root", str(original), "--out", str(out / name), "--cc", args.cc]
        if source:
            command += ["--source-root", str(source)]
        command += list(map(str, extra))
        completed = subprocess.run(command, text=True, capture_output=True)
        evidence_path = out / name / "evidence.json"
        evidence = json.loads(evidence_path.read_text()) if evidence_path.exists() else {}
        good = (completed.returncode == expected and evidence.get("build_exit_code") == 0 and
                evidence.get("run_exit_code") == expected and evidence.get("inputs_unchanged_during_run"))
        metrics = evidence.get("metrics", [])
        if kind == "result" and good:
            m = metrics[0]
            good = bool(m["actors_equal"])
            if expected:
                good &= m["status"] == 1 and m["first_failed_offset"] == 0x2800
                good &= m["commits"] == m["wait_calls"] == 0 and m["failed_reads"] == 1
            else:
                good &= m["status"] == 0 and m["commits"] == 2 and m["wait_periods"] == 97
                good &= m["events"] == "HCLCW" and m["original_values_equal"] and m["snapshot_equal"]
                good &= m["result_pages"] == [10, 11, 14, 17, 18, 148, 151] and m["result_page_bytes_equal"]
                if m["mode"] != "baseline":
                    good &= m["post_preload_result_reads"] == 0 and m["failed_reads"] == 0
                if m["mode"] == "churn":
                    good &= m["churn_pages"] == 512 - m["capacity"] and m["churn_lines"] == 64
                good &= evidence.get("board_comparison", {}).get("exact", False)
        if kind == "provenance" and good:
            good = len(metrics) == 2
            for row, address in zip(metrics, (0x2A98, 0x3922)):
                good &= row["status"] == 1 and row["expected_address"] == address
                good &= row["actual_address"] == (0 if expected else address)
        row = {"case": name, "expected_exit_code": expected, "observed_exit_code": completed.returncode,
               "contract_observed": bool(good), "command": command}
        cases.append(row)
        write_json(out / "check-summary.json", {"pass": all(x["contract_observed"] for x in cases), "cases": cases})
        print(json.dumps({k: v for k, v in row.items() if k != "command"}), flush=True)
        if not good:
            sys.stderr.write(completed.stdout + completed.stderr)
        require(good, f"Contract failed: {name}; inspect {out / name}")

    invoke("oracle", "oracle")
    board = out / "oracle/original-board-state.bin"
    for variant, old_capacity, fixed_capacity in (("base", 32, 39), ("cart", 128, 135)):
        baseline = f"original-{variant}-reference"
        common = ["--variant", variant, "--hot-header", "4096"]
        invoke(baseline, "result", original, common + ["--capacity", old_capacity, "--mode", "baseline", "--board-reference", board])
        reference = out / baseline / "result-state.bin"
        for mode in ("deny", "churn"):
            invoke(f"original-{variant}-{mode}", "result", original,
                   common + ["--capacity", old_capacity, "--mode", mode, "--reference", reference], expected=1)
            invoke(f"fixed-{variant}-{mode}", "result", fixed,
                   common + ["--capacity", fixed_capacity, "--mode", mode, "--reference", reference,
                             "--board-reference", board])
    invoke("provenance-original", "provenance", original, expected=1)
    invoke("provenance-fixed", "provenance", fixed)
    return 0


def main():
    args = parser().parse_args()
    try:
        return check(args) if args.kind == "check" else run_one(args)
    except (OSError, ValueError, KeyError) as exc:
        print(f"SETUP_OR_CONTRACT_ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
