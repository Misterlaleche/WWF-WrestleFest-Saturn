#!/usr/bin/env python3
"""Lossless random-access original sprite atlas for a base 2 MiB Saturn.

R18F/R18Z store sorted original tile IDs, block offsets, and independent raw
DEFLATE fixed/default-strategy blocks. R18L uses bounded LZ4 blocks;
R18U stores the same blocks verbatim for the separately labeled RAM-cartridge build.
No sprite pixel, palette, pose or behavior is changed.
Runtime splits compressed blocks across existing LWRAM/HWRAM leases and keeps
decoded blocks in the unused HWRAM tail. SPRITES.PAK remains a cold fallback.
"""
import argparse
import ctypes
import ctypes.util
import hashlib
import json
import struct
import zlib
from pathlib import Path

PACK_SHA256='130ac19d5ab3646f2d51c9d2ce52cd42f53f60b125fd18d674ee8de722c55d31'

def align(n,a):return (n+a-1)//a*a
def read_ids(path):
    return {int(s,16 if s.startswith('0x') else 10)
            for s in path.read_text().splitlines() if s and not s.startswith('#')}

def build(ids,source,block_bytes=4096,codec='fixed'):
    if block_bytes not in (1024,2048,4096,8192,16384):raise ValueError('block width')
    max_ids=32768 if codec=='raw' else 16384
    if not ids or len(ids)>max_ids or ids!=sorted(set(ids)) or min(ids)<0 or max(ids)>65535:
        raise ValueError('original tile IDs')
    raw=b''.join(source[t*128:t*128+128] for t in ids)
    blocks=[];offsets=[0];encoder_version=None
    if codec=='lz4':
        library=ctypes.CDLL(ctypes.util.find_library('lz4'))
        compress=library.LZ4_compress_HC
        compress.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int]
        decompress=library.LZ4_decompress_safe
        decompress.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int]
        encoder_version=library.LZ4_versionNumber()
    for i in range(0,len(raw),block_bytes):
        original=raw[i:i+block_bytes]
        if codec=='raw':
            block=original
        elif codec=='lz4':
            destination=ctypes.create_string_buffer(len(original)*2+64)
            size=compress(original,destination,len(original),len(destination),12)
            assert size>0
            block=destination.raw[:size]
            check=ctypes.create_string_buffer(len(original))
            assert decompress(block,check,len(block),len(original))==len(original)
            assert check.raw==original
        elif codec in ('fixed','dynamic'):
            strategy=zlib.Z_FIXED if codec=='fixed' else zlib.Z_DEFAULT_STRATEGY
            c=zlib.compressobj(9,zlib.DEFLATED,-15,9,strategy)
            block=c.compress(original)+c.flush()
            assert zlib.decompress(block,-15)==original
            encoder_version=zlib.ZLIB_VERSION
        else:
            raise ValueError('unknown codec')
        blocks.append(block);offsets.append(offsets[-1]+len(block))
    offset_at=align(32+2*len(ids),32)
    # Raw cartridge data has no decoded-block map. Its only payload bound is
    # the physical4MiB destination; compressed formats keep the512-block map.
    max_blocks=4096 if codec=='raw' else 512
    if len(blocks)>max_blocks:raise ValueError(f'Use larger blocks: {codec} supports at most{max_blocks} independent blocks')
    metadata_bytes=align(offset_at+len(offsets)*4,2048)
    header=bytearray(metadata_bytes)
    magic={'lz4':b'R18L','fixed':b'R18F','dynamic':b'R18Z','raw':b'R18U'}[codec]
    struct.pack_into('>4sHH6I',header,0,magic,len(ids),128,block_bytes,len(blocks),
                     offsets[-1],metadata_bytes,len(raw),1)
    struct.pack_into('>'+str(len(ids))+'H',header,32,*ids)
    struct.pack_into('>'+str(len(offsets))+'I',header,offset_at,*offsets)
    return bytes(header)+b''.join(blocks),{
        'format':magic.decode(),'version':1,'tiles':len(ids),'raw_bytes':len(raw),
        'codec':codec,'encoder_version':encoder_version,
        'block_bytes':block_bytes,'blocks':len(blocks),'metadata_bytes':metadata_bytes,
        'compressed_bytes':offsets[-1],'raw_sha256':hashlib.sha256(raw).hexdigest(),
        'source_sprite_pack_sha256':hashlib.sha256(source).hexdigest()}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ids',type=Path,nargs='+',required=True)
    p.add_argument('--sprites',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--block-bytes',type=int,default=4096)
    p.add_argument('--codec',choices=['fixed','lz4','dynamic','raw'],default='fixed')
    a=p.parse_args()
    source=a.sprites.read_bytes()
    if len(source)!=8388608 or hashlib.sha256(source).hexdigest()!=PACK_SHA256:
        raise SystemExit('Original SPRITES.PAK identity mismatch')
    selected=set()
    for path in a.ids:selected.update(read_ids(path))
    data,report=build(sorted(selected),source,a.block_bytes,a.codec)
    a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_bytes(data)
    report.update({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
                   'tile_id_sources':[str(x) for x in a.ids]})
    a.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
