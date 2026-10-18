#!/usr/bin/env python3
import argparse, hashlib, struct
p=argparse.ArgumentParser()
p.add_argument('program')
a=p.parse_args()
b=open(a.program,'rb').read()
assert len(b)==0x80000
assert hashlib.sha1(b).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert b[0x1f914:0x1f91a]==bytes.fromhex('41f9001c11f4')
assert b[0x1f92e:0x1f940]==bytes.fromhex('4eb90001faf04eb90000247c4eb9000027b8')
primary=[struct.unpack_from('>I',b,0x1f952+i*4)[0] for i in range(11)]
assert primary==[0x1f97e,0x1f9e0,0x1f9f8,0x1fa82,0x1fa40,0x1f99e,0x1f9d8,0x1f9f8,0x1fa40,0x1faec,0x1faec]
action=[struct.unpack_from('>I',b,0x1fb1a+i*4)[0] for i in range(11)]
assert action==[0x1fb46,0x1fc22,0x1fc22,0x1fd5e,0x1fc22,0x1ff52,0x20022,0x202ba,0x20354,0x203ee,0x1ff52]
assert b[0x1faf0:0x1fafe]==bytes.fromhex('082800070020670c08a800070020')
assert b[0x20022:0x20038]==bytes.fromhex('08e80007001c6600002e4279001c169a42680024317c')
# Critical direct-ROM correction: CLR.W +$24(A0), which clears bytes +$24 and +$25.
assert b[0x20032:0x20036]==bytes.fromhex('42680024')
assert b[0x133a2:0x133a8]==bytes.fromhex('08e800000035')
assert b[0x20556:0x2055e]==bytes.fromhex('4a79001c007c6742')
assert b[0x2188:0x21ac]==bytes.fromhex('0c68017f00146c1e0c68ffc100146d160c68013f00166c0e0c68ffc100166d06023c00ee')
assert b[0xf8e4:0xf8ea]==bytes.fromhex('48e780c043f9')
print('PASS R17 referee ROM audit')
print('PASS $20032 = CLR.W +$24(A0): +$24 and +$25 are cleared on first entry')
