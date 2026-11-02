#!/usr/bin/env python3
"""Rebuild SHA-verified PCM goldens from original OKI ROM and test native SFX.

All ROM material and generated test fixtures remain in the explicit output
directory. The independent archived audit fixes every expected PCM SHA256.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct
import subprocess

STEPS = (16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,
         107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,
         494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552)
INDEX_SHIFT = (-1,-1,-1,-1,2,4,6,8)
ROM_SHA256 = "e4cbe71f079e2cb95b3463115557fa1d99e18ea975f2d3354827b01994fc4615"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def decode(data: bytes) -> bytes:
    signal = index = 0
    result = bytearray()
    for byte in data:
        for nibble in (byte >> 4, byte & 15):
            step = STEPS[index]
            delta = (step // 8) + ((step // 4) if nibble & 1 else 0) + \
                    ((step // 2) if nibble & 2 else 0) + (step if nibble & 4 else 0)
            signal = min(2047, max(-2048, signal + (-delta if nibble & 8 else delta)))
            index = min(48, max(0, index + INDEX_SHIFT[nibble & 7]))
            result += struct.pack(">h", signal * 16)
    return bytes(result)


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "reference", "pack", "out"):
        p.add_argument("--" + name, type=Path, required=True)
    oracle = Path(__file__).resolve().parent / "oracle_sfx"
    p.add_argument("--manifest", type=Path, default=oracle / "ROM_OKI_MANIFEST.csv")
    p.add_argument("--audit", type=Path, default=oracle / "WF_OKI_AUDIT.json")
    p.add_argument("--production", type=Path, default=Path(__file__).resolve().parents[2])
    p.add_argument("--cc", default="cc")
    a = p.parse_args()
    for name in ("rom", "manifest", "audit", "reference", "pack", "out", "production"):
        setattr(a, name, getattr(a, name).resolve())
    a.out.mkdir(parents=True, exist_ok=True)
    rom = a.rom.read_bytes()
    if len(rom) != 524288 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("Original 512 KiB OKI ROM identity mismatch")
    audit = json.loads(a.audit.read_text())
    with a.manifest.open(newline="") as f:
        rows = {(int(r["bank"]), int(r["index"])): r for r in csv.DictReader(f)}
    if len(rows) != 114 or len(audit["samples"]) != 114:
        raise ValueError("Expected complete 114-entry independent ROM audit")
    golden = bytearray(audit["pak_bytes"])
    samples = []
    for sample in audit["samples"]:
        bank, sid = sample["bank"], sample["sample_id"]
        row = rows[bank, sid]
        start, count = bank * 0x40000 + int(row["adpcm_start"]), int(row["adpcm_bytes"])
        data = rom[start:start + count]
        if hashlib.sha1(data).hexdigest() != sample["sha1_adpcm"]:
            raise ValueError(f"ADPCM identity mismatch {bank}/{sid}")
        pcm = decode(data)
        if len(pcm) != sample["bytes"] or hashlib.sha256(pcm).hexdigest() != sample["sha256_pcm16be"]:
            raise ValueError(f"Archived complete PCM digest mismatch {bank}/{sid}")
        offset = sample["offset"]
        golden[offset:offset + len(pcm)] = pcm
        samples.append({"bank": bank, "id": sid, "frames": len(pcm) // 2,
                        "sha256_pcm16be": hashlib.sha256(pcm).hexdigest()})
    if hashlib.sha256(golden).hexdigest() != audit["pak_sha256"]:
        raise ValueError("Complete audited PCM pack mismatch")
    pcm_path = a.out / "WF_OKI_PCM16BE.PAK"
    pcm_path.write_bytes(golden)
    fixture = "typedef struct {unsigned bank,id,offset,frames;} fixture_t;\n"
    fixture += "static const fixture_t fixtures[114]={\n"
    fixture += "".join("{%d,%d,%d,%d},\n" % (s["bank"], s["sample_id"], s["offset"], s["frames"])
                       for s in audit["samples"])
    (a.out / "sfx_fixture.h").write_text(fixture + "};\n")
    sources = [a.production / "src/wf_scsp_sfx.c", a.reference / "wf_audio_sfx.c",
               a.production / "tests/test_sfx_native.c"]
    observed = sources + [a.production / "src/wf_scsp_sfx.h", a.reference / "wf_audio_sfx.h",
                          a.audit, a.manifest]
    before = {str(path): digest(path) for path in observed}
    executable = a.out / "test_sfx_native"
    command = [a.cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
               "-I", str(a.production / "src"), "-I", str(a.reference), "-I", str(a.out),
               *map(str, sources), "-o", str(executable)]
    subprocess.run(command, check=True, text=True, capture_output=True)
    result = subprocess.run([str(executable), str(a.pack), str(pcm_path)],
                            text=True, capture_output=True)
    stable = before == {str(path): digest(path) for path in observed}
    report = {
        "kind": "HOST_LOGIC_AND_FULL_ROM_PCM_VALIDATION_NOT_HARDWARE_CAPTURE",
        "source_rom_sha256": hashlib.sha256(rom).hexdigest(),
        "pack_sha256": digest(a.pack), "pcm_pack_sha256": digest(pcm_path),
        "all_114_full_pcm_digest_matches": True,
        "samples": samples, "build_command": command,
        "source_sha256": before, "source_stable_during_tests": stable,
        "passed": result.returncode == 0 and stable,
        "exit_code": result.returncode, "stdout": result.stdout, "stderr": result.stderr,
        "cases": ["114 complete ROM PCM decodes", "98 commands for 2400 ticks each",
                  "114 prepared prefixes and decoder states: monolithic, 4x64 and uneven chunks",
                  "30000 ticks of mixed commands and priority decisions",
                  "bank inheritance and F1 suppression", "refill and starvation containment",
                  "late starts and prefill timing", "16 volume settings", "invalid pack bind",
                  "DSP backend and guard sample"],
    }
    (a.out / "sfx-validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(result.stdout + result.stderr, end="")
    return result.returncode or (0 if stable else 1)


if __name__ == "__main__":
    raise SystemExit(main())
