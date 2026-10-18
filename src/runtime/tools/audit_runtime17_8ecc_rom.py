#!/usr/bin/env python3
import hashlib,sys,zipfile
zp=sys.argv[1] if len(sys.argv)>1 else '/mnt/data/wwfwfest.zip'
with zipfile.ZipFile(zp) as z:
    even=z.read('31e14-0.ic18'); odd=z.read('31e13-0.ic19')
rom=bytearray(len(even)*2); rom[0::2]=even; rom[1::2]=odd
sha=hashlib.sha1(rom).hexdigest()
assert sha=='a726c46f350723ea73158bc78055d17d418b14e2',sha
checks={
 0x8ECC:'08390000001c0161677841f9001c05b0303c0003',
 0x8EE0:'082800070000675c0c28000b001d67540c28000c001d674c',
 0x8EF8:'0c2800ff001d660c226800260c29000c001d6738',
 0x8F0C:'0c280005001d66080c28004a00616728',
 0x8F1C:'32280066b2680072671e0668000101060c68008001066510',
 0x8F2C:'0c680080010665104268010606680001',
 0x8F3C:'006606680001006ad1fc0000010c51c8ff944e75',
}
for a,h in checks.items():
    exp=bytes.fromhex(h); got=bytes(rom[a:a+len(exp)])
    assert got==exp,(hex(a),got.hex(),exp.hex())
print('PASS canonical SHA1',sha)
print('PASS $8ECC gates/counter/recovery/stride/DBF ROM signatures')
