#!/usr/bin/env python3
import hashlib,sys
p=sys.argv[1] if len(sys.argv)>1 else '/mnt/data/main68000.bin'
b=open(p,'rb').read()
sha1=hashlib.sha1(b).hexdigest()
assert sha1=='a726c46f350723ea73158bc78055d17d418b14e2',sha1
segs={
'init1004c':(0x1004c,0x1011a,'a231de46c03e65dc6556a44bebe65e0e9accc31ea8b5b2aed230ff99100ee75d'),
'owner10120':(0x10120,0x10162,'1527cba4d606d1e9c1e7b4a394cf7a7a68a53403be8dfd3994251764727dbacc'),
'tables10162':(0x10162,0x10186,'b4ebe07132204c98bd3a9b66b70f00ee9de0b2db485c5016c052d86a50878e8d'),
'state0_10186':(0x10186,0x101b2,'569060c33da6e09e9659718a7a5d418bb1c1998eb72478dfed41257cf5ecce1e'),
'state1_101b2':(0x101b2,0x10228,'358abf5203d71f5a828acbbc94505c97fbc63631aecb9f72ef0854046e3f2eb8'),
'state2_10228':(0x10228,0x1029e,'978102a98e2f238e903bf2de55546f127e541d9acd29ee2ca0bd48502706247f'),
'state3_1029e':(0x1029e,0x102f6,'ffb3efa52d905edf5a47b27632b9a21330cd4c4a28031cd312e3917236d6317c'),
'state4_102f6':(0x102f6,0x1034a,'c36495937f7d688098dd7faafb8e68abd825cd65d4b845cd9b2d0ad0f2aa928e')}
for n,(a,z,h) in segs.items():
 got=hashlib.sha256(b[a:z]).hexdigest();assert got==h,(n,got,h)
# Exact fixed object table and state-handler table.
assert b[0x10162:0x10172].hex()=='001c1134001c1164001c1194001c11c4'
assert b[0x10172:0x10186].hex()=='00010186000101b2000102280001029e000102f6'
print('PASS RUNTIME-17 $10120/$1004C fixed-four ROM audit')
