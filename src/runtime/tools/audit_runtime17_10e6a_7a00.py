#!/usr/bin/env python3
import hashlib,sys,zipfile
from pathlib import Path
EXPECTED='a726c46f350723ea73158bc78055d17d418b14e2'
def main():
 if len(sys.argv)!=2:
  print('usage: audit_runtime17_10e6a_7a00.py wwfwfest.zip',file=sys.stderr);return 2
 z=zipfile.ZipFile(sys.argv[1]);e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19');m=bytearray(len(e)*2);m[0::2]=e;m[1::2]=o
 sha=hashlib.sha1(m).hexdigest();assert sha==EXPECTED,(sha,EXPECTED)
 anchors={
  0x10e6a:bytes.fromhex('41f9001c1258303c000b08a800070000d1fc0000002a51c8fff24e75'),
  0x7a00:bytes.fromhex('720241f9001c145008e80007000066126100a7a20240007f31400024317c0000001c43f900007a443028001c02400001e540227100004e91d1fc0000002a51c9ffc84e75'),
  0x7a44:bytes.fromhex('00007a4c00007a5c'),
  0x7a4c:bytes.fromhex('0468000100246a0606680001001c4e75'),
  0x7a5c:bytes.fromhex('08e80007001c660000be317c00000024317c00000022317c001d0002317c0140000e08390001001c016167000074'),
  0x7afc:bytes.fromhex('6100a6b648400240003fe74006400190314000066100a6a248400240003f064001e03140000a'),
  0x7b22:bytes.fromhex('0468000100226a22317c000000220668000100240c6800060024650e317c0000001c08a800070000600e6100a92e3168002400046100ac604e75'),
  0x7b5c:bytes.fromhex('7404600274056100a650484002400003e5604e75'),
  0x88a2:bytes.fromhex('41f9001c05b042400828000700566624')
 }
 for a,b in anchors.items():
  got=bytes(m[a:a+len(b)]);assert got==b,(hex(a),got.hex(),b.hex())
 print('PASS canonical main68000 SHA1',sha)
 print('PASS $10E6A exact 12-record BCLR loop')
 print('PASS $7A00 root/dispatch/state0/state1/RNG/render anchors')
 print('PASS next scheduler owner $88A2 anchor')
 return 0
if __name__=='__main__': raise SystemExit(main())
