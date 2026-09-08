#!/usr/bin/env python3
import zipfile,sys
p=sys.argv[1] if len(sys.argv)>1 else 'wwfwfest.zip'
with zipfile.ZipFile(p) as z:
    e=z.read('31e14-0.ic18'); o=z.read('31e13-0.ic19')
rom=bytearray(len(e)+len(o)); rom[0::2]=e; rom[1::2]=o

def eq(a,h):
    b=bytes.fromhex(h); got=bytes(rom[a:a+len(b)])
    assert got==b,(hex(a),got.hex(),b.hex())

eq(0x15d08,'47 f9 00 01 5d 20 30 28 00 44 02 40 00 0f e5 48 26 73 00 00 4e 93')
ptr=[int.from_bytes(rom[0x15d20+i*4:0x15d24+i*4],'big') for i in range(9)]
assert ptr==[0x15d44,0x15db8,0x15db8,0x15db8,0x15db8,0x175fc,0x176c4,0x1770c,0x15e42],ptr

eq(0x15d94,'31 7c 00 05 00 20 06 68 00 01 00 44')
eq(0x15da0,'08 39 00 01 00 1c 01 61 67 06 31 7c 00 08 00 44')
eq(0x15de2,'31 7c 05 00 00 5c 31 7c 00 48 00 5e')
eq(0x15df4,'11 7c 00 02 00 01 60 00 00 44')
eq(0x15dfe,'08 28 00 04 00 37')
eq(0x15e0c,'42 28 00 01 42 68 00 22')
eq(0x15e24,'31 7c 00 05 00 20 0c 28 00 01 00 36 67 08 06 68 00 01 00 44 60 06 31 7c 00 05 00 44')
eq(0x176a0,'31 7c 00 05 00 20 52 68 00 44')
eq(0x176fa,'31 7c 00 05 00 20 52 68 00 44')
eq(0x17776,'31 7c 00 05 00 20 52 68 00 44')
eq(0x15e42,'08 28 00 07 00 60')
eq(0x15e58,'31 7c 06 00 00 5c 31 7c 00 48 00 5e 30 3c 00 02')
eq(0x15ea8,'31 7c 00 20 00 42')
eq(0x15eae,'08 28 00 04 00 37')
eq(0x15ebe,'42 68 00 42')
eq(0x0f9d6,'08 f9 00 01 00 1c 01 61')
eq(0x0f9f4,'08 b9 00 01 00 1c 01 61')
print('PASS Test426: raw23 +$44 subdispatcher separates initial held gate (+44=1..4, motion 0500/0048/2) from +44=8 later probe/contact route (0600/0048/2)')
