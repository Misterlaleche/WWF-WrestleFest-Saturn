#!/usr/bin/env python3
from __future__ import annotations
import hashlib,sys,zipfile
EXPECTED='a726c46f350723ea73158bc78055d17d418b14e2'

def load(p):
    with zipfile.ZipFile(p) as z:
        e=z.read('31e14-0.ic18'); o=z.read('31e13-0.ic19')
    b=bytearray(0x80000); b[0::2]=e; b[1::2]=o
    b=bytes(b); assert hashlib.sha1(b).hexdigest()==EXPECTED; return b

def w(b,a): return int.from_bytes(b[a:a+2],'big')

def main():
    if len(sys.argv)!=2: raise SystemExit('usage: test410_actor32_raw10_rom.py wwfwfest.zip')
    b=load(sys.argv[1])
    by_role={('BCLR',0):[],('BSET',0):[],('BTST',0):[],('BCLR',1):[],('BSET',1):[],('BTST',1):[],('BCLR',2):[],('BSET',2):[],('BTST',2):[]}
    for pc in range(0,0x30000,2):
        op=w(b,pc); mode=(op>>3)&7; reg=op&7
        if mode==5 and (op&0xff00)==0x0800 and w(b,pc+4)==0x0032 and (w(b,pc+2)&7)==2:
            k={0:'BTST',1:'BCHG',2:'BCLR',3:'BSET'}[(op>>6)&3]
            if (k,reg) in by_role: by_role[(k,reg)].append(pc)
    assert by_role[('BCLR',0)]==[0x114CC,0x11E04]
    assert by_role[('BSET',0)]==[0x0F3BC,0x1C4CA,0x1C52E,0x1CED0,0x1D85A,0x1F354]
    assert by_role[('BSET',2)]==[0x214A4]
    assert by_role[('BTST',0)]==[0x1D938]
    assert by_role[('BTST',1)]==[0x2414C]
    clears={0x588A:'42 68 00 32',0x74B0:'31 7c 00 07 00 32',0x9446:'31 7c 80 03 00 32',0xFAD2:'02 68 00 c3 00 32',0xFB2A:'02 68 00 c3 00 32',0xFBFA:'02 68 00 c3 00 32',0x10000:'31 7c 00 04 00 32',0x10034:'31 7c 00 04 00 32',0x1052A:'42 68 00 32',0x108BE:'31 7c 00 01 00 32',0x10930:'31 7c 00 03 00 32',0x10A48:'31 7c 00 01 00 32',0x125FC:'02 68 00 87 00 32',0x1A39C:'31 7c 10 00 00 32',0x1C2E8:'02 68 fb a7 00 32',0x6008:'42 69 00 32',0xBCB0:'33 7c 80 05 00 32',0x104B6:'42 69 00 32',0x2029C:'42 69 00 32'}
    for a,h in clears.items():
        q=bytes.fromhex(h); assert b[a:a+len(q)]==q,hex(a)
    assert b[0x985C:0x9862]==bytes.fromhex('00 68 00 03 00 32')
    assert int.from_bytes(b[0x1C1F4:0x1C1F8],'big')==0x1C2CC
    assert b[0x1C2E8:0x1C2EE]==bytes.fromhex('02 68 fb a7 00 32')
    tab=0x12614
    d=int.from_bytes(b[tab+0x7A*4:tab+0x7A*4+4],'big'); assert d==0x1A348
    assert int.from_bytes(b[d:d+4],'big')==0x1A364
    assert b[0x1A39C:0x1A3A2]==bytes.fromhex('31 7c 10 00 00 32')
    d=int.from_bytes(b[tab+0x10*4:tab+0x10*4+4],'big'); assert d==0x141D8
    assert int.from_bytes(b[d:d+4],'big')==0x141EC
    for a,h in {0xE382:'22 68 00 7a',0xE386:'08 28 00 05 00 34',0xE390:'0c 28 00 00 00 21',0xE398:'0c 28 00 01 00 21',0xE3A2:'4a 28 00 af',0xE3AA:'4a a9 00 26',0xE3B2:'0c 29 00 04 00 21',0xE3BC:'0c 29 00 01 00 65',0xE3FC:'30 3c 00 10'}.items():
        q=bytes.fromhex(h); assert b[a:a+len(q)]==q,hex(a)
    print('PASS Test410 actor32/raw10 corrected ROM audit')
if __name__=='__main__': main()
