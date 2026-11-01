"""Read-only ELF/state debt/FIFO/code validation, never loads an emulator."""
from pathlib import Path
import argparse,hashlib,importlib.util,json
base=Path(__file__).resolve().parent;fix=base.parent
p=argparse.ArgumentParser();p.add_argument('--run',type=Path,required=True);a=p.parse_args();run=a.run.resolve()
spec=importlib.util.spec_from_file_location('cold',fix.parent/'emulator/cold_overlay.py');c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
module=c.Elf(base/'music-hot/MUSIC.DEBUG.ELF');master=c.Elf(fix/'saturn-real-v25/master/wrestlefest-runtime18-scsp.elf');transport=c.Elf(base/'music-hot/MUSIC.ELF')
sha=lambda b:hashlib.sha256(b).hexdigest();report=json.loads((run/'report.json').read_text());assert report['existing_media_unchanged']and not report['guest_memory_edits']and not report['state_loads']
assert report['elf']['sha256']==sha(master.data)and report['manifest']['MUSIC.ELF']['sha256']==sha(transport.data)
rows=[]
for path in sorted(run.glob('frame-*.state'),key=lambda p:int(p.stem.split('-')[1])):
 state=path.read_bytes();frame=int(path.stem.split('-')[1]);work,_=c.field(state,'WorkRAMH',1048576);vram,_=c.field(state,'VRAM',524288);mp,_=c.field(state,'DSP.MPROG',1024);sound,_=c.field(state,'RAM',524288,mp)
 def read(address,n):
  address&=0x1fffffff
  for begin,end,at in [(0x06000000,0x06100000,work),(0x05c00000,0x05c80000,vram),(0x05a00000,0x05a80000,sound)]:
   if begin<=address and address+n<=end:return bytes(state[at+((address-begin+i)^1)]for i in range(n))
  raise ValueError(hex(address))
 def u16(a):return int.from_bytes(read(a,2),'big')
 def u32(a):return int.from_bytes(read(a,4),'big')
 def u64(a):return int.from_bytes(read(a,8),'big')
 s=module.symbols;hot=u32(s['_hot'][0]);assert hot==s['_wf_hotcode_end'][0]
 assert u32(0x05c7f000+172)==s['_wf_hotcode_start'][0]and u32(hot+16)==hot
 for off,name in [(0,'_read_byte'),(4,'_write_byte'),(8,'_port_in'),(12,'_port_out'),(2148,'_event')]:assert u32(hot+off)==s[name][0]
 start=s['_wf_hotcode_start'][0];end=hot;segment=next(p for p in module.ph if p[0]==1 and p[2]<=start and end<=p[2]+p[4]);code=module.data[segment[1]+start-segment[2]:segment[1]+end-segment[2]];assert read(start,len(code))==code
 live=u64(hot+2124)+((u32(hot+20)-u32(hot+2120))&0xffffffff);target=u64(s['_target_cycle'][0]);producer=u16(s['_fm_producer'][0]);consumer=u16(s['_fm_consumer'][0]);occupancy=(producer-consumer)&65535;assert occupancy<=512
 dsp=u16(0x05a0b010);oldest=u16(s['_fm_events'][0]+(consumer&511)*8)if occupancy else None
 row=next(r for r in report['observations']if r['frame']==frame)
 rows.append(dict(frame=frame,game_frame=row.get('game_frame'),dropped_fm=row['dropped_fm'],late_count=row['late_count'],music_state=row['music_state'],music_error=row['music_error'],music_events=row['music_events'],sequence_live_cycles=live,target_cycle=target,debt_fs=(target-live)*44100/3579545 if target else None,io_epoch=u32(hot+2160),fm_producer=producer,fm_consumer=consumer,fm_occupancy=occupancy,oldest_when=oldest,oldest_late_fs=((dsp-oldest+32768)&65535)-32768 if oldest is not None else None,dsp_clock=dsp,resident_code_exact=True,workspace_userdata_callbacks_valid=True))
result=dict(master_sha256=sha(master.data),module_sha256=sha(transport.data),module_debug_sha256=sha(module.data),rows=rows,audio_gate=report['audio_gate'],media_created=False,guest_memory_edits=False,state_loads=0)
(run/'DEBT-FIFO-READBACK.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
