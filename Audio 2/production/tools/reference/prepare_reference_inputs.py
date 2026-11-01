#!/usr/bin/env python3
"""Recreate private reference inputs from the user's original ROM zip.

The archived A53 oracle consumes the legacy A15 signed PCM8 pack, whose decoder
starts at -2. Native PCM16 validation independently uses the archived initial-0
decoder. These are distinct, hash-pinned baselines, never interchangeable.
No decoded sample or recovered command table is stored in this source bundle.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import sys
import zipfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import generate_resources as resources

PCM8_SHA256 = "bbb80c6444d9e9e2a5d9d496418b9650171562ef8cf32533d969fb2f4a5e34b9"
INDEX_SHA256 = "18ba394e95a57b5f4688a866f400f6453a63e1e70752eb50cf7418d292560fa5"
MODEL_SHA256 = "7129466a7d688fcfd2600413b91d4951d80c16de1f411489bbfbfa66fdbc5e6b"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def verify_bundle() -> None:
    manifest = json.loads((HERE / "oracle-input-sha256.json").read_text())
    for name, item in manifest.items():
        if isinstance(item, dict):
            data = (HERE / name).read_bytes()
            if len(data) != item["bytes"] or sha(data) != item["sha256"]:
                raise ValueError("Archived oracle input changed: " + name)


def legacy_pcm8(encoded: bytes) -> bytes:
    signal, index = -2, 0
    result = bytearray()
    for byte in encoded:
        for nibble in (byte >> 4, byte & 15):
            step = resources.STEPS[index]
            delta = (step >> 3) + ((step >> 2) if nibble & 1 else 0) + \
                    ((step >> 1) if nibble & 2 else 0) + (step if nibble & 4 else 0)
            signal = max(-2048, min(2047, signal + (-delta if nibble & 8 else delta)))
            index = max(0, min(48, index + resources.SHIFTS[nibble & 7]))
            result.append((signal >> 4) & 255)
    return bytes(result)


def make_pcm8(samples: bytes) -> tuple[bytes, bytes]:
    result = bytearray()
    index_rows = []
    seen = {}
    for bank, count in ((0, 73), (1, 41)):
        for sid in range(1, count + 1):
            table = bank * 0x40000 + sid * 8
            start = int.from_bytes(samples[table:table + 3], "big")
            end = int.from_bytes(samples[table + 3:table + 6], "big")
            if not 0 <= start <= end < 0x40000:
                raise ValueError("Invalid original ROM sample range")
            encoded = samples[bank * 0x40000 + start:bank * 0x40000 + end + 1]
            identity = hashlib.sha1(encoded).hexdigest()
            alias = ""
            if identity not in seen:
                decoded = legacy_pcm8(encoded)
                result += b"\0" * (-len(result) % 2048)
                seen[identity] = (len(result), len(decoded), bank)
                result += decoded
            else:
                alias = "bank" + str(seen[identity][2])
            offset, length, _ = seen[identity]
            index_rows.append((bank, sid, offset // 2048, offset, length, alias))
    text = io.StringIO(newline="")
    writer = csv.writer(text)
    writer.writerow(("bank", "sample_id", "sector", "offset_bytes", "pcm8_bytes", "alias"))
    writer.writerows(index_rows)
    index = text.getvalue().encode("ascii")
    if len(seen) != 95 or len(result) != 980196 or sha(result) != PCM8_SHA256:
        raise ValueError("Legacy A15 PCM8 pack identity mismatch")
    if sha(index) != INDEX_SHA256:
        raise ValueError("Complete archived A15 PCM8 index identity mismatch")
    return bytes(result), index


def make_model(sound: bytes) -> bytes:
    rows = []
    for item in resources.commands(sound):
        events = item["events"] + [[0, 0, 0, 0]] * (9 - len(item["events"]))
        fields = (item["priority"], item["bank"], len(item["events"]), item["cleanup_stop"])
        row = " {" + ",".join(str(value) + "u" for value in fields) + ",{"
        row += ",".join("{" + ",".join(str(value) + "u" for value in event) + "}" for event in events)
        rows.append(row + "}}")
    table = "const wf_sfx_command_t wf_sfx_commands[98]={\n" + ",\n".join(rows) + "\n};"
    template = (HERE / "oracle_sfx/wf_audio_sfx.c.in").read_bytes()
    marker = b"@ROM_DERIVED_COMMAND_TABLE@"
    if template.count(marker) != 1:
        raise ValueError("A15 model template marker mismatch")
    result = template.replace(marker, table.encode("ascii"))
    if sha(result) != MODEL_SHA256:
        raise ValueError("Reconstructed independent A15 C model differs from archive")
    return result


def prepare(rom_zip: Path, out: Path) -> dict:
    verify_bundle()
    with zipfile.ZipFile(rom_zip) as archive:
        def original(name: str) -> bytes:
            choices = [item for item in archive.namelist() if Path(item).name == name]
            if len(choices) != 1:
                raise ValueError("Need exactly one original ROM member: " + name)
            return archive.read(choices[0])
        sound = original("31a11-2.ic42")
        samples = original("31j10.ic73")
    generated = resources.generate(sound, samples)
    pcm8, index = make_pcm8(samples)
    files = {
        "SOUND.ROM": sound,
        "SAMPLES.ROM": samples,
        "SFX.DAT": generated["pack"],
        "PCM16BE.PAK": generated["pcm16be"],
        "AUDPCM.PAK": pcm8,
        "AUDPCM-INDEX.csv": index,
        "a15_model/wf_audio_sfx.c": make_model(sound),
        "a15_model/wf_audio_sfx.h": (HERE / "oracle_sfx/wf_audio_sfx.h").read_bytes(),
    }
    out.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        path = out / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    report = {
        "kind": "PRIVATE_ROM_DERIVED_REFERENCE_INPUTS",
        "source_zip_sha256": sha(rom_zip.read_bytes()),
        "files": {name: {"bytes": len(data), "sha256": sha(data)} for name, data in files.items()},
        "command_model": "Archived A15 C, reconstructed from original ROM, whole-file SHA256 exact",
        "music_pcm": "Legacy A15 signed PCM8; signal starts at -2; 2048-byte alignment; exact original pack",
        "native_sfx_pcm": "Independent archived PCM16BE audit; signal starts at 0; 32-byte alignment",
        "distinct_samples": generated["distinct_samples"],
        "complete": True,
    }
    (out / "reference-inputs.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom-zip", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    result = prepare(args.rom_zip.resolve(), args.out.resolve())
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
