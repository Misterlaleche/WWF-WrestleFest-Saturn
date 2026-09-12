#!/usr/bin/env python3
import hashlib, zipfile, sys
from pathlib import Path
rom=Path(sys.argv[1] if len(sys.argv)>1 else '/mnt/data/wwfwfest.zip')
with zipfile.ZipFile(rom) as z:
    even=z.read('31e14-0.ic18'); odd=z.read('31e13-0.ic19')
p=bytearray(len(even)*2); p[0::2]=even; p[1::2]=odd
assert hashlib.sha1(p).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
assert hashlib.sha256(p).hexdigest()=='265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'

def sig(addr,h):
    b=bytes.fromhex(h); assert bytes(p[addr:addr+len(b)])==b,(hex(addr),p[addr:addr+len(b)].hex(),h)

# A654 visible ordering / TAG-only helper / entry audio / actor/referee/render/sync/input.
sig(0xA654,'0c 39 00 00 00 1c 01 63 66 08 06 79 00 80 00 1c 11 fa')
sig(0xA666,'08 39 00 00 00 1c 01 61 66 06 4e b9 00 00 98 ba')
sig(0xA676,'30 3c 31 32 4e b9 00 00 20 52')
sig(0xA6CC,'30 3c 00 08 41 f9 00 1c 05 b0')
sig(0xA702,'41 f9 00 1c 11 f4 4e b9 00 00 24 7c 4e b9 00 00 27 b8')
sig(0xA714,'4e b9 00 00 28 36')
sig(0xA732,'30 39 00 14 00 20 46 40 02 40 00 30')
# A814 ownership / activation predicate / id0A second record.
sig(0xA85A,'15 7c 00 01 00 1d')
sig(0xA860,'30 3c 30 32 32 2a 00 56 0c 01 00 05 65 04 30 3c 30 31')
sig(0xA878,'0c 2a 00 0a 00 57 66 0c d5 fc 00 00 01 0c 15 7c 00 01 00 1d')
sig(0xA88C,'42 68 00 22 31 7c 00 0c 00 04')
sig(0xA8B2,'4a 79 00 1c 01 68 67 06 06 28 00 04 00 1d')
# Presenter terminal visual callbacks.
sig(0xA8EC,'08 e8 00 07 00 1c')
sig(0xA952,'08 e8 00 07 00 1c')
sig(0xA99A,'08 e8 00 07 00 1c')
# AAEA voice73 latch and shared RNG.
sig(0xAB48,'42 79 00 1c 01 68 0c 00 00 73 66 08 33 fc 80 00 00 1c 01 68')
sig(0xAB78,'4e b9 00 00 21 b4 02 40 00 0f')
# Local program tables and terminal clear.
sig(0xAD0E,'45 f9 00 00 ad 6c')
sig(0xAD66,'42 68 00 1c 4e 75')
# Royal setup fields and tables.
sig(0x1086C,'48 e7 f0 e0 08 39 00 00 00 1c 01 61')
assert [(p[0x10ACC+4*i]<<8)|p[0x10ACD+4*i] for i in range(6)]==[0x240,0x2d0,0x230,0x2c0,0x240,0x2b0]
assert [(p[0x10ACE+4*i]<<8)|p[0x10ACF+4*i] for i in range(6)]==[0x120,0x120,0x150,0x150,0x180,0x180]
assert [(p[0x10AE4+2*i]<<8)|p[0x10AE5+2*i] for i in range(6)]==[0x8000,0,0x8000,0,0x8000,0]
# Referee setup.
sig(0x10718,'41 f9 00 1c 11 f4 31 7c 80 00 00 00 31 7c 00 0c 00 02')
# Entry and teardown / post-route.
sig(0xBD2,'42 79 00 1c 01 66 4e b9 00 00 8e b6')
sig(0xBE8,'0c 39 00 09 00 1c 01 63 66 10 30 3c 00 00 4e b9 00 00 ae 20')
sig(0xC08,'08 39 00 00 00 1c 01 61')
sig(0xC38,'08 f9 00 06 00 1c 01 60')
sig(0xA778,'30 3c 00 1f 4e b9 00 00 2b 58 4e b9 00 00 9a 42')
sig(0xA7A2,'4e b9 00 00 8e b6 30 3c 31 2f 4e b9 00 00 20 52')
sig(0x8EB6,'30 3c 31 00 4e b9 00 00 20 52 30 3c 31 20 4e b9 00 00 20 52 4e 75')
sig(0x8AE6,'33 fc ff ff 00 1c 00 72')
assert [(p[0xDEE+2*i]<<8)|p[0xDEF+2*i] for i in range(10)]==[0x3104,0x310a,0x310c,0x310f,0x3106]*2
print('WP12 R02 direct ROM audit: PASS')
