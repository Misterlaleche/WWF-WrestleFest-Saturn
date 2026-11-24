#!/usr/bin/env python3
"""Verify static HWRAM leaves a safe gap before Jo's music/work lease."""
from __future__ import annotations

import argparse
import re
from pathlib import Path

END_RE = re.compile(r"^\s*(0x[0-9a-fA-F]+)\s+_end\s*=\s*\.\s*$")
JO_WORK_RE = re.compile(r"^JO_WORK\s+(0x[0-9a-fA-F]+)\b")


def parse_layout(path: Path) -> tuple[int, int]:
    end = work = None
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = END_RE.match(line)
        if match:
            end = int(match.group(1), 16)
            continue
        match = JO_WORK_RE.match(line)
        if match and work is None:
            work = int(match.group(1), 16)
    if end is None:
        raise SystemExit(f"missing _end in {path}")
    if work is None:
        raise SystemExit(f"missing JO_WORK in {path}")
    if end > work:
        raise SystemExit(f"static image overlaps JO_WORK: end=0x{end:08x} work=0x{work:08x}")
    return end, work


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("map", type=Path)
    parser.add_argument("--minimum", type=int, default=8192)
    args = parser.parse_args()
    end, work = parse_layout(args.map)
    margin = work - end
    print(f"_end=0x{end:08x} JO_WORK=0x{work:08x} margin={margin} required={args.minimum}")
    if margin < args.minimum:
        print(f"FAIL: static margin short by {args.minimum - margin} bytes")
        return 1
    print("PASS: static margin contract satisfied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
