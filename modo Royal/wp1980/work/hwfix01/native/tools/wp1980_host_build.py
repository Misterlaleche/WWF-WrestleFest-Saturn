#!/usr/bin/env python3
"""Build the WP1980 host probes incrementally, without mutating prior work."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("--probe", choices=["launch", "combat", "frontend_wait", "sprite_pack", "royal_init", "rom_pages"], default="combat")
parser.add_argument("--jobs", type=int, default=2)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
makefile = (root / "Makefile").read_text()
sources = re.search(r"^CORE=(.*)$", makefile, re.M).group(1).split()
sources += re.search(r"^R18_HOST=(.*)$", makefile, re.M).group(1).split()
sources += ["src/wp1980_input.c", "src/wp1980_inflate.c", "src/wp1980_lz4.c", "src/wp1980_inflate_dynamic.c", "src/wp1980_vram_cache.c", "src/wp1980_royal_init.c", f"tests/r18/wp1980_{args.probe}_test.c"]
build = root / "audit/wp1980-host"
objects = build / "objects"
objects.mkdir(parents=True, exist_ok=True)
headers_mtime = max(p.stat().st_mtime for p in (root / "include").glob("*.h"))
flags = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O0", "-g", "-Iinclude",
         "-DWF_R18_ROM_EXTRA_CAPACITY=32u", "-DWF_SATURN_HWRAM_HOT_INDEX_BYTES=0x34000u"]
config_stamp = objects / "compile-flags.txt"
config_text = "\n".join(flags) + "\n"
if not config_stamp.exists() or config_stamp.read_text() != config_text:
    config_stamp.write_text(config_text)
config_mtime = config_stamp.stat().st_mtime


def compile_source(source):
    obj = objects / (source.replace("/", "_") + ".o")
    newest = max((root / source).stat().st_mtime, headers_mtime, config_mtime)
    if source.endswith(("wp1980_combat_test.c", "wp1980_frontend_wait_test.c")):
        newest = max(newest, (root / "tests/r18/wp1980_launch_test.c").stat().st_mtime)
    if not obj.exists() or obj.stat().st_mtime < newest:
        subprocess.run(["cc", *flags, "-c", source, "-o", str(obj)], cwd=root, check=True)
    return obj


with ThreadPoolExecutor(max_workers=args.jobs) as workers:
    compiled = list(workers.map(compile_source, sources))
binary = build / f"{args.probe}-test"
subprocess.run(["cc", *(str(p) for p in compiled), "-o", str(binary)], cwd=root, check=True)
print(binary)
