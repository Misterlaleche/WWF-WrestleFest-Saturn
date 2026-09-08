#!/usr/bin/env python3
from __future__ import annotations
import hashlib
import sys
import zipfile

EXPECTED = "a726c46f350723ea73158bc78055d17d418b14e2"


def load(zpath: str) -> bytes:
    with zipfile.ZipFile(zpath) as z:
        even = z.read("31e14-0.ic18")
        odd = z.read("31e13-0.ic19")
    assert len(even) == len(odd) == 0x40000
    out = bytearray(0x80000)
    out[0::2] = even
    out[1::2] = odd
    image = bytes(out)
    assert hashlib.sha1(image).hexdigest() == EXPECTED
    return image


def findall(image: bytes, pattern: bytes):
    result = []
    start = 0
    while True:
        pos = image.find(pattern, start)
        if pos < 0:
            return result
        result.append(pos)
        start = pos + 1


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: test409_actor32_raw10_rom.py wwfwfest.zip")

    image = load(sys.argv[1])
    bclr_a0 = bytes.fromhex("08 a8 00 02 00 32")
    bset_a0 = bytes.fromhex("08 e8 00 02 00 32")
    bset_a2 = bytes.fromhex("08 ea 00 02 00 32")
    btst_a0 = bytes.fromhex("08 28 00 02 00 32")

    assert findall(image, bclr_a0) == [0x114CC, 0x11E04]
    assert findall(image, bset_a0) == [
        0x0F3BC, 0x1C4CA, 0x1C52E, 0x1CED0, 0x1D85A, 0x1F354
    ]
    assert findall(image, bset_a2) == [0x214A4]
    assert findall(image, btst_a0) == [0x1D938]

    rawtab = 0x12614
    desc = int.from_bytes(image[rawtab + 0x10 * 4:rawtab + 0x10 * 4 + 4], "big")
    assert desc == 0x141D8
    assert int.from_bytes(image[desc:desc + 4], "big") == 0x141EC

    signatures = {
        0xE382: "22 68 00 7a",
        0xE386: "08 28 00 05 00 34",
        0xE390: "0c 28 00 00 00 21",
        0xE398: "0c 28 00 01 00 21",
        0xE3A2: "4a 28 00 af",
        0xE3AA: "4a a9 00 26",
        0xE3B2: "0c 29 00 04 00 21",
        0xE3BC: "0c 29 00 01 00 65",
        0xE3FC: "30 3c 00 10",
    }
    for address, hexbytes in signatures.items():
        pattern = bytes.fromhex(hexbytes)
        assert image[address:address + len(pattern)] == pattern

    for lo, hi in [(0xE382, 0xE408), (0x141EC, 0x14658)]:
        window = image[lo:hi]
        for pattern in (bclr_a0, bset_a0, bset_a2, btst_a0):
            assert pattern not in window

    assert image[0x114CC:0x114D2] == bclr_a0
    assert image[0x11E04:0x11E0A] == bclr_a0
    assert image[0x11DE0:0x11DE6] == bytes.fromhex("08 28 00 07 00 1c")

    state1_write = bytes.fromhex("31 7c 00 01 00 20")
    assert image[0xF228:0xF22E] == state1_write
    assert image[0xF2A2:0xF2A8] == state1_write
    state1_control = image[0x11652:0x11C62]
    for pattern in (bclr_a0, bset_a0, bset_a2):
        assert pattern not in state1_control

    state1_sites = findall(image, state1_write)
    assert len(state1_sites) == 36
    for address in (0x1DEF8, 0x1DF42, 0x1DFB8):
        assert address in state1_sites
    for pattern in (bclr_a0, bset_a0, bset_a2):
        assert pattern not in image[0x1DA7E:0x1DFE2]

    assert image[0x1D85A:0x1D860] == bset_a0
    assert int.from_bytes(image[rawtab + 0x75 * 4:rawtab + 0x75 * 4 + 4], "big") == 0x1A08C
    assert int.from_bytes(image[0x1A08C:0x1A090], "big") == 0x1A0AC
    assert image[0x1A0C2:0x1A0C8] == bytes.fromhex("31 7c 00 06 00 20")
    assert image[0x1A0D8:0x1A0DE] == bytes.fromhex("31 7c 00 00 00 20")
    assert image[0x1A0E0:0x1A0E6] == bytes.fromhex("31 7c 00 02 00 20")

    print("PASS Test409 actor32/raw10 ROM audit")


if __name__ == "__main__":
    main()
