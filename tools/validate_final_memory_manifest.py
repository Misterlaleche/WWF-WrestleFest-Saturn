#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

CAP = {"vdp1": 0x80000, "vdp2": 0x80000, "cram": 0x1000, "scsp": 0x80000}
HWRAM_BASE = 0x06000000
HWRAM_END = 0x06100000
LWRAM_RANGES = ((0x00200000, 0x00300000), (0x20200000, 0x20300000))
HWRAM_RANGES = ((0x06000000, 0x06100000), (0x26000000, 0x26100000))
CDFS_CAP = 384
REQUIRED_REGION_FIELDS = ("name", "owner", "start", "bytes", "scenes", "lifetime", "alignment", "source_workpack")

def num(v):
    if v is None:
        return None
    if isinstance(v, bool):
        raise ValueError("boolean is not numeric")
    return int(v, 0) if isinstance(v, str) else int(v)

def in_ranges(addr, length, ranges):
    if addr is None or length is None or length <= 0:
        return False
    end = addr + length
    return any(lo <= addr and end <= hi for lo, hi in ranges)

def overlaps(a0, alen, b0, blen):
    return max(a0, b0) < min(a0 + alen, b0 + blen)

def simultaneous(a, b):
    sa = set(a.get("scenes", ["ALL"]))
    sb = set(b.get("scenes", ["ALL"]))
    return "ALL" in sa or "ALL" in sb or bool(sa & sb)

def add(errors, msg):
    errors.append(msg)

def validate_region(space, r, idx, errors):
    tag = f"{space}[{idx}]"
    for key in REQUIRED_REGION_FIELDS:
        if key not in r:
            add(errors, f"{tag}: missing {key}")
    try:
        start = num(r.get("start"))
        size = num(r.get("bytes"))
        align = num(r.get("alignment"))
    except Exception as e:
        add(errors, f"{tag}: invalid numeric field: {e}")
        return None
    if start is None or size is None or align is None:
        add(errors, f"{tag}: null start/bytes/alignment")
        return None
    if start < 0 or size <= 0 or start + size > CAP[space]:
        add(errors, f"{tag}: out of bounds")
    if align <= 0 or (align & (align - 1)) != 0:
        add(errors, f"{tag}: alignment must be power-of-two")
    elif start % align:
        add(errors, f"{tag}: start not aligned to {align}")
    scenes = r.get("scenes")
    if not isinstance(scenes, list) or not scenes or not all(isinstance(x, str) and x for x in scenes):
        add(errors, f"{tag}: scenes must be non-empty string list")
    if not isinstance(r.get("owner"), str) or not r.get("owner"):
        add(errors, f"{tag}: invalid owner")
    if not isinstance(r.get("lifetime"), str) or not r.get("lifetime"):
        add(errors, f"{tag}: invalid lifetime")
    if not isinstance(r.get("source_workpack"), str) or not r.get("source_workpack"):
        add(errors, f"{tag}: invalid source_workpack")
    return start, size

def validate(m, final=False):
    errors = []
    blockers = []
    for key in ("manifest_version", "hwram", "vdp1", "vdp2", "cram", "scsp", "cdfs_entries", "dma"):
        if key not in m:
            add(errors, f"missing top-level {key}")

    h = m.get("hwram", {})
    for key in ("link_end", "static_end", "scene_peaks"):
        if key not in h:
            add(errors, f"missing hwram.{key}")
    link_end = num(h.get("link_end")) if "link_end" in h else None
    static_end = num(h.get("static_end")) if "static_end" in h else None
    if link_end is None or static_end is None:
        blockers.append("HWRAM_LINK_MAP_OPEN")
    else:
        if not (HWRAM_BASE <= link_end <= HWRAM_END):
            add(errors, "hwram.link_end out of range")
        if not (link_end <= static_end <= HWRAM_END):
            add(errors, "hwram.static_end invalid")
    peaks = h.get("scene_peaks", {})
    if not isinstance(peaks, dict):
        add(errors, "hwram.scene_peaks must be object")
    elif static_end is not None:
        for scene, peak_v in peaks.items():
            peak = num(peak_v)
            if peak is None or peak < 0:
                add(errors, f"hwram.scene_peaks.{scene} invalid")
            elif static_end + peak > HWRAM_END:
                add(errors, f"hwram.scene_peaks.{scene} exceeds HWRAM")
    if not peaks:
        blockers.append("HWRAM_SCENE_PEAKS_OPEN")

    for space in CAP:
        node = m.get(space)
        if not isinstance(node, dict):
            add(errors, f"{space} must be object")
            continue
        regs = node.get("regions")
        if not isinstance(regs, list):
            add(errors, f"{space}.regions must be list")
            continue
        parsed = []
        for i, r in enumerate(regs):
            if not isinstance(r, dict):
                add(errors, f"{space}[{i}] must be object")
                continue
            p = validate_region(space, r, i, errors)
            if p:
                parsed.append((i, r, p[0], p[1]))
        for pos, (i, a, a0, alen) in enumerate(parsed):
            for j, b, b0, blen in parsed[pos + 1:]:
                if simultaneous(a, b) and overlaps(a0, alen, b0, blen):
                    add(errors, f"{space}: simultaneous overlap {a.get('name')} / {b.get('name')}")

    vdp1 = m.get("vdp1", {})
    if vdp1.get("final_partition_status") != "CLOSED":
        blockers.append("VDP1_FINAL_PARTITION_OPEN")

    entries = num(m.get("cdfs_entries")) if m.get("cdfs_entries") is not None else None
    if entries is None:
        blockers.append("CDFS_FINAL_ROOT_COUNT_OPEN")
    elif entries < 1 or entries > CDFS_CAP:
        add(errors, f"cdfs_entries must be 1..{CDFS_CAP}")

    dma = m.get("dma", {})
    if not isinstance(dma, dict):
        add(errors, "dma must be object")
        dma = {}
    for i, d in enumerate(dma.get("transfers", [])):
        try:
            src = num(d.get("src")); dst = num(d.get("dst")); size = num(d.get("bytes"))
        except Exception as e:
            add(errors, f"dma.transfers[{i}] invalid numeric: {e}")
            continue
        if src is None or dst is None or size is None or size <= 0:
            add(errors, f"dma.transfers[{i}] invalid range")
            continue
        if in_ranges(src, size, LWRAM_RANGES) or in_ranges(dst, size, LWRAM_RANGES):
            add(errors, f"DMA touches LWRAM: {d.get('name','?')}")
        if d.get("class") == "graphics_scu" and not in_ranges(src, size, HWRAM_RANGES):
            add(errors, f"graphics SCU DMA source not bounded HWRAM: {d.get('name','?')}")
    for i, t in enumerate(dma.get("indirect_tables", [])):
        try:
            count = num(t.get("count")); base = num(t.get("base"))
        except Exception as e:
            add(errors, f"dma.indirect_tables[{i}] invalid numeric: {e}")
            continue
        if count is None or count <= 0 or base is None:
            add(errors, f"dma.indirect_tables[{i}] invalid")
            continue
        align = 1
        needed = count * 12
        while align < needed:
            align <<= 1
        if base % align:
            add(errors, f"indirect DMA table {t.get('name','?')} base not aligned to {align}")
        if not t.get("end_marker", False):
            add(errors, f"indirect DMA table {t.get('name','?')} missing end marker")

    for b in m.get("open_blockers", []):
        if isinstance(b, str) and b and b not in blockers:
            blockers.append(b)

    if final and blockers:
        for b in blockers:
            add(errors, f"FINAL blocked: {b}")
    return errors, blockers

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest")
    ap.add_argument("--final", action="store_true", help="fail if any final integration blocker remains")
    args = ap.parse_args()
    m = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
    errors, blockers = validate(m, final=args.final)
    if errors:
        print("FAIL")
        for e in errors:
            print(" -", e)
        return 1
    print("PASS: structural memory contract")
    if blockers:
        print("BLOCKED_FINAL:")
        for b in blockers:
            print(" -", b)
    else:
        print("FINAL_READY: no memory-manifest blockers")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
