#!/usr/bin/env python3
import argparse,hashlib,struct
p=argparse.ArgumentParser();p.add_argument('program');a=p.parse_args();b=open(a.program,'rb').read()
assert len(b)==0x80000
assert hashlib.sha1(b).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
# Main scheduler and exact $F518 per-actor order.
assert b[0x0fe8:0x0ffa]==bytes.fromhex('4eb90000f4c24eb9000240624eb900024e58')
hx=bytes.fromhex('426800364a28000167244eb9000280dc4a280037671832280038d36800063228003ad368000a3228003cd368000e4eb90001c03e4eb90000247c4eb9000027b84eb9000022084e75');assert b[0xf518:0xf518+len(hx)]==hx
assert b[0xf510:0xf518]==bytes.fromhex('4eb9000240904e75')
# $280DC preparation and 14-entry mode/bit2 table.
# Column select is MOVE.W +$32 / ANDI.L #4, not byte +$32.
assert b[0x2813c:0x28146]==bytes.fromhex('38280032028400000004')
hx=bytes.fromhex('48e7f0c0426800384268003a4268003c0828000500326600002a302800063228000a3428000ed2680040d468004208280007002e67044468003ed068003e610000084cdf030f4e75');assert b[0x280dc:0x280dc+len(hx)]==hx
ptr=[struct.unpack_from('>I',b,0x28154+i*4)[0] for i in range(14)]
assert ptr==[0x2818e,0x28288,0x2831c,0x2818c,0x28480,0x2851e,0x2818c,0x2818c,0x2818c,0x2818c,0x2818e,0x28288,0x28480,0x2851e]
# Route anchors, including $28480 Z->bit1 (not bit4).
for off,hx in {
 0x2818e:'48e7f080082800000032',
 0x28288:'48e7f0800c4201006418',
 0x2831c:'48e7f080082800000032',
 0x28480:'48e7f0800c4101606718',
 0x2851e:'48e7f0800c4101106418',
}.items(): assert b[off:off+len(bytes.fromhex(hx))]==bytes.fromhex(hx)
assert b[0x284a2:0x284a8]==bytes.fromhex('0c4201406418')
assert b[0x284a8:0x284ae]==bytes.fromhex('08e800010037')
# $2208 three-way motion dispatcher and mode2 decrement/dead clamp sequence.
hx=bytes.fromhex('48e7f040700010280001e34043f9000022244eb100004cdf020f4e756004600460284e75');assert b[0x2208:0x2208+len(hx)]==hx
assert b[0x22a6:0x22be]==bytes.fromhex('9168005c4a68005c6a0e0c682000005c6406317c2000005c')
# Exhaustive condition proof: no 16-bit value can reach MOVE.W #$2000 after TST/BPL then CMP/BCC.
reach=[]
for v in range(65536):
    sv=v if v<0x8000 else v-0x10000
    if sv>=0: continue
    if v>=0x2000: continue
    reach.append(v)
assert not reach
# $1C03E table-domain selection + exact common tail.
hx=bytes.fromhex('48e780480c280005001d660c43f90001261430280060602c0c280004001d660c43f90001afd430280064601843f9000114783028001c0c0000ff660843f9000125c060120c2800ff00216738');assert b[0x1c03e:0x1c03e+len(hx)]==hx
hx=bytes.fromhex('4eb90001c0c80828000700186708426800184268001a4cdf1201');assert b[0x1c0ac:0x1c0ac+len(hx)]==hx
print('PASS RUNTIME-17 F4C2/$280DC/$2208 ROM audit')
