"""Check target-memory selection, including ABI4 HWRAM executable loads."""
from pathlib import Path
import hashlib,sys,types
here=Path(__file__).resolve().parent
source=(here/'cold_overlay.py').read_text()
if '--patched' in sys.argv:
    from observer_identity import direct_load_identity
    source=direct_load_identity(source)
ns={'__file__':str(here/'cold_overlay.py'),'__name__':'identity_fixture'}
exec(compile(source,str(here/'cold_overlay.py'),'exec'),ns)
state=bytearray(2*1048576)
regions={'DSP.MPROG':0,'RAM':0,'VRAM':524288,'WorkRAMH':1048576}
ns['field']=lambda state,name,size,*args:(regions[name],size)
def add(region,relative,payload):
    swapped=bytearray(payload);swapped[::2],swapped[1::2]=payload[1::2],payload[::2]
    at=regions[region]+relative;state[at:at+len(swapped)]=swapped
def load(address,payload):return(1,0,address,address,len(payload),len(payload),5,4)
sound=bytes(range(64));video=bytes(range(64,128));work=bytes(range(128,192))
add('RAM',0,sound);add('VRAM',0x5c000,video);add('WorkRAMH',0x88b30,work)
music=video+work
elves={'SCSP.ELF':types.SimpleNamespace(data=sound,ph=[load(0,sound)]),
       'MUSIC.ELF':types.SimpleNamespace(data=music,ph=[load(0x05c5c000,video),
                    (1,64,0x06088b30,0x06088b30,64,64,7,4)])}
report=ns['code_identity'](bytes(state),elves)
assert len(report['SCSP.ELF']['executable_samples'])==1
assert len(report['MUSIC.ELF']['executable_samples'])==2
assert all(row['matches'] for item in report.values() for row in item['executable_samples']),report
print('PASS SCSP/VDP1/HWRAM direct executable identity uses the correct target region and byte order')
