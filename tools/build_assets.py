#!/usr/bin/env python3
"""Pack every canonical WrestleFest sprite for the mandatory 4 MB cartridge.

WFSZ v1 stores independent zlib streams for 4 or 8 KB of original 4bpp pixels.
No pixels, palette indices, tile numbers or composition data are changed.
The runtime can decode any of the 65536 tiles without reading the CD.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import struct
import zlib
import concurrent.futures

SPRITE_BYTES = 8388608
ROM_BYTES = 524288
BLOCK_BYTES = 8192
CART_BYTES = 4194304
SFX_RESERVE = 440710
MUSIC_PREFIX_RESERVE = 32768
SCENE_RESIDENT_BYTES = 253952
SPRITE_CACHE_BYTES = 610304
SPRITE_CACHE_FRONTEND_BYTES = 593920
SCENE_ZOPFLI_OPTIONS = dict(numiterations=15, blocksplitting=1,
                           blocksplittinglast=0, blocksplittingmax=15)
SCENE_ASSETS = ("R18RING.BIN", "R18MENU.BIN", "R18MAP1.BIN",
                "R18MAP2.BIN", "R18MAP4.BIN", "R18MAP5.BIN", "R18MAP6.BIN")
SCENE_BYTES = (114918, 124938, 127138, 95978, 129998, 115568, 95198)
COPIED_ASSETS = ("ROM68K.BIN", "FONT8.BIN", *SCENE_ASSETS)


def align32(value: int) -> int:
    return (value + 31) & ~31


def build_sprite_pack(raw: bytes, block_bytes: int = BLOCK_BYTES,
                      compressor: str = "zlib") -> bytes:
    if len(raw) != SPRITE_BYTES:
        raise ValueError(f"SPRITES.PAK must contain exactly {SPRITE_BYTES} bytes")
    if block_bytes not in (4096, 8192):
        raise ValueError("sprite blocks must contain 4096 or 8192 bytes")
    blocks = len(raw) // block_bytes
    parts = [raw[at:at + block_bytes] for at in range(0, len(raw), block_bytes)]
    if compressor == "zopfli":
        import zopfli.zlib
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            payloads = list(pool.map(lambda b: zopfli.zlib.compress(b, numiterations=15), parts))
    elif compressor == "zlib":
        payloads = [zlib.compress(part, 9) for part in parts]
    else:
        raise ValueError("unsupported sprite compressor")
    data_offset = align32(32 + 4 * (blocks + 1))
    offsets = [data_offset]
    for payload in payloads:
        offsets.append(offsets[-1] + len(payload))
    index = struct.pack(f">{blocks + 1}I", *offsets)
    index += bytes(data_offset - 32 - len(index))
    header = struct.pack(">4s7I", b"WFSZ", 1, block_bytes, len(raw), blocks,
                         data_offset, offsets[-1], zlib.adler32(index) & 0xFFFFFFFF)
    packed = header + index + b"".join(payloads)
    if ROM_BYTES + align32(len(packed)) + align32(SFX_RESERVE) + MUSIC_PREFIX_RESERVE > CART_BYTES:
        raise ValueError("Sprite pack, ROM, SFX and music prefix do not fit the 4 MB cartridge")
    return packed


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def build_scene_pack(original: list[bytes], compressor: str = "zopfli") -> bytes:
    """Lossless scene files with independent blocks and original file views.

    WFSC v2: 32-byte header, seven 16-byte directory records (raw bytes,
    first logical block, block count, zero), absolute start/end pairs, then
    aligned payload. Identical raw blocks share a single zlib stream. Length
    participates in equality, so a short final block cannot alias a full one.
    Whole scene transfers decode directly into the caller's scene staging.
    Zopfli 0.4.3 with the fixed 15-iteration options reproduces the measured
    D03/E01 pack. Every compressor must fit the same reduced reservation.
    """
    if tuple(map(len, original)) != SCENE_BYTES:
        raise ValueError("scene files must match the seven canonical lengths")
    parts, references, unique, directory = [], [], {}, bytearray()
    for raw in original:
        count = (len(raw) + BLOCK_BYTES - 1) // BLOCK_BYTES
        directory += struct.pack(">4I", len(raw), len(references), count, 0)
        for at in range(0, len(raw), BLOCK_BYTES):
            block = raw[at:at + BLOCK_BYTES]
            if block not in unique:
                unique[block] = len(parts)
                parts.append(block)
            references.append(unique[block])
    if compressor == "zopfli":
        import zopfli.zlib
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            payloads = list(pool.map(lambda block: zopfli.zlib.compress(block, **SCENE_ZOPFLI_OPTIONS), parts))
    elif compressor == "zlib":
        payloads = [zlib.compress(block, 9) for block in parts]
    else:
        raise ValueError("unsupported scene compressor")
    blocks = len(references)
    data_at = align32(32 + len(directory) + blocks * 8)
    offsets = [data_at]
    for payload in payloads:
        offsets.append(offsets[-1] + len(payload))
    metadata = bytes(directory) + b"".join(
        struct.pack(">2I", offsets[index], offsets[index + 1])
        for index in references)
    metadata += bytes(data_at - 32 - len(metadata))
    header = struct.pack(">4s7I", b"WFSC", 2, BLOCK_BYTES, len(original), blocks, data_at,
                         offsets[-1], zlib.adler32(metadata) & 0xFFFFFFFF)
    packed = header + metadata + b"".join(payloads)
    if len(packed) > SCENE_RESIDENT_BYTES:
        raise ValueError(f"resident scene pack is {len(packed)} bytes; LWRAM allocation is {SCENE_RESIDENT_BYTES}")
    return packed


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", required=True, type=Path,
                        help="Directory with canonical ROM68K.BIN, SPRITES.PAK and ring/font packs")
    parser.add_argument("--output-dir", type=Path, default=Path(__file__).resolve().parents[1] / "cd")
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--sprite-block-bytes", type=int, choices=(4096,8192), default=4096)
    parser.add_argument("--sprite-compressor", choices=("zopfli","zlib"), default="zopfli")
    parser.add_argument("--scene-compressor", choices=("zopfli","zlib"), default="zopfli",
                        help="Scene streams must fit 253952 bytes; legacy zlib scenes exceed this reservation")
    args = parser.parse_args()
    rom = (args.source_dir / "ROM68K.BIN").read_bytes()
    if len(rom) != ROM_BYTES:
        raise SystemExit(f"ROM68K.BIN must contain exactly {ROM_BYTES} bytes")
    raw = (args.source_dir / "SPRITES.PAK").read_bytes()
    packed = build_sprite_pack(raw, args.sprite_block_bytes, args.sprite_compressor)
    original_scenes = [(args.source_dir / name).read_bytes() for name in SCENE_ASSETS]
    scene_pack = build_scene_pack(original_scenes, args.scene_compressor)
    # Independently decode each output stream before accepting the asset.
    sprite_blocks=len(raw)//args.sprite_block_bytes
    offsets = struct.unpack_from(f">{sprite_blocks+1}I", packed, 32)
    for block in range(sprite_blocks):
        if zlib.decompress(packed[offsets[block]:offsets[block + 1]]) != raw[block * args.sprite_block_bytes:(block + 1) * args.sprite_block_bytes]:
            raise SystemExit(f"Sprite block {block} failed byte-identity verification")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    # Frozen variants hardlink inputs. Replace their path atomically instead
    # of rewriting the shared inode while a build/capture is in progress.
    sprite_target=args.output_dir / "SPRITES.WFP"
    sprite_pending=sprite_target.with_suffix(".WFP.new")
    sprite_pending.write_bytes(packed);sprite_pending.replace(sprite_target)
    # Check each independently encoded scene before accepting its container.
    scene_blocks = struct.unpack_from(">I", scene_pack, 16)[0]
    scene_spans = [struct.unpack_from(">2I", scene_pack,
                   32 + len(original_scenes) * 16 + b * 8) for b in range(scene_blocks)]
    for index, expected in enumerate(original_scenes):
        _, first, count, _ = struct.unpack_from(">4I", scene_pack, 32 + index * 16)
        actual = b"".join(zlib.decompress(scene_pack[scene_spans[b][0]:scene_spans[b][1]])
                          for b in range(first, first + count))
        if actual != expected:
            raise SystemExit(f"Scene {SCENE_ASSETS[index]} failed byte identity")
    scene_target=args.output_dir / "SCENES.WFP"
    scene_pending=scene_target.with_suffix(".WFP.new")
    scene_pending.write_bytes(scene_pack);scene_pending.replace(scene_target)
    files = {}
    for name in COPIED_ASSETS:
        source = args.source_dir / name
        target = args.output_dir / name
        if source.resolve() != target.resolve():
            pending=target.with_suffix(target.suffix+".new")
            shutil.copyfile(source, pending);pending.replace(target)
        data = target.read_bytes()
        files[name] = {"bytes": len(data), "sha256": sha256(data)}
    files["SPRITES.WFP"] = {"bytes": len(packed), "sha256": sha256(packed)}
    files["SCENES.WFP"] = {"bytes": len(scene_pack), "sha256": sha256(scene_pack)}
    used = ROM_BYTES + align32(len(packed))
    prefix_available=CART_BYTES-used-align32(SFX_RESERVE)
    prefix_bytes=114688 if prefix_available>=114688 else 98304 if prefix_available>=98304 else MUSIC_PREFIX_RESERVE
    manifest = {
        "format": "WFSZ", "version": 1, "zlib_builder_version": zlib.ZLIB_VERSION,
        "sprite_tiles": 65536, "sprite_block_bytes": args.sprite_block_bytes,
        "sprite_blocks": sprite_blocks, "source_sprite_sha256": sha256(raw),
        "sprite_compressor": args.sprite_compressor,
        "scene_compressor": args.scene_compressor,
        "scene_compressor_options": SCENE_ZOPFLI_OPTIONS if args.scene_compressor == "zopfli" else {"level": 9},
        "cartridge_capacity": CART_BYTES, "cartridge_assets_used": used,
        "cartridge_remaining": CART_BYTES - used,
        "native_sfx_reserved_bytes": SFX_RESERVE,
        "remaining_after_native_sfx": CART_BYTES - used - SFX_RESERVE,
        "music_prefix_bytes": prefix_bytes,
        "music_prefix_workspace_bytes": 0 if prefix_bytes>=98304 else 32768,
        "remaining_after_audio": CART_BYTES-used-align32(SFX_RESERVE)-prefix_bytes,
        "resident_scene_bytes": len(scene_pack), "resident_scene_capacity": SCENE_RESIDENT_BYTES,
        "sprite_cache_bytes": SPRITE_CACHE_BYTES, "sprite_cache_blocks": SPRITE_CACHE_BYTES//args.sprite_block_bytes,
        "sprite_cache_bytes_with_frontend": SPRITE_CACHE_FRONTEND_BYTES,
        "sprite_cache_blocks_with_frontend": SPRITE_CACHE_FRONTEND_BYTES//args.sprite_block_bytes,
        "pixel_changes": 0, "files": files,
    }
    manifest_path = args.manifest or args.output_dir.parent / "evidence/assets-manifest.json"
    if "zopfli" in (args.sprite_compressor, args.scene_compressor):
        import zopfli
        manifest["zopfli_builder_version"]=zopfli.__version__
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"SPRITES.WFP: {len(packed)} bytes; all 65536 tiles verified; cartridge assets {used}/{CART_BYTES}, free {CART_BYTES-used}")
    print(f"SCENES.WFP: {len(scene_pack)} bytes; all seven scene files verified; LWRAM sprite cache with frontend {SPRITE_CACHE_FRONTEND_BYTES} bytes ({SPRITE_CACHE_FRONTEND_BYTES//args.sprite_block_bytes} blocks)")


if __name__ == "__main__":
    main()
