#!/usr/bin/env python3
"""Single-track Mode 1: ISO 2048 -> raw BIN 2352 with sync, MSF, EDC and P/Q ECC.

Reference layout/ECC cross-check: MAME src/lib/util/cdrom.cpp (BSD-3-Clause).
https://github.com/mamedev/mame/blob/master/src/lib/util/cdrom.cpp
This is a data-only homebrew image, no CD audio/subchannel/security track.
"""
import argparse,hashlib,json,struct
from pathlib import Path

SYNC=b'\0'+b'\xff'*10+b'\0'
EDC=[];FWD=[0]*256;BACK=[0]*256
for i in range(256):
    v=i
    for _ in range(8):v=(v>>1)^((0xd8018001) if v&1 else 0)
    EDC.append(v)
    FWD[i]=((i<<1)^(0x11d if i&128 else 0))&255
    BACK[i^FWD[i]]=i

def edc(data):
    value=0
    for byte in data:value=(value>>8)^EDC[(value^byte)&255]
    return value

def ecc(data,majors,minors,major_mult,minor_inc):
    size=majors*minors;out=bytearray(majors*2)
    for major in range(majors):
        pos=(major>>1)*major_mult+(major&1);a=b=0
        for _ in range(minors):
            value=data[pos];pos=(pos+minor_inc)%size;a=FWD[a^value];b^=value
        a=BACK[FWD[a]^b];out[major]=a;out[major+majors]=a^b
    return out

def bcd(value):return (value//10)*16+value%10
def sector(payload,lba):
    if len(payload)!=2048 or not 0<=lba<449850:raise ValueError('Invalid sector size/LBA')
    frame=lba+150;minute,rem=divmod(frame,4500);second,frame=divmod(rem,75)
    raw=bytearray(2352);raw[:12]=SYNC;raw[12:16]=bytes([bcd(minute),bcd(second),bcd(frame),1]);raw[16:2064]=payload
    struct.pack_into('<I',raw,2064,edc(raw[:2064]))
    raw[2076:2248]=ecc(raw[12:2076],86,24,2,86)
    raw[2248:2352]=ecc(raw[12:2248],52,43,86,88)
    return raw

def convert(iso,out):
    iso=Path(iso);out=Path(out);size=iso.stat().st_size
    if not size or size%2048:raise ValueError('ISO must contain whole 2048-byte sectors')
    out.mkdir(parents=True,exist_ok=True);binary=out/(iso.stem+'.bin');cue=out/(iso.stem+'.cue')
    if binary.exists() or cue.exists():raise FileExistsError('Do not replace an existing disc package')
    with iso.open('rb') as src,binary.open('xb') as dst:
        for lba in range(size//2048):dst.write(sector(src.read(2048),lba))
    with cue.open('x') as f:f.write(f'FILE "{binary.name}" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n')
    record=verify(iso,binary,cue)
    (out/'disc-validation.json').write_text(json.dumps(record,indent=2)+'\n');return record

def verify(iso,binary,cue):
    data=Path(iso).read_bytes();raw=Path(binary).read_bytes();assert len(raw)==len(data)//2048*2352
    cue_text=Path(cue).read_text();assert f'FILE "{Path(binary).name}" BINARY' in cue_text
    assert 'TRACK 01 MODE1/2352' in cue_text and 'INDEX 01 00:00:00' in cue_text
    for lba in range(len(data)//2048):
        block=raw[lba*2352:(lba+1)*2352]
        assert block==sector(data[lba*2048:(lba+1)*2048],lba),f'Invalid raw sector {lba}'
    assert raw[16:32]==b'SEGA SEGASATURN ' and b'U' in raw[16+0x40:16+0x50]
    assert raw[16*2352+17:16*2352+22]==b'CD001'
    return {'iso_sha256':hashlib.sha256(data).hexdigest(),'bin_sha256':hashlib.sha256(raw).hexdigest(),
            'cue_sha256':hashlib.sha256(cue_text.encode()).hexdigest(),'sectors':len(data)//2048,
            'raw_sector_bytes':2352,'payload_roundtrip':'all sectors byte-identical to ISO',
            'edc_ecc':'all sectors regenerated and compared','usa_region':True,
            'hardware_boot_test':False,'emulator_boot_test':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('iso',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();print(json.dumps(convert(a.iso,a.output),indent=2))
