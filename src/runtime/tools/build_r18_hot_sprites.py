#!/usr/bin/env python3
"""Copy traced ROM sprite tiles into a deterministic, sector-aligned CD cache."""

import argparse
import hashlib
from pathlib import Path

PACK_SHA256 = "130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31"
HEADER_BYTES = 4096
EXTENDED_HEADER_BYTES = 8192
LARGE_HEADER_BYTES = 16384
TILE_BYTES = 128
LEGACY_MAX_IDS = (HEADER_BYTES - 8) // 2
EXTENDED_MAX_IDS = (EXTENDED_HEADER_BYTES - 8) // 2
MAX_IDS = (LARGE_HEADER_BYTES - 8) // 2


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("ids", type=Path)
    parser.add_argument("sprites", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    source = args.sprites.read_bytes()
    digest = hashlib.sha256(source).hexdigest()
    if digest != PACK_SHA256 or len(source) != 65536 * TILE_BYTES:
        raise SystemExit("SPRITES.PAK size or SHA-256 differs from the traced ROM pack")
    ids = [int(line) for line in args.ids.read_text().splitlines()
           if line and not line.startswith("#")]
    if not ids or len(ids) > MAX_IDS or ids != sorted(set(ids)) or any(
            tile < 0 or tile >= 65536 for tile in ids):
        raise SystemExit("Hot sprite IDs must be sorted, unique and in range")

    header = bytearray(HEADER_BYTES if len(ids) <= LEGACY_MAX_IDS
                      else EXTENDED_HEADER_BYTES if len(ids) <= EXTENDED_MAX_IDS
                      else LARGE_HEADER_BYTES)
    header[:4] = b"R18H"
    header[4:6] = len(ids).to_bytes(2, "big")
    header[6:8] = TILE_BYTES.to_bytes(2, "big")
    for i, tile in enumerate(ids):
        header[8 + 2 * i:10 + 2 * i] = tile.to_bytes(2, "big")
    data = bytes(header) + b"".join(source[tile * TILE_BYTES:(tile + 1) * TILE_BYTES]
                                    for tile in ids)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"HOTSPR.BIN tiles={len(ids)} bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}")


if __name__ == "__main__":
    main()
