#!/usr/bin/env python3
"""Causal 4 KiB neighbor-prefetch model; no game changes or FPS prediction.

Input: big-endian (u32 frame, u16 count, count*u16 tile IDs), one record per
native frame. Every visible tile is a page demand, including VDP1-cache hits.
Consequently this is an isolated decoded-page model, not a prediction of the
production loader's exact reads. Adjacent repeats of the same page are elided;
that does not change LRU ordering or misses.

After all demands in a frame, newly visible tile IDs (absent in the preceding
frame) nominate neighboring pages in original draw order. A bounded FIFO keeps
nonresident, nonduplicate nominees. At most one page completes at that frame's
end, inserted as MRU. There is no oracle, frame lookahead, or same-frame rescue.
"""
import argparse
from collections import Counter, OrderedDict
import hashlib
import json
from pathlib import Path
import struct

PAGE_BYTES = 4096
PAGE_COUNT = 2048
TILES_PER_PAGE = 32
QUEUE_CAPACITY = 64
POLICIES = {"plus": (1,), "minus": (-1,),
            "plus_minus": (1, -1), "minus_plus": (-1, 1)}


def parse(data):
    frames, at, previous = [], 0, -1
    while at < len(data):
        if len(data) - at < 6:
            raise ValueError("truncated frame header")
        frame, count = struct.unpack_from(">IH", data, at)
        at += 6
        if frame <= previous or count > 256 or len(data) - at < count * 2:
            raise ValueError("nonmonotonic frame, excessive count, or truncated IDs")
        ids = struct.unpack_from(">" + str(count) + "H", data, at)
        at += count * 2
        if len(set(ids)) != count:
            raise ValueError("duplicate visible tile ID")
        frames.append((frame, ids))
        previous = frame
    return frames


def prepare(frames):
    """All derived inputs use the current and previous frame only."""
    prepared, previous = [], set()
    for frame, ids in frames:
        visits, origins, seen_origins = [], [], set()
        for tile in ids:
            page = tile // TILES_PER_PAGE
            if not visits or visits[-1] != page:
                visits.append(page)
            if tile not in previous and page not in seen_origins:
                seen_origins.add(page)
                origins.append(page)
        prepared.append((frame, tuple(visits), tuple(origins)))
        previous = set(ids)
    return prepared


def distribution(values):
    if not values:
        return {"count": 0, "min": None, "p50": None, "p95": None,
                "p99": None, "max": None}
    ordered = sorted(values)
    def percentile(n):
        return ordered[max(0, (len(ordered) * n + 99) // 100 - 1)]
    return {"count": len(values), "min": ordered[0], "p50": percentile(50),
            "p95": percentile(95), "p99": percentile(99), "max": ordered[-1]}


def simulate(frames, capacity, offsets=(), queue_capacity=QUEUE_CAPACITY):
    if capacity < 1 or queue_capacity < 1:
        raise ValueError("cache and queue must have positive capacity")
    cache, queue = OrderedDict(), OrderedDict()
    counts, miss_events, decisions = Counter(), [], []
    miss_histogram, leads, queue_ages = [], [], []
    evicted_unused = 0

    def insert(page, speculative_frame):
        nonlocal evicted_unused
        if len(cache) == capacity:
            _, pending_use = cache.popitem(last=False)
            if pending_use is not None:
                evicted_unused += 1
        cache[page] = speculative_frame

    for frame, visits, origins in frames:
        missed = set()
        for index, page in enumerate(visits):
            if page in queue:
                del queue[page]
                counts["queued_candidates_reached_by_demand"] += 1
            if page not in cache:
                missed.add(index)
                insert(page, None)
            else:
                loaded_frame = cache[page]
                if loaded_frame is not None:
                    counts["useful_prefetches"] += 1
                    leads.append(frame - loaded_frame)
                    cache[page] = None
                cache.move_to_end(page)
        for origin in origins:
            for offset in offsets:
                candidate = origin + offset
                if not 0 <= candidate < PAGE_COUNT:
                    counts["candidate_out_of_bounds"] += 1
                elif candidate in cache:
                    counts["candidate_already_resident"] += 1
                elif candidate in queue:
                    counts["candidate_already_queued"] += 1
                elif len(queue) == queue_capacity:
                    counts["candidate_queue_full"] += 1
                else:
                    queue[candidate] = frame
                    counts["candidates_enqueued"] += 1
                    counts["queue_peak"] = max(counts["queue_peak"], len(queue))
        prefetched = None
        while queue:
            page, queued_frame = queue.popitem(last=False)
            if page in cache:
                counts["queued_candidates_became_resident"] += 1
                continue
            insert(page, frame)
            prefetched = page
            queue_ages.append(frame - queued_frame)
            counts["prefetch_loads"] += 1
            break  # The entire frame's speculative budget is one page.
        miss_events.append(missed)
        miss_histogram.append(len(missed))
        decisions.append((frame, tuple(sorted(missed)), prefetched))
        assert len(cache) <= capacity and len(queue) <= queue_capacity
    unused = sum(value is not None for value in cache.values())
    assert counts["prefetch_loads"] == counts["useful_prefetches"] + evicted_unused + unused
    assert not leads or min(leads) >= 1
    misses = sum(miss_histogram)
    result = {
        "demand_misses": misses,
        "demand_frames_with_misses": sum(bool(v) for v in miss_histogram),
        "demand_misses_per_frame": distribution(miss_histogram),
        "demand_miss_histogram": dict(sorted(Counter(miss_histogram).items())),
        "prefetch_loads": counts["prefetch_loads"],
        "useful_prefetches": counts["useful_prefetches"],
        "prefetches_evicted_unused": evicted_unused,
        "prefetches_resident_unused_at_end": unused,
        "useful_prefetch_percent": round(100 * counts["useful_prefetches"] /
                                          counts["prefetch_loads"], 4) if counts["prefetch_loads"] else None,
        "prefetch_lead_frames": distribution(leads),
        "candidate_age_at_load_frames": distribution(queue_ages),
        "demand_decoded_bytes": misses * PAGE_BYTES,
        "prefetch_decoded_bytes": counts["prefetch_loads"] * PAGE_BYTES,
        "total_decoded_pages": misses + counts["prefetch_loads"],
        "total_decoded_bytes": (misses + counts["prefetch_loads"]) * PAGE_BYTES,
        "max_total_decodes_in_frame": max((len(m) + (p is not None)
                for _, m, p in decisions), default=0),
        "candidate_counters": dict(counts),
        "candidates_pending_at_end": len(queue),
        "peak_demand_frames": [{"frame": frames[i][0], "demand_misses": value}
                for i, value in sorted(enumerate(miss_histogram),
                                       key=lambda pair: (-pair[1], pair[0]))[:10]],
        "decision_sha256": hashlib.sha256(json.dumps(decisions,
                separators=(",", ":")).encode()).hexdigest(),
    }
    return result, miss_events, decisions


def validate_lease(base, capacity, protected=()):
    end = base + capacity * PAGE_BYTES
    if base % PAGE_BYTES or capacity < 1 or end > 0x100000000:
        raise ValueError("unaligned or overflowing decoded-page lease")
    for low, high in protected:
        if not 0 <= low <= high <= 0x100000000 or base < high and low < end:
            raise ValueError("decoded-page lease overlaps another owner")
    return {"base": hex(base), "end_exclusive": hex(end),
            "bytes": end - base, "pages": capacity}


def self_test():
    frames = prepare([(0, (32,)), (1, (64,)), (2, (64,))])
    baseline, _, _ = simulate(frames, 2)
    prediction, _, decisions = simulate(frames, 2, (1,))
    assert baseline["demand_misses"] == 2
    assert prediction["demand_misses"] == 1 and prediction["useful_prefetches"] == 1
    assert decisions[0] == (0, (0,), 2)  # Current page still misses.
    # Predictions are identical for every common prefix, regardless of suffix.
    alternative = prepare([(0, (32,)), (1, (65535,)), (2, (0,))])
    assert simulate(alternative, 2, (1,))[2][:1] == decisions[:1]
    for length in range(len(frames) + 1):
        assert simulate(frames[:length], 2, (1,))[2] == decisions[:length]
    resident, _, _ = simulate(prepare([(0, (32, 64)), (1, (32, 64))]), 2, (1,))
    assert resident["prefetch_loads"] == 1  # Page 2 is skipped; page 3 is nominated.
    edge, _, _ = simulate(prepare([(0, (0, 65535))]), 2, (1, -1))
    assert edge["candidate_counters"]["candidate_out_of_bounds"] == 2
    bounded, _, _ = simulate(prepare([(0, tuple(range(0, 1024, 64)))]), 32, (1, -1), 2)
    assert bounded["candidate_counters"]["queue_peak"] == 2
    assert bounded["candidate_counters"]["candidate_queue_full"] > 0
    assert bounded["prefetch_loads"] == 1
    validate_lease(0x272000, 130, ((0x2f4000, 0x300000),))
    for args in ((0x272000, 142, ((0x2fc000, 0x300000),)),
                 (0xfffff000, 2, ()), (1, 1, ())):
        try:
            validate_lease(*args)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid memory lease accepted")
    for bad in (b"\0", struct.pack(">IH", 0, 1),
                struct.pack(">IHHH", 0, 2, 1, 1),
                struct.pack(">IHIH", 1, 0, 0, 0)):
        try:
            parse(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("malformed trace accepted")
    return {"passed": True, "checks": ["no future-suffix dependence",
        "same-frame misses remain demanded", "one-page budget", "resident rejection",
        "bounded FIFO", "page bounds", "useful/unused accounting",
        "protected memory leases", "truncated/duplicate/nonmonotonic trace rejection"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = {
        "description": "Causal decoded-page model; not measured Saturn performance",
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "self_tests": self_test(),
        "model": {"page_bytes": PAGE_BYTES, "page_count": PAGE_COUNT,
            "tile_bytes": 128, "cache_policy": "LRU; demand and prefetch insert MRU",
            "cache_start": "empty", "queue_capacity": QUEUE_CAPACITY,
            "candidate_origin": "newly visible tile IDs versus preceding recorded frame; unique source pages in draw order",
            "candidate_queue": "FIFO; reject out of range, resident, queued, then full; cancel upon demand",
            "prefetch_budget_pages_per_frame": 1,
            "prefetch_completion": "end of current recorded frame after every demand",
            "limitations": [
                "All visible tile IDs are demands, even if a future VDP1 texture cache could serve them.",
                "No frontend scene access, initial HOTSPR preload, CD traffic, CPU contention, latency, or asynchronous cancellation is modeled.",
                "Completion before the next recorded frame is an assumption, not a measured 12 ms decode guarantee.",
                "No future trace data influences decisions; paired baseline comparisons are accounting only."]},
        "memory_contract": {
            "direct_142": validate_lease(0x272000, 142),
            "frontend_130": validate_lease(0x272000, 130, ((0x2f4000, 0x300000),)),
            "direct_142_condition": "frontend raw RAM and cue08 prefix must not be live in this lease",
            "frontend_protected": [{"base": "0x2f4000", "end_exclusive": "0x2fc000", "owner": "cue08 prefix"},
                                   {"base": "0x2fc000", "end_exclusive": "0x300000", "owner": "original frontend RAM"}],
            "bounded_metadata_arrays_minimum_bytes": 256 * 2 + QUEUE_CAPACITY * 2,
            "metadata_note": "Previous 256 u16 IDs + 64 u16 candidates; counters, synchronization, lookup and stack are additional.",
            "implementation_guards_required": [
                "No page may be replaced while any caller still borrows its tile pointer.",
                "A background decode must own its destination slot until validation and publication; a pending slot is unavailable to demand replacement.",
                "The current loader has one shared z_stream and 8192-byte inflate storage; concurrent decoding requires separately reserved state and stack, or serialized resumable work.",
                "Do not publish partially decoded data, share mutable LRU state unsynchronized, overlap ROM/SFX/prefix/scene/raw-RAM leases, or count an in-flight slot twice.",
                "This script reserves no production memory and enables no prefetch or DualCPU code."]},
        "modes": {},
    }
    for mode in ("tag", "royal"):
        path = args.directory / (mode + "-tiles.bin")
        data = path.read_bytes()
        raw = parse(data)
        frames = prepare(raw)
        row = {"trace": str(path), "trace_sha256": hashlib.sha256(data).hexdigest(),
            "trace_bytes": len(data), "frames": len(raw),
            "first_frame": raw[0][0], "last_frame": raw[-1][0],
            "unrecorded_frame_gaps": raw[-1][0] - raw[0][0] + 1 - len(raw),
            "tile_references": sum(len(ids) for _, ids in raw),
            "distinct_tiles": len({tile for _, ids in raw for tile in ids}),
            "distinct_pages": len({tile // TILES_PER_PAGE for _, ids in raw for tile in ids}),
            "page_visits_after_adjacent_repeat_elision": sum(len(v) for _, v, _ in frames),
            "capacities": {}}
        for capacity in (142, 130):
            baseline, baseline_misses, _ = simulate(frames, capacity)
            models = {"baseline": baseline}
            for name, offsets in POLICIES.items():
                model, misses, _ = simulate(frames, capacity, offsets)
                avoided = sum(len(a - b) for a, b in zip(baseline_misses, misses))
                extra = sum(len(b - a) for a, b in zip(baseline_misses, misses))
                net = baseline["demand_misses"] - model["demand_misses"]
                assert net == avoided - extra
                model["comparison_to_baseline"] = {
                    "miss_events_avoided": avoided, "extra_miss_events": extra,
                    "net_demand_misses_avoided": net,
                    "net_demand_miss_reduction_percent": round(100 * net / baseline["demand_misses"], 4),
                    "additional_total_decoded_bytes": model["total_decoded_bytes"] - baseline["total_decoded_bytes"],
                    "total_decode_change_percent": round(100 * (model["total_decoded_pages"] -
                        baseline["total_decoded_pages"]) / baseline["total_decoded_pages"], 4)}
                models[name] = model
            row["capacities"][str(capacity)] = models
            print(mode, capacity, {k: {"misses": v["demand_misses"],
                "prefetch": v["prefetch_loads"], "useful": v["useful_prefetches"],
                "decoded_bytes": v["total_decoded_bytes"]} for k, v in models.items()})
        report["modes"][mode] = row
    target = args.output or args.directory / "neighbor-prefetch.json"
    target.write_text(json.dumps(report, indent=2) + "\n")
    print(target)


if __name__ == "__main__":
    main()
