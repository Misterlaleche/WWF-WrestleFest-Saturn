#!/usr/bin/env python3
import hashlib,sys,zipfile,struct
zp=sys.argv[1] if len(sys.argv)>1 else '/mnt/data/wwfwfest.zip'
with zipfile.ZipFile(zp) as z:e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
r=bytearray(len(e)*2);r[0::2]=e;r[1::2]=o
sha=hashlib.sha1(r).hexdigest();assert sha=='a726c46f350723ea73158bc78055d17d418b14e2'
checks={
0x899c:'08390000001c167b660000d648e700e008390007001c167a661408390007001c167b670000b808f90006001c167a',
0x89ca:'06790001001c00aa0c790002001c00aa650e4279001c00aa0a790001001c00b241f9000c1008303c00004281',
0x89f4:'42813200e54143f900008a7e4a39001c00b3670406410002323110003401e04204420001024100ff2448',
0x8a1e:'08390006001c167a670824bcff00ff00600a15410001157c00e4000306410001d5fc0000000451caffd8',
0x8a48:'d1fc000001003200e54147f900008ab222331000d1c1064000010c40000d658c13f9001c167a001c167b4239001c167a4cdf07004e75'
}
for a,h in checks.items():
 b=bytes.fromhex(h);g=bytes(r[a:a+len(b)]);assert g==b,(hex(a),g.hex(),b.hex())
pairs=[struct.unpack('>HH',r[0x8a7e+i*4:0x8a82+i*4]) for i in range(13)]
assert pairs==[(0x700,0x707),(0x70e,0x715),(0x31c,0x31f),(0x522,0x527),(0x52c,0x531),(0x836,0x83e),(0x846,0x84e),(0x856,0x85e),(0x766,0x76d),(0x774,0x77b),(0x782,0x789),(0x590,0x596),(0x195,0x19b)]
adj=[struct.unpack('>i',r[0x8ab2+i*4:0x8ab6+i*4])[0] for i in range(13)]
assert adj==[0,8,-4,0,-8,0,0,4,-4,0,0,-232,0]
print('PASS canonical SHA1',sha)
print('PASS $899C gates/13-row loop/$8A7E/$8AB2 signatures')
