#!/usr/bin/env python3
import hashlib, zipfile, sys
ROM=sys.argv[1] if len(sys.argv)>1 else '/mnt/data/wwfwfest.zip'
with zipfile.ZipFile(ROM) as z:
    even=z.read('31e14-0.ic18'); odd=z.read('31e13-0.ic19')
main=bytearray(len(even)+len(odd)); main[0::2]=even; main[1::2]=odd
sha=hashlib.sha1(main).hexdigest()
exp='a726c46f350723ea73158bc78055d17d418b14e2'
assert sha==exp,(sha,exp)
checks={
0x001018:'4eb9000262d2',
0x0262d2:'4a79001c007c670001a0',
0x0262dc:'08390004001c169e66000194',
0x0262e8:'08390007001c169e67000188',
0x0262f4:'08f90006001c169e66000072',
0x026300:'41f9000c1d04203c00e4009c223c00e400a2343c0005',
0x026334:'23fc00b200e4000c1d2423fc00b200e4000c1e24',
0x026358:'33fc0030001c16a0',
0x026360:'33fc00ff0018071c33fc000f0018071e',
0x026370:'5379001c16a46600010233fc0004001c16a4',
0x026382:'303c00013239001c16a2023c00ee8300',
0x026390:'8300650a13c1001c16a3',
0x02639e:'33fc0059001c16a2',
0x0263a6:'303c00013239001c16a0023c00ee8300',
0x0263b4:'8300652013c1001c16a10c010005',
0x0263c4:'08f90005001c169e303c000b4eb900002052',
0x0263d8:'08f90004001c169e4279001c16a04279001c16a2',
0x026416:'317c800500fe60ea42403039001c16a0',
0x02647c:'207c000c1d1ce5490281000000ffd1c1243c00e400a8',
0x02649a:'243c00e400b3d4404842214201004e75',
}
for a,h in checks.items():
    want=bytes.fromhex(h); got=bytes(main[a:a+len(want)])
    if got!=want:
        print(f'FAIL {a:06X}: got={got.hex()} want={want.hex()}');sys.exit(1)
print('PASS canonical main68000 SHA1',sha)
print('PASS $262D2 callsite/gates/init/BCD/time-up/display signatures')
print('PASS $2647C digit renderer signature')
