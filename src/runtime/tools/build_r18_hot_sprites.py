#!/usr/bin/env python3
"""Copy traced ROM sprite tiles into a deterministic, sector-aligned CD cache."""

import argparse
import hashlib
import re
from pathlib import Path

PACK_SHA256 = "130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31"
HEADER_BYTES = 4096
EXTENDED_HEADER_BYTES = 8192
LARGE_HEADER_BYTES = 16384
TILE_BYTES = 128
LEGACY_MAX_IDS = (HEADER_BYTES - 8) // 2
EXTENDED_MAX_IDS = (EXTENDED_HEADER_BYTES - 8) // 2
MAX_IDS = (LARGE_HEADER_BYTES - 8) // 2
DICTIONARY_PREFIX_BYTES = 24
MEMORY_CONTRACT = (Path(__file__).resolve().parents[1] / 'include/wf_saturn_memory.h').read_text()
CACHE_BYTES = int(re.search(r'#define WF_SATURN_LWRAM_TILE_BYTES\s+(0x[0-9a-f]+)u', MEMORY_CONTRACT)[1], 16)
INDEX_BYTES = int(re.search(r'#define WF_SATURN_HWRAM_HOT_INDEX_BYTES\s+(0x[0-9a-f]+)u', MEMORY_CONTRACT)[1], 16)


def build_pack(ids: list[int], source: bytes, format_name: str,
               chunk_bytes: int | None = None) -> bytes:
    if format_name == "raw" and chunk_bytes is not None:
        raise SystemExit("Chunk width requires dictionary format")
    if chunk_bytes is not None and chunk_bytes not in (4, 16, 32):
        raise SystemExit("Chunk width must be 4, 16 or 32 bytes")
    dictionary = chunk_bytes is not None or format_name == "dictionary" or (format_name == "auto" and
                                                (len(ids) + 1) * TILE_BYTES > CACHE_BYTES)
    prefix = DICTIONARY_PREFIX_BYTES if dictionary else 8
    needed = prefix + len(ids) * 2
    if needed > LARGE_HEADER_BYTES:
        raise SystemExit("Hot sprite ID table exceeds the 16 KiB header")
    header_bytes = (HEADER_BYTES if needed <= HEADER_BYTES else
                    EXTENDED_HEADER_BYTES if needed <= EXTENDED_HEADER_BYTES else
                    LARGE_HEADER_BYTES)
    header = bytearray(header_bytes)
    header[:4] = b"R18D" if dictionary else b"R18H"
    header[4:6] = len(ids).to_bytes(2, "big")
    header[6:8] = TILE_BYTES.to_bytes(2, "big")
    for i, tile in enumerate(ids):
        header[prefix + 2 * i:prefix + 2 * i + 2] = tile.to_bytes(2, "big")
    if dictionary:
        for width in ((chunk_bytes,) if chunk_bytes is not None else (32, 16, 4)):
            indices = bytearray()
            chunks: dict[bytes, int] = {}
            overflow = False
            for tile in ids:
                for offset in range(0, TILE_BYTES, width):
                    at = tile * TILE_BYTES + offset
                    chunk = source[at:at + width]
                    if chunk not in chunks and len(chunks) == 65536:
                        overflow = True
                        break
                    index = chunks.setdefault(chunk, len(chunks))
                    indices += index.to_bytes(2, "big")
                if overflow:
                    break
            if overflow:
                continue
            dictionary_bytes = b"".join(chunks)
            padded_indices = (len(indices) + 31) // 32 * 32
            spill = max(0, padded_indices - INDEX_BYTES)
            resident_bytes = (spill + len(dictionary_bytes) + 127) // 128 * 128
            if resident_bytes + 128 <= CACHE_BYTES:
                break
        else:
            raise SystemExit(f"No exact 4/16/32-byte dictionary fits {INDEX_BYTES} HWRAM index bytes, {CACHE_BYTES} LWRAM bytes and BE16 indices")
        header[8:12] = len(chunks).to_bytes(4, "big")
        header[12:16] = len(indices).to_bytes(4, "big")
        header[16:20] = len(dictionary_bytes).to_bytes(4, "big")
        header[20:24] = header_bytes.to_bytes(4, "big")
        payload = bytes(indices) + bytes((-len(indices)) % 32) + dictionary_bytes
    else:
        payload = b"".join(source[tile * TILE_BYTES:(tile + 1) * TILE_BYTES]
                           for tile in ids)
    # R18D indices use a bounded HWRAM table; LWRAM retains the dictionary.
    resident_payload = spill + len(dictionary_bytes) if dictionary else len(payload)
    resident_bytes = (resident_payload + TILE_BYTES - 1) // TILE_BYTES * TILE_BYTES
    if resident_bytes + TILE_BYTES > CACHE_BYTES:
        raise SystemExit(f"Hot sprite payload leaves no cold fallback slot in {CACHE_BYTES} bytes")
    return bytes(header) + payload


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("ids", type=Path)
    parser.add_argument("sprites", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--format", choices=("auto", "raw", "dictionary"), default="auto")
    parser.add_argument("--chunk-bytes", type=int, choices=(4, 16, 32))
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

    data = build_pack(ids, source, args.format, args.chunk_bytes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"HOTSPR.BIN format={data[:4].decode()} tiles={len(ids)} bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}")


if __name__ == "__main__":
    main()
