#!/usr/bin/env python3
import hashlib,sys,zipfile
zp=sys.argv[1] if len(sys.argv)>1 else '/mnt/data/wwfwfest.zip'
with zipfile.ZipFile(zp) as z:e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
r=bytearray(len(e)*2);r[0::2]=e;r[1::2]=o
assert hashlib.sha1(r).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
checks={
0x9052:'4a79001c007c677a08390005001c169e667041f9001c05b0323c0003',
0x906e:'08280007000067260c6800180066641e08390007001c0166664c303c310b4eb90000205233fc8000001c01666038',
0x909c:'d1fc0000010c51c9ffca08390006001c0166662433fc4000001c016641f900000dee3039001c0162024000ffe340303000004eb9000020524e75',
0x0dee:'3104310a310c310f31063104310a310c310f3106',
0x2052:'48e780004a79001c007c660a08390005001c006766120040310033c00014000c303c004051c8fffe4cdf00014e75'
}
for a,h in checks.items():
 b=bytes.fromhex(h);g=bytes(r[a:a+len(b)]);assert g==b,(hex(a),g.hex(),b.hex())
print('PASS canonical SHA1')
print('PASS $9052 gates/table/$2052 D0-preserve signatures')
