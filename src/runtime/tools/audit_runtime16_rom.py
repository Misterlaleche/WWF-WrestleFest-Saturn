#!/usr/bin/env python3
import argparse,hashlib,struct
p=argparse.ArgumentParser();p.add_argument('program');a=p.parse_args();b=open(a.program,'rb').read()
assert len(b)==0x80000
assert hashlib.sha1(b).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert b[0x0fd0:0x0fd6]==bytes.fromhex('4eb90000f560')
assert b[0x0fd6:0x0fdc]==bytes.fromhex('4eb90000f18a')
assert b[0x0fdc:0x0fe2]==bytes.fromhex('4eb90001c150')
assert b[0x0fe2:0x0fe8]==bytes.fromhex('4eb90001f914')
assert b[0xf560:0xf574]==bytes.fromhex('6100001261000426610007964eb90000a0e04e75')
assert b[0xf18a:0xf196]==bytes.fromhex('42404eb90000250e6500004e')
state=[struct.unpack_from('>I',b,0xf1e4+i*4)[0] for i in range(13)]
assert state==[0xf218,0xf244,0xf34c,0xf45e,0xf460,0xf462,0xf4a4,0xf4a6,0xf4a8,0xf4aa,0xf4ba,0xf4bc,0xf4be]
assert b[0xf98c:0xf994]==bytes.fromhex('08390007001c1678')
assert b[0xa0e0:0xa0e8]==bytes.fromhex('4a39001c15d56616')
assert b[0x1f914:0x1f91a]==bytes.fromhex('41f9001c11f4')
print('PASS RUNTIME-16 canonical ROM audit')
