#!/usr/bin/env python3
"""Independent pack decoding, native bounds checking and full sprite identity.

These checks catch a wrong offset/byte order, a lost tile, a stale cache hit,
an accepted corrupt compressed block, and writes beyond the caller's cache.
The expected graphics bytes come from the supplied canonical sprite source,
never from the compressor or native decoder under test.
"""
from pathlib import Path
import argparse
import importlib.util
import struct
import shutil
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
SCENES = ("R18RING.BIN", "R18MENU.BIN", "R18MAP1.BIN", "R18MAP2.BIN", "R18MAP4.BIN", "R18MAP5.BIN", "R18MAP6.BIN")


def check_scenes(source: Path) -> bytes:
    spec = importlib.util.spec_from_file_location("wf_build_assets", ROOT / "tools/build_assets.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    original = [(source / name).read_bytes() for name in SCENES]
    assert hasattr(module, "build_scene_pack"), "compressed resident scene pack is not implemented"
    packed = module.build_scene_pack(original)
    magic, version, block_size, files, blocks, data_at, total, checksum = struct.unpack_from(
        ">4s7I", packed)
    assert (magic, version, block_size, files) == (b"WFSC", 2, 8192, 7)
    assert total == len(packed) and total <= 253952
    assert checksum == zlib.adler32(packed[32:data_at]) & 0xFFFFFFFF
    spans = [struct.unpack_from(">2I", packed, 32 + files * 16 + b * 8)
             for b in range(blocks)]
    unique = sorted(set(spans))
    assert len(unique) < blocks, "identical scene blocks must share resident storage"
    assert unique[0][0] == data_at and unique[-1][1] == total
    assert all(data_at <= a < b <= total for a, b in spans)
    assert all(left[1] == right[0] for left, right in zip(unique, unique[1:]))
    next_block = 0
    for index, expected in enumerate(original):
        raw_size, first, count, reserved = struct.unpack_from(">4I", packed, 32 + index * 16)
        assert (raw_size, first, reserved) == (len(expected), next_block, 0)
        assert count == (len(expected) + 8191) // 8192
        actual = b"".join(zlib.decompress(packed[spans[b][0]:spans[b][1]])
                          for b in range(first, first + count))
        assert actual == expected, SCENES[index]
        next_block += count
    assert next_block == blocks
    return packed


def check_pack(raw: bytes, packed: bytes, block_bytes: int) -> bytes:
    assert len(packed) > 32, "empty WFSZ pack"
    magic, version, block_size, raw_size, blocks, data_at, total, checksum = struct.unpack_from(
        ">4s7I", packed
    )
    assert (magic, version, block_size, raw_size, blocks) == (
        b"WFSZ", 1, block_bytes, 8388608, 8388608//block_bytes
    )
    assert total == len(packed) and data_at % 32 == 0
    assert checksum == zlib.adler32(packed[32:data_at]) & 0xFFFFFFFF
    offsets = struct.unpack_from(f">{blocks+1}I", packed, 32)
    assert offsets[0] == data_at and offsets[-1] == len(packed)
    for index in range(blocks):
        assert offsets[index] < offsets[index + 1]
        actual = zlib.decompress(packed[offsets[index]:offsets[index + 1]])
        assert actual == raw[index * block_size:(index + 1) * block_size], index
    assert (len(packed) + 31 & ~31) + 524288 + 440710 <= 4194304, (
        "full graphics, ROM and native SFX exceed the four-megabyte cartridge"
    )
    return packed


def check_builder(raw: bytes, block_bytes: int, compressor: str) -> bytes:
    script = ROOT / "tools/build_assets.py"
    if not script.exists():
        raise AssertionError("asset pack builder is not implemented")
    spec = importlib.util.spec_from_file_location("wf_build_assets", script)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    packed = check_pack(raw, module.build_sprite_pack(raw, block_bytes, compressor), block_bytes)
    try:
        module.build_sprite_pack(raw[:-1])
    except ValueError:
        pass
    else:
        raise AssertionError("truncated source must be rejected")
    return packed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--sprite-block-bytes",type=int,choices=(4096,8192),default=8192)
    parser.add_argument("--sprite-compressor",choices=("zopfli","zlib"),default="zlib")
    parser.add_argument("--sprite-pack",type=Path,
                        help="Verify an existing WFSZ against canonical pixels without recompressing it.")
    args = parser.parse_args()
    raw = (args.source_dir / "SPRITES.PAK").read_bytes()
    assert len(raw) == 8388608
    try:
        packed = (check_pack(raw,args.sprite_pack.read_bytes(),args.sprite_block_bytes)
                  if args.sprite_pack else check_builder(raw,args.sprite_block_bytes,args.sprite_compressor))
        scene_pack = check_scenes(args.source_dir)
    except (AssertionError, ValueError) as error:
        print("FAIL:", error, file=sys.stderr)
        return 1
    build = (args.build_dir or Path(tempfile.mkdtemp(prefix="wf-jo-assets-"))).resolve()
    build.mkdir(parents=True, exist_ok=True)
    pack_path = build / "SPRITES.WFP"
    pack_path.write_bytes(packed)
    (build / "SCENES.WFP").write_bytes(scene_pack)
    for name in ("ROM68K.BIN", *SCENES):
        shutil.copyfile(args.source_dir/name,build/name)
    sources = [ROOT / "tests/test_assets.c", ROOT / "port/wf_jo_assets.c"]
    sources += [ROOT / "port/zlib" / (name + ".c") for name in (
        "inflate", "inffast", "inftrees", "adler32", "zutil"
    )]
    command = ["cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
               "-DZ_SOLO", "-DNO_GZIP", "-DWF_JO_ASSETS_HOST",
               "-I" + str(ROOT / "port"), "-I" + str(ROOT / "port/zlib")]
    if args.sanitize:
        command += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    objects = []
    for source in sources:
        obj = build / (source.stem + ".o")
        vendor_flags = ["-Wno-implicit-fallthrough"] if source.parent.name == "zlib" else []
        subprocess.run([*command, *vendor_flags, "-c", str(source), "-o", str(obj)], check=True)
        objects.append(str(obj))
    subprocess.run([*command, *objects, "-o", str(build / "test_assets")], check=True)
    subprocess.run([str(build / "test_assets"), str(pack_path),
                    str((args.source_dir / "ROM68K.BIN").resolve()),
                    str((args.source_dir / "SPRITES.PAK").resolve())],
                   cwd=build, check=True)
    print(f"PASS: independent Python zlib decoded all {len(raw) // 128} tiles; pack={len(packed)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
