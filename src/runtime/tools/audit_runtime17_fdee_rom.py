#!/usr/bin/env python3
import argparse,hashlib,struct
p=argparse.ArgumentParser();p.add_argument('program');a=p.parse_args();b=open(a.program,'rb').read()
assert len(b)==0x80000
assert hashlib.sha1(b).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
# Owner gate, two fixed objects, state dispatcher and exact 5-entry table.
hx=bytes.fromhex('08390001001c0161671041f9001c0f1c610a41f9001c102861024e7543f90000fe223028001c0240000fe548227100004e914e75')
assert b[0xfdee:0xfdee+len(hx)]==hx
ptr=[struct.unpack_from('>I',b,0xfe22+i*4)[0] for i in range(5)]
assert ptr==[0xfe36,0xfe3e,0xfe82,0xff1e,0xff54]
# State 0/1 anchors.
assert b[0xfe36:0xfe3e]==bytes.fromhex('317cffff00044e75')
hx=bytes.fromhex('08280007001c6606317cffff0004226800761169002e002e3169000600063169000a000a3169000e000e06680050000e303c002008280007002e66024440d16800064e75')
assert b[0xfe3e:0xfe3e+len(hx)]==hx
# State 2: first-entry setup, $258E, $2208, two $280DC probes, corrections, bit4 transition, render.
hx=bytes.fromhex('08280007001c662c08e80007001c303c00104eb90000258e30280074c0fc0003314000041168002e0004317c0000001242a800764eb900002208317c0020003e4eb9000280dc4a6800366614317cffe0003e4eb9000280dc4a6800366602602e4268003e30280038d16800063028003ad168000a3028003cd168000e082800040037670a317c0004001c426800364eb90000247c4eb9000027b84e75')
assert b[0xfe82:0xfe82+len(hx)]==hx
# State 3 and state 4 clock/render machine.
hx=bytes.fromhex('08280007001c661c4228000130280074c0fc0003314000041168002e0004317c0002001242a800764eb90000247c4eb9000027b84e75')
assert b[0xff1e:0xff1e+len(hx)]==hx
hx=bytes.fromhex('08280007001c661608e80007001c4228000142680024317c000200126008046800010022664a317c001000220668000100240c6800040024673030280074c0fc000331400004082800000025670e0628000100051168002e0004600c0628000200051168002e00046006317c0003001c4eb90000247c4eb9000027b84e75')
assert b[0xff54:0xff54+len(hx)]==hx
# $280DC column selector consumed by state2 is WORD +$32 bit2.
assert b[0x2813c:0x28146]==bytes.fromhex('38280032028400000004')
# $258E selected rows for fixed object types 0 and 1: caller multiplies +$74 by 3.
assert b[0x25ca:0x25d0]==bytes.fromhex('00c003000038')
assert b[0x25dc:0x25e2]==bytes.fromhex('000003000038')
# Fixed object initializer $FFD2 establishes +$74 0/1 and state 3, validating bounded $258E selection.
hx=bytes.fromhex('41f9001c0f1c317c8000000042680074117c0000002e');assert b[0xffd2:0xffd2+len(hx)]==hx
hx=bytes.fromhex('41f9001c1028317c80000000317c00010074');assert b[0x1000a:0x1000a+len(hx)]==hx
print('PASS RUNTIME-17 $FDEE/$FE22/$258E ROM audit')
