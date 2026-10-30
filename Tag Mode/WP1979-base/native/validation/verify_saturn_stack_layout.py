#!/usr/bin/env python3
"""Reject a Saturn SH-2 image whose slave stack collides with code or RAM."""

import struct
import sys
from pathlib import Path


def sections_and_symbols(data):
    if data[:4] != b"\x7fELF" or data[4] != 1 or data[5] not in (1, 2):
        raise ValueError("expected ELF32")
    endian = "<" if data[5] == 1 else ">"
    shoff = struct.unpack_from(endian + "I", data, 32)[0]
    shentsize, shnum, shstrndx = struct.unpack_from(endian + "HHH", data, 46)
    if shentsize < 40 or shstrndx >= shnum:
        raise ValueError("invalid section table")
    headers = [struct.unpack_from(endian + "IIIIIIIIII", data, shoff + i * shentsize)
               for i in range(shnum)]

    def blob(header):
        offset, size = header[4], header[5]
        if offset + size > len(data):
            raise ValueError("section outside ELF file")
        return data[offset:offset + size]

    def name(table, offset):
        end = table.find(b"\0", offset)
        if offset >= len(table) or end < 0:
            raise ValueError("invalid ELF name")
        return table[offset:end].decode("ascii")

    section_names = blob(headers[shstrndx])
    bss = next((h for h in headers if name(section_names, h[0]) == ".bss"), None)
    if bss is None:
        raise ValueError("missing .bss")
    symbols = {}
    for h in headers:
        if h[1] != 2:  # SHT_SYMTAB
            continue
        if h[6] >= shnum or h[9] < 16:
            raise ValueError("invalid symbol table")
        strings = blob(headers[h[6]])
        table = blob(h)
        for offset in range(0, len(table), h[9]):
            name_offset, value = struct.unpack_from(endian + "II", table, offset)
            if name_offset:
                symbols[name(strings, name_offset)] = value
    return bss[3], bss[3] + bss[5], symbols


def verify(data):
    bss_start, bss_end, symbols = sections_and_symbols(data)
    master = symbols["___master_stack"]
    slave = symbols["___slave_stack"]
    if not (0x06000000 < master <= 0x06004000):
        raise ValueError(f"master stack outside its reserved region: {master:#010x}")
    if not (0x06080000 < slave <= 0x06100000):
        raise ValueError(f"slave stack overlaps the master or falls outside high RAM: {slave:#010x}")
    if bss_end + 0x2000 > slave:
        raise ValueError(f"less than 8 KiB between .bss end {bss_end:#010x} and slave stack {slave:#010x}")
    print(f"Saturn stacks OK: master={master:#010x}, slave={slave:#010x}, "
          f".bss={bss_start:#010x}..{bss_end:#010x}, slave headroom={slave - bss_end} bytes")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]).read_bytes())
    except (IndexError, KeyError, ValueError) as exc:
        raise SystemExit(f"Saturn stack layout invalid: {exc}")
