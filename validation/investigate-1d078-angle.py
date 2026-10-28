"""Limited AE5 angle investigation; NOT the full native/ROM oracle.

Uses angle immediates and branch bytes from the SHA-verified original ROM.
Compares the rejected arithmetic with the corrected branch expression.
Does not execute an M68000, C, TAG requests, facing, or AE6.
"""
from pathlib import Path
import hashlib
import json

root = Path(__file__).resolve().parents[1]
rom = (root / "src/runtime/audit/runtime18-cd/ROM68K.BIN").read_bytes()
assert hashlib.sha1(rom).hexdigest() == "a726c46f350723ea73158bc78055d17d418b14e2"
expected = {
    0x1D0C0: "b069000a6416",
    0x1D0C6: "117c00a0002d",
    0x1D0CC: "0828000000b5671c",
    0x1D0D4: "117c0060002d6014",
    0x1D0DC: "117c00e0002d",
    0x1D0E2: "0828000000b56706",
    0x1D0EA: "117c0020002d",
}
for address, value in expected.items():
    assert rom[address:address + len(bytes.fromhex(value))] == bytes.fromhex(value)

def immediate(address):
    return int.from_bytes(rom[address + 2:address + 4], "big")

count = rejected_mismatches = 0
first = None
for boundary in (0x160, 0x120):
    for y in range(0x10000):
        for bit0 in (0, 1):
            # CMP.W D0 - target Y; BCC takes the unsigned >= branch.
            site = ((0x1D0EA if bit0 else 0x1D0DC) if boundary >= y
                    else (0x1D0D4 if bit0 else 0x1D0C6))
            want = immediate(site)
            initial = 0xE0 if boundary >= y else 0xA0
            rejected = (initial - (0xC0 if bit0 else 0)) & 255
            candidate = ((0x20 if bit0 else 0xE0) if boundary >= y
                         else (0x60 if bit0 else 0xA0))
            assert candidate == want, (boundary, y, bit0, candidate, want)
            if rejected != want:
                rejected_mismatches += 1
                if first is None:
                    first = dict(boundary=boundary, target_y=y, b5_bit0=bit0,
                                 rejected=rejected, rom=want, pc=hex(site))
            count += 1
assert rejected_mismatches > 0
print(json.dumps(dict(
    scope="angle expression only; ROM byte inspection, no CPU execution",
    cases=count, candidate_mismatches=0,
    rejected_mismatches=rejected_mismatches, first_mismatch=first,
    full_native_oracle="NOT RUN; machine68k and C compiler unavailable",
    production_changed=False,
), indent=2))
