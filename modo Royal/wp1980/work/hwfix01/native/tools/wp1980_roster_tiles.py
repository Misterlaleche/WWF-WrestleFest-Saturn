#!/usr/bin/env python3
"""Inventory exact immutable ROM pose tiles for specified Royal identities.

This is an asset inventory only. It does not select an animation, participant,
pose, or gameplay outcome. Both actor identities in linked compositions are
enumerated from the requested roster; all valid original pose slots are read.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
parser = argparse.ArgumentParser()
parser.add_argument("--program", type=Path, required=True)
parser.add_argument("--decoder", type=Path, required=True)
parser.add_argument("--identities", default="0,5,3,8,10,9")
parser.add_argument("--out", type=Path, required=True)
args = parser.parse_args()
identities = sorted({int(value, 0) for value in args.identities.split(",")})
assert identities and all(0 <= value < 12 for value in identities)
spec = importlib.util.spec_from_file_location("wp1980_original_graphics", args.decoder)
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)
program = args.program.read_bytes()
assert hashlib.sha1(program).hexdigest() == decoder.MAIN_SHA1
rom = decoder.Rom.__new__(decoder.Rom)
rom.p = program
args.out.mkdir(parents=True, exist_ok=True)
by_set = {item[0]: item for item in rom.active_sets()}
all_tiles = set()
summary = {"program_sha1": decoder.MAIN_SHA1, "identities": identities,
           "scope": "All valid original poses and all linked partner variants within the supplied identity set",
           "sets": []}
for identity in identities:
    tiles = set()
    poses = 0
    count = by_set[identity][3]
    for pose in range(count):
        if rom.pose_entry(identity, pose) in (None, 0xfffe, 0xffff):
            continue
        poses += 1
        for partner in identities:
            for dx, dy, attr, resource, high, low in rom.pose_rows(identity, pose, partner):
                chain = (attr >> 5) & 7
                tile = ((high << 8) | low) & ~((1 << chain.bit_length()) - 1)
                tiles.update(range(tile, tile + chain + 1))
    (args.out / f"identity-{identity:02d}.txt").write_text("".join(f"0x{tile:04x}\n" for tile in sorted(tiles)))
    summary["sets"].append({"identity": identity, "pose_slots": count, "valid_poses": poses,
                            "unique_tiles_including_partner_compositions": len(tiles)})
    all_tiles.update(tiles)
(args.out / "tiles.txt").write_text("".join(f"0x{tile:04x}\n" for tile in sorted(all_tiles)))
summary["unique_tiles"] = len(all_tiles)
summary["raw_tile_bytes"] = len(all_tiles) * 128
(args.out / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps(summary, indent=2))
