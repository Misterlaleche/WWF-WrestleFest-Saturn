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
    source=observer.read_text().replace('candidates.append(values)',"values['_host_ram_base']=base; candidates.append(values)")
    exec(compile(source,str(observer),'exec'),ns)
state=json.loads(captured.getvalue());pid=ns['a'].pid
hwr_base=state.pop('_host_ram_base')
root=Path(__file__).resolve().parents[1]
assets=root/'src/runtime/audit/runtime18-cd'
memory=(root/'src/runtime/include/wf_saturn_memory.h').read_text()
def memory_constant(name):
    return int(re.search(r'#define '+name+r'\s+(0x[0-9a-f]+)u',memory)[1],16)
rom_at=memory_constant('WF_SATURN_LWRAM_ROM_CACHE')-0x00200000
rom_capacity=memory_constant('WF_SATURN_LWRAM_ROM_BYTES')
tiles_at=memory_constant('WF_SATURN_LWRAM_TILE_CACHE')-0x00200000
tiles_capacity=memory_constant('WF_SATURN_LWRAM_TILE_BYTES')
staging_at=memory_constant('WF_SATURN_LWRAM_VIDEO_WRITES')-0x00200000
staging_bytes=memory_constant('WF_SATURN_LWRAM_VIDEO_WRITE_BYTES')
index_capacity=memory_constant('WF_SATURN_HWRAM_HOT_INDEX_BYTES')
scene_at=memory_constant('WF_SATURN_LWRAM_SCENE')-0x00200000
scenes={name:(assets/name).read_bytes() for name in ('R18RING.BIN','R18MAP2.BIN')}
assert all(len(scene)<=memory_constant('WF_SATURN_LWRAM_SCENE_BYTES') for scene in scenes.values())
rom=(assets/'ROM68K.BIN').read_bytes()
hot=(assets/'HOTSPR.BIN').read_bytes();count=int.from_bytes(hot[4:6],'big')
sprite_format=hot[:4].decode('ascii')
chunk_count=0
if sprite_format=='R18H':
    header=16384 if count>4092 else 8192 if count>2044 else 4096
    assert len(hot)==header+count*128
    logical=hot[header:]
elif sprite_format=='R18D':
    chunk_count=int.from_bytes(hot[8:12],'big')
    map_bytes=int.from_bytes(hot[12:16],'big')
    dict_bytes=int.from_bytes(hot[16:20],'big')
    header=int.from_bytes(hot[20:24],'big')
    assert header in (4096,8192,16384) and 0<count<=(header-24)//2
    assert hot[6:8]==b'\x00\x80' and map_bytes in (count*8,count*16,count*64)
    chunk_bytes=128//(map_bytes//count//2)
    assert chunk_bytes in (4,16,32)
    assert 0<chunk_count<=min(65536,count*128//chunk_bytes) and dict_bytes==chunk_count*chunk_bytes
    dict_at=(map_bytes+31)&~31
    assert len(hot)==header+dict_at+dict_bytes
    payload=hot[header:];dictionary=payload[dict_at:]
    indices=[int.from_bytes(payload[o:o+2],'big') for o in range(0,map_bytes,2)]
    assert all(i<chunk_count for i in indices)
    logical=b''.join(dictionary[i*chunk_bytes:(i+1)*chunk_bytes] for i in indices)
    pack=(assets/'SPRITES.PAK').read_bytes()
    ids=[int.from_bytes(hot[24+i*2:26+i*2],'big') for i in range(count)]
    assert ids==sorted(set(ids))
    assert logical==b''.join(pack[i*128:(i+1)*128] for i in ids), 'Dictionary differs from original sprite pixels'
else:raise AssertionError('Unknown hot sprite format')
index_prefix=min(dict_at,index_capacity) if sprite_format=='R18D' else 0
index_spill=dict_at-index_prefix if sprite_format=='R18D' else 0
tiles=hot[header:] if sprite_format=='R18H' else hot[header+index_prefix:]
assert (len(tiles)+127)//128*128+128<=tiles_capacity
header_symbol=re.search(r'\.bss\.hot_header(?:\.lto_priv\.\d+)?\s+0x([0-9a-f]+)\s',ns['mapping'])
assert header_symbol, 'Hot sprite header symbol missing from exact map'
pages=[int(x) for x in re.findall(r'\b\d+\b',(root/'src/runtime/include/wf_r18_rom_hot_pages.h').read_text().split('{',1)[1].split('}',1)[0])]
assert 0<len(pages)<=rom_capacity//1024 and len(logical)==count*128
rom_cache=b''.join(rom[page*1024:(page+1)*1024] for page in pages)
def swapped(b):
    out=bytearray(b);out[0::2],out[1::2]=b[1::2],b[0::2];return bytes(out)
matches=[]
with open(f'/proc/{pid}/mem','rb',buffering=0) as mem:
    def read(at,n):mem.seek(at);return mem.read(n)
    actual_header=read(hwr_base+int(header_symbol[1],16)-0x06000000,header)
    assert actual_header==swapped(hot[:header]), 'Loaded hot sprite IDs/header differ'
    if sprite_format=='R18D':
        index_symbol=re.search(r'\.bss\.hot_indices(?:\.lto_priv\.\d+)?\s+0x([0-9a-f]+)\s',ns['mapping'])
        assert index_symbol, 'Dictionary map symbol missing from exact build'
        actual_indices=read(hwr_base+int(index_symbol[1],16)-0x06000000,min(map_bytes,index_prefix))
        assert actual_indices==swapped(hot[header:header+min(map_bytes,index_prefix)]), 'Loaded HWRAM dictionary indices differ'
    for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
        fields=line.split()
        if not fields[1].startswith('rw') or len(fields)>5 and fields[5] not in ('[heap]',):continue
        lo,hi=(int(x,16) for x in fields[0].split('-'))
        if hi-lo>128*1024*1024:continue
        try:data=read(lo,hi-lo)
        except OSError:continue
        for name,ring in scenes.items():
            wanted=swapped(ring);at=data.find(wanted)
            while at>=0:
                base=lo+at-scene_at
                try:
                    actual_rom=read(base+rom_at,len(rom_cache))
                    actual_tiles=read(base+tiles_at,len(tiles))
                except OSError:actual_rom=actual_tiles=b''
                if actual_rom==swapped(rom_cache) and actual_tiles==swapped(tiles):
                    staging=swapped(read(base+staging_at,staging_bytes))
                    entry={'address':hex(int.from_bytes(staging[:4],'big')),
                           'value':hex(int.from_bytes(staging[4:8],'big')),
                           'width':staging[8]}
                    assert entry['width'] in (1,2,4),'No completed real video staging write'
                    matches.append({'host_base':hex(base),'scene_asset':name,'scene_bytes_equal':len(ring),'video_staging_first_write':entry})
                at=data.find(wanted,at+2)
assert matches,f'No intact ring+{len(pages)} program pages+{count} hot tiles in the same LWRAM image'
state['LWRAM']={'matches':matches,'ROM_pages_equal':len(pages),
                'ROM_bytes_equal':len(rom_cache),'hot_sprite_bytes_equal':len(tiles),'hot_sprite_header_bytes_equal':header,
                'hot_sprite_format':sprite_format,'logical_tile_count':count,
                'logical_sprite_bytes':len(logical),'dictionary_chunk_count':chunk_count,
                'dictionary_chunk_bytes':chunk_bytes if sprite_format=='R18D' else 0,
                'hot_sprite_index_bytes_equal':map_bytes if sprite_format=='R18D' else 0,
                'hot_sprite_index_HWRAM_bytes_equal':min(map_bytes,index_prefix) if sprite_format=='R18D' else 0,
                'hot_sprite_index_LWRAM_bytes_equal':index_spill,
                'logical_sprite_sha256':hashlib.sha256(logical).hexdigest(),
                'video_staging_range':[hex(0x00200000+staging_at),hex(0x00200000+staging_at+staging_bytes)],
                'ROM_sha256':hashlib.sha256(rom_cache).hexdigest(),
                'sprites_sha256':hashlib.sha256(tiles).hexdigest()}
print(json.dumps(state,indent=2))
