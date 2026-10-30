#!/usr/bin/env python3
"""Deterministic single-track Mode 1 ISO/2048 -> raw BIN/2352 conversion."""
import argparse,hashlib,json,struct
from pathlib import Path
SYNC=b'\0'+b'\xff'*10+b'\0'
EDC=[];FWD=[0]*256;BACK=[0]*256
for i in range(256):
    v=i
    for _ in range(8): v=(v>>1)^(0xd8018001 if v&1 else 0)
    EDC.append(v);FWD[i]=((i<<1)^(0x11d if i&128 else 0))&255;BACK[i^FWD[i]]=i

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

def bcd(v):return (v//10)*16+v%10

def sector(payload,lba):
    if len(payload)!=2048:raise ValueError('payload must be exactly 2048 bytes')
    if not 0<=lba<449850:raise ValueError('LBA out of supported CD range')
    frame=lba+150;minute,rem=divmod(frame,4500);second,frame=divmod(rem,75)
    raw=bytearray(2352);raw[:12]=SYNC;raw[12:16]=bytes([bcd(minute),bcd(second),bcd(frame),1]);raw[16:2064]=payload
    struct.pack_into('<I',raw,2064,edc(raw[:2064]));raw[2076:2248]=ecc(raw[12:2076],86,24,2,86);raw[2248:2352]=ecc(raw[12:2248],52,43,86,88)
    return bytes(raw)

def convert(iso,out_dir):
    iso=Path(iso);out_dir=Path(out_dir);size=iso.stat().st_size
    if not size or size%2048:raise ValueError('ISO must contain whole 2048-byte sectors')
    out_dir.mkdir(parents=True,exist_ok=True);binary=out_dir/(iso.stem+'.bin');cue=out_dir/(iso.stem+'.cue')
    if binary.exists() or cue.exists():raise FileExistsError('refusing to replace existing BIN/CUE')
    with iso.open('rb') as src,binary.open('xb') as dst:
        for lba in range(size//2048):dst.write(sector(src.read(2048),lba))
    cue.write_text(f'FILE "{binary.name}" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n')
    return {'iso_sha256':hashlib.sha256(iso.read_bytes()).hexdigest(),'bin_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'cue_sha256':hashlib.sha256(cue.read_bytes()).hexdigest(),'sectors':size//2048}

def main():
    p=argparse.ArgumentParser();p.add_argument('iso',type=Path);p.add_argument('out_dir',type=Path);a=p.parse_args();print(json.dumps(convert(a.iso,a.out_dir),indent=2))
if __name__=='__main__':main()
