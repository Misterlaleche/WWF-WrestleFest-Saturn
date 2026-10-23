#!/usr/bin/env python3
"""Read-only exact-build check of borrowed LWRAM after real Saturn execution.

Uses the normal observer arguments, including optional --profile. Identifies
LWRAM through the complete immutable ring plus the loaded ROM and tile pages,
not a signature from an unrelated CD buffer. No process writes.
"""
import contextlib,hashlib,io,json,re
from pathlib import Path

observer=Path(__file__).with_name('observe-mednafen-runtime18.py')
ns={'__name__':'__main__'};captured=io.StringIO()
with contextlib.redirect_stdout(captured):
    exec(compile(observer.read_text(),str(observer),'exec'),ns)
state=json.loads(captured.getvalue());pid=ns['a'].pid
root=Path(__file__).resolve().parents[1]
assets=root/'src/runtime/audit/runtime18-cd'
ring=(assets/'R18RING.BIN').read_bytes();rom=(assets/'ROM68K.BIN').read_bytes()
hot=(assets/'HOTSPR.BIN').read_bytes();count=int.from_bytes(hot[4:6],'big')
header=16384 if count>4092 else 8192 if count>2044 else 4096;tiles=hot[header:]
pages=[int(x) for x in re.findall(r'\b\d+\b',(root/'src/runtime/include/wf_r18_rom_hot_pages.h').read_text().split('{',1)[1].split('}',1)[0])]
assert 0<len(pages)<=256 and len(tiles)==count*128
rom_cache=b''.join(rom[page*1024:(page+1)*1024] for page in pages)
def swapped(b):
    out=bytearray(b);out[0::2],out[1::2]=b[1::2],b[0::2];return bytes(out)
wanted=swapped(ring);matches=[]
with open(f'/proc/{pid}/mem','rb',buffering=0) as mem:
    def read(at,n):mem.seek(at);return mem.read(n)
    for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
        fields=line.split()
        if not fields[1].startswith('rw') or len(fields)>5 and fields[5] not in ('[heap]',):continue
        lo,hi=(int(x,16) for x in fields[0].split('-'))
        if hi-lo>128*1024*1024:continue
        try:data=read(lo,hi-lo)
        except OSError:continue
        at=data.find(wanted)
        while at>=0:
            base=lo+at-0x1000
            try:
                actual_rom=read(base+0x20000,len(rom_cache))
                actual_tiles=read(base+0x60000,len(tiles))
            except OSError:actual_rom=actual_tiles=b''
            if actual_rom==swapped(rom_cache) and actual_tiles==swapped(tiles):
                staging=swapped(read(base+0x54000,0xc000))
                entry={'address':hex(int.from_bytes(staging[:4],'big')),
                       'value':hex(int.from_bytes(staging[4:8],'big')),
                       'width':staging[8]}
                assert entry['width'] in (1,2,4),'No completed real video staging write'
                matches.append({'host_base':hex(base),'video_staging_first_write':entry})
            at=data.find(wanted,at+2)
assert matches,f'No intact ring+{len(pages)} program pages+{count} hot tiles in the same LWRAM image'
state['LWRAM']={'matches':matches,'ring_bytes_equal':len(ring),'ROM_pages_equal':len(pages),
                'ROM_bytes_equal':len(rom_cache),'hot_sprite_bytes_equal':len(tiles),
                'video_staging_range':['0x00254000','0x00260000'],
                'ROM_sha256':hashlib.sha256(rom_cache).hexdigest(),
                'sprites_sha256':hashlib.sha256(tiles).hexdigest()}
print(json.dumps(state,indent=2))
