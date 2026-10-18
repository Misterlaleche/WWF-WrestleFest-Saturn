#!/usr/bin/env python3
import argparse, hashlib, sys
from pathlib import Path

SHA1 = "a726c46f350723ea73158bc78055d17d418b14e2"
SIGS = {
    0x0F9A: "4279001c006e",                         # CLR.W $1C006E
    0x0FA0: "61001212",                             # BSR $21B4
    0x100C: "610069f261007890610076fa",             # $7A00->$88A2->$8710
    0x1018: "4eb9000262d24eb90000bb1461006340",     # $262D2->$BB14->$7366
    0x1028: "61007cce4eb9000090526100796861007e94", # $8CF8->$9052->$899C->$8ECC
    0x103A: "610017fa610008844eb9000298ec",         # $2836->$18C4->$298EC
    0x1048: "08f90000001c006f60000096",             # frame-ready then wait
    0x1054: "4eb9000285da4eb900026a42",             # Branch-B preowners
    0x10B0: "610064966100765a61006946610062a8",     # $7548->$8710->$7A00->$7366
    0x10C0: "61007c364eb90000bcfe4eb90000c04a4eb90000c7106100175e4eb9000298ec", # B tail
    0x10E0: "08f90000001c006f08390001001c006f67f6", # BSET bit0; wait bit1
    0x10F2: "52b9001c0080",                         # ADDQ.L #1,$1C0080
    0x114C: "08390000001c0161",                     # Royal mode bit gate
    0x1156: "4eb90002017a",                         # JSR $2017A
    0x1172: "4ef900000f9a",                         # JMP $F9A
    0x1178: "3039001c06500240c000",                 # actor0 result top-bits gate
    0x1186: "3039001c08680240c000",                 # actor2 result top-bits gate
    0x1194: "4ef900000f9a",                         # non-Royal return to $F9A
}
THRESHOLD = bytes.fromhex("035002e0041003a0029003a004b003b0065004800320033003400360")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("rom", type=Path)
    args=ap.parse_args()
    data=args.rom.read_bytes()
    got=hashlib.sha1(data).hexdigest()
    if got != SHA1:
        print(f"FAIL SHA1 {got} != {SHA1}")
        return 1
    bad=0
    for addr, hx in SIGS.items():
        exp=bytes.fromhex(hx); gotb=data[addr:addr+len(exp)]
        if gotb != exp:
            print(f"FAIL ${addr:05X}: got {gotb.hex()} expected {exp.hex()}")
            bad+=1
        else:
            print(f"PASS ${addr:05X} {len(exp)} bytes")
    gotb=data[0x119A:0x119A+len(THRESHOLD)]
    if gotb != THRESHOLD:
        print(f"FAIL $0119A threshold: got {gotb.hex()} expected {THRESHOLD.hex()}")
        bad+=1
    else:
        print("PASS $0119A 14-WORD threshold table")
    if bad:
        print(f"FAIL final-frame ROM audit: {bad} mismatch(es)")
        return 1
    print("PASS final-frame scheduler/VBlank/post-ACK ROM audit")
    return 0

if __name__ == '__main__':
    sys.exit(main())
