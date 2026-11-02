from pathlib import Path
import importlib.util,struct,sys
here=Path(__file__).resolve().parent
path=(here.parent/'saturn-real-v25/repo/tools/audio/native_probe/compact_music_elf.py'
      if sys.argv[1]=='red'else here/'src/compact_music_elf.py')
spec=importlib.util.spec_from_file_location('compact_under_test',path)
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
raw=bytearray(576)
raw[:7]=b'\x7fELF\x01\x02\x01'
struct.pack_into('>HHIIIIIHHHHHH',raw,16,2,42,1,0x05c5c020,52,0,2,52,32,4,40,0,0)
loads=[(1,256,0x05c5c000,0x05c5c000,64,64,5,4),
       (1,0,0x05c7c000,0x05c7c000,0,256,6,4),
       (1,0,0x05c7f000,0x05c7f000,0,4096,6,4),
       (1,512,0x06088000,0x06088000,64,3284,7,4)]
for i,load in enumerate(loads):struct.pack_into('>8I',raw,52+32*i,*load)
raw[256:320]=bytes(range(64));raw[512:576]=bytes(range(64,128))
result=module.compact(bytes(raw))
assert len(result)==384 and len(raw)==576
assert struct.unpack_from('>HH',result,42)==(32,4)
assert struct.unpack_from('>I',result,24)[0]==0x05c5c020
assert struct.unpack_from('>I',result,32)[0]==0
for i,old in enumerate(loads):
    new=struct.unpack_from('>8I',result,52+i*32)
    assert new[0]==1 and new[2:7]==old[2:7]
    assert new[7]==4 and (new[1]&3)==(new[2]&3)
    assert result[new[1]:new[1]+new[4]]==raw[old[1]:old[1]+old[4]]
if sys.argv[1]!='red':
    for truncated in (b'',bytes(raw[:51]),bytes(raw[:179]),bytes(raw[:-1])):
        try:module.compact(truncated)
        except ValueError:pass
        else:raise AssertionError('truncated load accepted')
print('PASS four direct loads compacted without changing entry, payloads, memory extents or flags; truncated files rejected')
