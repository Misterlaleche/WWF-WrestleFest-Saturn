"""Read-only saved-state sequence debt and coarse FM drop checkpoint bounds."""
from pathlib import Path
import importlib.util,json,sys
here=Path(__file__).resolve().parent;run=here/(sys.argv[1]if len(sys.argv)>1 else 'cold-4200')
sp=importlib.util.spec_from_file_location('cold',here.parent.parent/'emulator/cold_overlay.py')
c=importlib.util.module_from_spec(sp);sp.loader.exec_module(c)
e=c.Elf(here/'music-hot/MUSIC.DEBUG.ELF');r=json.loads((run/'report.json').read_text());rows=[]
for f in sorted(run.glob('frame-*.state'),key=lambda x:int(x.stem.split('-')[1])):
 d=f.read_bytes();v,_=c.field(d,'VRAM',524288);w,_=c.field(d,'WorkRAMH',1048576)
 def u(a,n=4):
  off=v+a-0x05c00000 if a<0x06000000 else w+a-0x06000000
  return int.from_bytes(bytes(d[off+(i^1)]for i in range(n)),'big')
 hot=u(e.symbols['_hot'][0]);target=u(e.symbols['_target_cycle'][0],8)
 live=u(hot+2124,8)+(u(hot+20)-u(hot+2120))%2**32
 producer=u(e.symbols['_fm_producer'][0],2);consumer=u(e.symbols['_fm_consumer'][0],2)
 rows.append(dict(frame=int(f.stem.split('-')[1]),target=target,live=live,
  debt_fs=None if not target else(target-live)*44100/3579545,
  debt_ms=None if not target else(target-live)*1000/3579545,z80pc=hex(u(hot+24,2)),
  steps=u(hot+2132,8),synth_writes=u(hot+2164+1044),fm_producer=producer,fm_consumer=consumer,
  fifo_pending=(producer-consumer)&65535))
edges=[];last=0;previous=0
for q in json.loads((run/'counter-trace.json').read_text()):
 a=q.get('audio_checkpoint')
 if not a:continue
 drop=a.get('dropped_fm',last)
 if drop!=last:edges.append(dict(after_previous_checkpoint_frame=previous,first_observed_frame=q['frame'],previous_count=last,new_count=drop,checkpoint=a))
 previous=q['frame'];last=drop
out=dict(snapshot_debt=rows,fm_drop_checkpoint_bounds=edges,exact_per_frame_FM_counter=False,
 audio_gate=r['audio_gate'],readback='MEMORY-AND-COMBAT-READBACK.json')
(run/'TIMELINE.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
