#!/usr/bin/env python3
"""Independently analyze completed captures without launching or changing anything.

Reads archived JSON/gzip/HWRAM only. Writes new evidence under --output.
Successful CD-window counters count completed transfers, not failed attempts.
"""
from __future__ import annotations

import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import sys

ACTORS = tuple(f"actor{i}" for i in range(9))
COUNTERS = ("window_misses", "window_sectors")
FIELDS = (
    "video_clock", "game_frame", "wp1980_draws", "finished_frame",
    "finished_presentation", "finished_presentation_vblanks",
    "window_misses", "window_sectors", "wp1979_cdfs_stage",
    "wp1979_cdfs_lba", "wp1979_cdfs_rc", "fatal_owner",
    "fatal_dependency", "boundary_stable", "sample_duration_ns",
)
HWRAM_BASE = 0x06000000


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_info(path):
    p = Path(path)
    h = hashlib.sha256()
    with p.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return {"path": str(p), "bytes": p.stat().st_size, "sha256": h.hexdigest()}


def load(path):
    return json.loads(Path(path).read_text())


def record(sample):
    return {"sample_index": sample["_index"],
            **{k: sample.get(k) for k in FIELDS}}


def actor_bytes(sample):
    records = [bytes.fromhex(sample[k]) for k in ACTORS]
    if any(len(b) != 268 for b in records):
        raise ValueError("All nine actor records must be exactly 268 bytes")
    return records


def actor_comparison(sample, reference):
    observed, expected = actor_bytes(sample), actor_bytes(reference)
    details = {}
    for key, a, b in zip(ACTORS, observed, expected):
        different = [i for i, (x, y) in enumerate(zip(a, b)) if x != y]
        details[key] = {
            "bytes": len(a), "equal": not different,
            "observed_sha256": sha(a), "reference_sha256": sha(b),
            "differing_byte_count": len(different),
            "first_differing_offsets": different[:32],
        }
    return {
        "total_actor_bytes": 9 * 268,
        "all_actor_bytes_equal": observed == expected,
        "rng_equal": sample["rng_state"] == reference["rng_state"],
        "observed_rng": sample["rng_state"],
        "reference_rng": reference["rng_state"],
        "observed_all_actors_sha256": sha(b"".join(observed)),
        "reference_all_actors_sha256": sha(b"".join(expected)),
        "actors": details,
    }


def dump_evidence(capture, report, layout_path):
    metadata_path = capture / "memory-dump.json"
    dump_path = capture / "memory_06000000_saturn.bin"
    if not metadata_path.exists() or not dump_path.exists():
        return {"available": False, "reason": "No completed HWRAM dump available"}
    metadata, data = load(metadata_path), dump_path.read_bytes()
    if len(data) != 0x100000:
        raise ValueError("Unexpected HWRAM dump size")
    if sha(data) != metadata["saturn_sha256"]:
        raise ValueError("HWRAM dump hash differs from capture metadata")
    symbols = load(capture / "symbols.json")
    watches = {w["name"]: w for w in load(capture / "watches.json")}

    def span(address, size):
        physical = address & 0x1FFFFFFF
        offset = physical - HWRAM_BASE
        if offset < 0 or offset + size > len(data):
            raise ValueError("Requested address is outside this HWRAM dump")
        return data[offset:offset + size]

    def number(address, size=4):
        return int.from_bytes(span(address, size), "big")

    last = report["last_sample"]
    terminal = {k: span(watches[k]["address"], 268).hex() == last[k]
                for k in ACTORS}
    terminal["rng_state"] = number(watches["rng_state"]["address"]) == last["rng_state"]
    result = {
        "available": True, "file": file_info(dump_path),
        "metadata": metadata, "atomic": False,
        "report_terminal_fields_match_dump": terminal,
        "all_terminal_fields_match_dump": all(terminal.values()),
        "first_8192_executable_bytes_match_program":
            span(0x06004000, 8192) == (capture / "program.bin").read_bytes()[:8192],
    }
    if not layout_path.exists() or "wf_r18_scsp_queue" not in symbols:
        result["audio"] = {"available": False, "reason": "Audio ABI or queue symbol absent"}
        return result
    abi_record = load(layout_path)
    abi = abi_record["layout"]
    q_address = symbols["wf_r18_scsp_queue"]
    queue = {}
    for key in ("host", "sequence", "last_unimplemented", "head", "tail", "used",
                "submitted", "completed", "unimplemented", "priority_rejected",
                "overflows", "invalid", "sent_at", "pending", "fault"):
        size = 1 if key in ("pending", "fault") else 2 if key in (
            "sequence", "last_unimplemented") else 4
        queue[key] = number(q_address + abi["queue_" + key], size)
    h_address = queue["host"]
    host = {key: number(h_address + abi["host_" + key],
                       1 if key in ("ready", "poisoned") else 4)
            for key in ("mail", "timeout_us", "ready", "poisoned")}
    invariants = {
        "indices_in_range": queue["head"] < 64 and queue["tail"] < 64,
        "used_in_range": queue["used"] <= 64,
        "submitted_minus_completed_equals_used":
            queue["submitted"] - queue["completed"] == queue["used"],
        "acknowledgement_categories_not_overcounted":
            queue["unimplemented"] + queue["priority_rejected"] <= queue["completed"],
    }
    result["audio"] = {
        "available": True, "abi_evidence": file_info(layout_path),
        "queue_address": f"0x{q_address:08X}",
        "queue_bytes": abi["queue_size"],
        "queue_raw_sha256": sha(span(q_address, abi["queue_size"])),
        "queue": queue,
        "acknowledged_ok_count": queue["completed"] - queue["unimplemented"] - queue["priority_rejected"],
        "host_address": f"0x{h_address:08X}", "host": host,
        "host_pointer_matches_elf_symbol":
            (h_address & 0x1FFFFFFF) == (symbols.get("host", -1) & 0x1FFFFFFF),
        "invariants": invariants,
        "mailbox_address": f"0x{host['mail']:08X}",
        "mailbox_contents_available": False,
        "mailbox_limit": "Mailbox is in SCSP sound RAM, outside the captured 0x06000000 HWRAM. A pointer is not mailbox contents.",
        "interpretation_limit": "Non-atomic final emulator host snapshot. Queue acknowledgements are command-transport evidence, not proof of audible output, waveform correctness, synchronization, or physical Saturn sound.",
    }
    for name, count in (("scsp_io", 2), ("scsp_timing", 8)):
        if name in symbols:
            result["audio"][name] = [number(symbols[name] + 4 * i) for i in range(count)]
    return result


def analyze(capture, reference_path, layout_path, expect_zero):
    report_path = capture / "report.json"
    if not report_path.exists():
        return {"status": "PENDING", "capture": str(capture),
                "reason": "Final report not yet available"}, []
    report = load(report_path)
    reference_report = load(reference_path)
    reference, last = reference_report["last_sample"], report["last_sample"]
    samples_path = capture / report.get("samples_file", "samples.jsonl.gz")
    with gzip.open(samples_path, "rt") as f:
        samples = [dict(json.loads(line), _index=i) for i, line in enumerate(f) if line.strip()]
    if not samples:
        raise ValueError("No saved samples")
    stable = [s for s in samples if s.get("boundary_stable") is True]
    pending = [s for s in stable if s.get("finished_frame") == 8474
               and s.get("finished_presentation") in (0, 1)]
    completed = [s for s in stable if s.get("finished_frame") == 8474
                 and s.get("finished_presentation") == 2]
    if not pending or not completed:
        raise ValueError("Need saved stable pending and completed presentation samples")
    first_pending, first_completed = pending[0], completed[0]
    preceding = [s for s in stable if s["_index"] < first_pending["_index"]
                 and s.get("finished_frame") != 8474]
    if not preceding:
        raise ValueError("No stable preterminal anchor; cannot establish the whole result bracket")
    anchor = preceding[-1]
    interval = [s for s in samples if anchor["_index"] <= s["_index"] <= first_completed["_index"]]
    stable_interval = [s for s in interval if s.get("boundary_stable") is True]
    increments = []
    for a, b in zip(stable_interval, stable_interval[1:]):
        delta = {k: b[k] - a[k] for k in COUNTERS}
        if any(delta.values()):
            increments.append({"before": record(a), "after": record(b), "delta": delta})
    clocks = sorted({s["video_clock"] for s in stable_interval})
    missing_clocks = sorted(set(range(clocks[0], clocks[-1] + 1)) - set(clocks))
    phase_delta = {k: first_completed[k] - first_pending[k] for k in COUNTERS}
    bracket_delta = {k: first_completed[k] - anchor[k] for k in COUNTERS}
    terminal = actor_comparison(last, reference)
    terminal_samples = [s for s in stable if s.get("finished_frame") == 8474]
    sample_mismatches = []
    expected_actors = actor_bytes(reference)
    for s in terminal_samples:
        if actor_bytes(s) != expected_actors or s["rng_state"] != reference["rng_state"]:
            sample_mismatches.append(record(s))
    fatal_max = {k: max([s.get(k, 0) for s in samples] + [last.get(k, 0)])
                 for k in ("fatal_owner", "fatal_dependency", "fatal_frame")}
    end_field_matches = {k: last[k] == reference[k] for k in (
        "game_frame", "finished_frame", "finished_result", "finished_count",
        "finished_timer", "boot_seed_observed")}
    all_counter_monotonic = {
        key: all(b[key] >= a[key] for a, b in zip(stable_interval, stable_interval[1:]))
        for key in COUNTERS
    }
    checks = {
        "capture_report_pass": report.get("status") == "PASS",
        "normal_completion": report.get("stop_reason") == "completed_gameover_and_post_fields"
            and report.get("emulator_return_code") == 0,
        "all_nine_complete_actor_records_equal": terminal["all_actor_bytes_equal"],
        "terminal_rng_equal": terminal["rng_equal"],
        "original_terminal_result_fields_equal": all(end_field_matches.values()),
        "result_presentation_completed": last.get("finished_presentation") == 2,
        "zero_fatal_observed": not any(fatal_max.values()),
        "all_stable_terminal_samples_match": not sample_mismatches,
        "CD_counters_monotonic_in_transition_bracket": all(all_counter_monotonic.values()),
        "saved_sample_count_matches_report": len(samples) == report.get("samples_saved"),
    }
    if expect_zero:
        checks["zero_counted_CD_delta_from_preterminal_bracket"] = not any(bracket_delta.values())
        checks["zero_counted_CD_delta_pending_to_completed"] = not any(phase_delta.values())
        checks["no_counted_CD_changes_at_saved_transition_samples"] = not increments
    dump = dump_evidence(capture, report, layout_path)
    if dump.get("available"):
        checks["final_dump_terminal_matches_report"] = dump["all_terminal_fields_match_dump"]
        checks["final_dump_executable_prefix_identity"] = dump["first_8192_executable_bytes_match_program"]
    active = [s for s in stable if s.get("wp1980_started") == 1 and not s.get("finished_frame")]
    capture_sequence = [s["video_clock"] for s in stable]
    result = {
        "schema": "wp1980.hw1.independent-terminal-analysis.v1",
        "status": "PASS" if all(checks.values()) else "FAIL",
        "capture": str(capture), "checks": checks,
        "source_files": {"report": file_info(report_path), "samples": file_info(samples_path),
                         "reference_report": file_info(reference_path),
                         "symbols": file_info(capture / "symbols.json"),
                         "watches": file_info(capture / "watches.json")},
        "terminal_comparison": terminal, "terminal_result_field_matches": end_field_matches,
        "final_native_counters": {k: last.get(k) for k in (
            "game_frame", "finished_frame", "finished_result", "finished_count", "finished_timer",
            "finished_presentation", "finished_presentation_vblanks", "finished_vblanks",
            "video_clock", "start_clock", "wp1980_draws", "wp1980_vblanks")},
        "terminal_stable_samples_checked": len(terminal_samples),
        "terminal_sample_mismatch_count": len(sample_mismatches),
        "terminal_sample_mismatch_examples": sample_mismatches[:12],
        "result_transition": {
            "last_preterminal_stable_anchor": record(anchor),
            "first_pending_stable_sample": record(first_pending),
            "last_pending_stable_sample": record(pending[-1]),
            "first_completed_stable_sample": record(first_completed),
            "pending_presentation_states_observed": sorted({s["finished_presentation"] for s in pending}),
            "pending_stable_samples": len(pending), "completed_stable_samples": len(completed),
            "pending_to_completed_CD_delta": phase_delta,
            "preterminal_anchor_to_completed_CD_delta": bracket_delta,
            "counted_CD_changes": increments,
            "counter_ranges": {k: [min(s[k] for s in stable_interval), max(s[k] for s in stable_interval)] for k in COUNTERS},
            "saved_samples_in_bracket": len(interval),
            "boundary_stable_samples_in_bracket": len(stable_interval),
            "boundary_unstable_samples_in_bracket": len(interval) - len(stable_interval),
            "unique_stable_video_clocks": len(clocks),
            "first_stable_video_clock": clocks[0], "last_stable_video_clock": clocks[-1],
            "missing_integer_video_clocks": missing_clocks,
            "max_observed_stable_clock_gap": max((b - a for a, b in zip(clocks, clocks[1:])), default=0),
            "max_saved_sample_duration_ns": max(s.get("sample_duration_ns", 0) for s in interval),
        },
        "combat_CD_range": {k: [min(s[k] for s in active), max(s[k] for s in active)] for k in COUNTERS} if active else None,
        "fatal_max": fatal_max,
        "sampling": {
            "saved_samples": len(samples), "saved_stable_samples": len(stable),
            "saved_unstable_samples": len(samples) - len(stable),
            "backwards_stable_video_clock_steps": sum(b < a for a, b in zip(capture_sequence, capture_sequence[1:])),
            "observer_stats_from_report": report.get("observer_stats"),
            "all_observed_presentation_states": dict(Counter(s.get("finished_presentation") for s in samples)),
        },
        "final_hwram_dump": dump,
        "limits": [
            "Saved observations are asynchronous and may be deduplicated. Boundary stability is a counter guard, not an atomic snapshot of all memory.",
            "The preterminal bracket can include the final combat quantum and the first result work. It prevents a read before the first observed finished_frame from being silently omitted.",
            "CD windows/sectors are successful-count counters. Zero delta does not prove absence of failed attempts or instrument every low-level CD command.",
            "No new emulation, process-memory writes, save states, production edits, or edits to prior tooling were performed by this analyzer.",
            "Final HWRAM is a live non-atomic emulator dump. It supports a second terminal-state comparison, not physical execution or audible sound claims.",
            "No physical FPS or sound conclusion is inferred. These are the specific captured emulator runs, not arbitrary input routes.",
        ],
    }
    return result, [record(s) for s in interval]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("captures", nargs="+", type=Path)
    parser.add_argument("--reference", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--audio-layout", type=Path,
                        default=Path(__file__).parent / "independent-observation/audio-abi-layout.json")
    parser.add_argument("--expect-zero-result-cd", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    summaries = []
    for capture in args.captures:
        result, samples = analyze(capture.resolve(), args.reference.resolve(),
                                  args.audio_layout.resolve(), args.expect_zero_result_cd)
        result["analyzer"] = file_info(Path(__file__).resolve())
        name = capture.name
        (args.output / (name + "-analysis.json")).write_text(json.dumps(result, indent=2) + "\n")
        if samples:
            (args.output / (name + "-result-samples.json")).write_text(json.dumps(samples, indent=2) + "\n")
        summaries.append({"capture": str(capture), "status": result["status"],
                          "checks": result.get("checks"),
                          "result_CD_delta": result.get("result_transition", {}).get("preterminal_anchor_to_completed_CD_delta")})
    print(json.dumps(summaries, indent=2))
    return 1 if any(s["status"] == "FAIL" for s in summaries) else 2 if any(s["status"] == "PENDING" for s in summaries) else 0


if __name__ == "__main__":
    sys.exit(main())
