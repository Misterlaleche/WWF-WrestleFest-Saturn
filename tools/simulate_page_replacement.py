#!/usr/bin/env python3
"""Causal, empty-cache 4 KiB replacement-policy audit of native visible tiles.

No prefetch, future references, predefined pose lists or game mutations.
All visible IDs are demands, including IDs the production VDP1 texture path
can serve without reading decoded RAM. These counts are therefore a paired
policy model, not exact loader reads and not Saturn execution-time estimates.
Adjacent references to the same page are elided, which preserves these policies.
LFU/GCLOCK count at most one use per frame; SLRU promotes only on a later frame.
The coarse parameter set is shared by both modes and both capacities.
"""
from collections import OrderedDict,Counter
from pathlib import Path
import sys,json,hashlib
sys.path.insert(0,str(Path.cwd()/'tools'))
from simulate_neighbor_prefetch import parse,prepare

class LRU:
 def __init__(self,c,**kw): self.c=c;self.q=OrderedDict();self.ops=Counter()
 def access(self,p,f):
  if p in self.q:self.q.move_to_end(p);return False
  if len(self.q)==self.c:self.q.popitem(last=False)
  self.q[p]=f;return True
class SLRU:
 def __init__(self,c,ratio,**kw):self.c=c;self.protected=max(1,int(c*ratio));self.a=OrderedDict();self.b=OrderedDict();self.ops=Counter()
 def access(self,p,f):
  if p in self.b:self.b.move_to_end(p);self.b[p]=f;return False
  if p in self.a:
   old=self.a[p]
   if old!=f:
    del self.a[p];self.b[p]=f;self.ops['promotions']+=1
    if len(self.b)>self.protected:
     q,last=self.b.popitem(last=False);self.a[q]=last;self.ops['demotions']+=1
   else:
    self.a.move_to_end(p);self.a[p]=f
   return False
  if len(self.a)+len(self.b)==self.c:
   assert self.a
   self.a.popitem(last=False)
  self.a[p]=f;return True
class LFU:
 def __init__(self,c,decay,**kw):self.c=c;self.decay=decay;self.epoch=-1;self.q={};self.seq=0;self.ops=Counter()
 def access(self,p,f):
  e=f//self.decay
  if e!=self.epoch:
   if self.epoch>=0:
    shift=min(8,e-self.epoch)
    for row in self.q.values():row[0]>>=shift
    self.ops['decay_updates']+=len(self.q)
   self.epoch=e
  self.seq+=1
  row=self.q.get(p)
  if row is not None:
   if row[1]!=f:row[0]=min(255,row[0]+1);row[1]=f
   row[2]=self.seq;return False
  if len(self.q)==self.c:
   self.ops['victim_scan_entries']+=len(self.q)
   q=min(self.q,key=lambda q:(self.q[q][0],self.q[q][2]));del self.q[q]
  self.q[p]=[1,f,self.seq];return True
class Clock:
 def __init__(self,c,levels=1,frame_aware=False,**kw):self.c=c;self.levels=levels;self.frame_aware=frame_aware;self.data=[None]*c;self.ref=[0]*c;self.frame=[-1]*c;self.slot={};self.hand=0;self.ops=Counter()
 def access(self,p,f):
  s=self.slot.get(p)
  if s is not None:
   if not self.frame_aware or self.frame[s]!=f:self.ref[s]=min(self.levels,self.ref[s]+1);self.frame[s]=f
   return False
  while self.ref[self.hand]:
   self.ops['victim_scan_entries']+=1;self.ref[self.hand]-=1;self.hand=(self.hand+1)%self.c
  s=self.hand;self.ops['victim_scan_entries']+=1
  if self.data[s] is not None:del self.slot[self.data[s]]
  self.data[s]=p;self.ref[s]=1;self.frame[s]=f;self.slot[p]=s;self.hand=(s+1)%self.c;return True

def run(frames,typ,cap,**kw):
 c=typ(cap,**kw);h=[]
 for f,v,_ in frames:h.append(sum(c.access(p,f) for p in v))
 return {'misses':sum(h),'frames_with_misses':sum(x>0 for x in h),'max_frame':max(h),'ops':dict(c.ops)}

spec=[('LRU',LRU,{})]+[(f'SLRU-{r}',SLRU,{'ratio':r}) for r in (.5,.67,.75,.875)]+[(f'LFU-{d}',LFU,{'decay':d}) for d in (64,256,1024)]+[('CLOCK',Clock,{}),('GCLOCK2',Clock,{'levels':2,'frame_aware':True}),('GCLOCK3',Clock,{'levels':3,'frame_aware':True})]

def independent_lru(frames,capacity):
 # Separate implementation of the loader's increasing-age/minimum-age rule.
 tags=[None]*capacity;ages=[0]*capacity;lookup={};clock=0;misses=0
 for frame,visits,_ in frames:
  for page in visits:
   slot=lookup.get(page)
   if slot is None:
    misses+=1
    slot=next((i for i,p in enumerate(tags) if p is None),None)
    if slot is None:slot=min(range(capacity),key=ages.__getitem__)
    if tags[slot] is not None:del lookup[tags[slot]]
    tags[slot]=page;lookup[page]=slot
   clock+=1;ages[slot]=clock
 return misses

def self_tests():
 c=SLRU(4,ratio=.5)
 assert c.access(1,0) and not c.access(1,0) and 1 in c.a and 1 not in c.b
 assert not c.access(1,1) and 1 in c.b and 1 not in c.a
 c=LFU(4,decay=64)
 c.access(1,0)
 for _ in range(32):c.access(1,0)
 assert c.q[1][0]==1
 c.access(1,1);assert c.q[1][0]==2
 c.access(1,64);assert c.q[1][0]==2
 samples=[(f,tuple((f*7+i*i)%17 for i in range(10)),()) for f in range(40)]
 # Adding a future suffix cannot change any prior hit/miss decision.
 for name,kind,args in spec:
  def decisions(frames):
   state=kind(8,**args)
   return [tuple(state.access(page,f) for page in visits) for f,visits,_ in frames]
  assert decisions(samples[:17])==decisions(samples)[:17],name
 assert run(samples,LRU,8)['misses']==independent_lru(samples,8)
 return ['SLRU same-frame protection','LFU one increment per frame and decay',
         'future suffix cannot change prefix decisions','independent age-scan LRU']

if __name__=='__main__':
 root=Path(__file__).resolve().parents[1]
 directory=root/'evidence/gpu-cache-simulation'
 baseline=json.loads((directory/'neighbor-prefetch.json').read_text())
 res={};checks=self_tests()
 for mode in ('tag','royal'):
  data=(directory/(mode+'-tiles.bin')).read_bytes();raw=parse(data);fs=prepare(raw)
  res[mode]={'trace_sha256':hashlib.sha256(data).hexdigest(),'frames':len(raw),
             'page_visits':sum(len(v) for _,v,_ in fs),'capacities':{}}
  for cap in (138,142):
   rows={}
   for name,kind,args in spec:
    rows[name]=run(fs,kind,cap,**args)
    print(mode,cap,name,rows[name],flush=True)
   assert rows['LRU']['misses']==independent_lru(fs,cap)
   if cap==142:assert rows['LRU']['misses']==baseline['modes'][mode]['capacities']['142']['baseline']['demand_misses']
   res[mode]['capacities'][str(cap)]=rows
 report={'scope':__doc__,'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
   'self_tests':checks+['actual traces match independent age scan and prior142-page baseline'],
   'policy_metadata_upper_bytes_at142':{'LRU':572,'SLRU':878,'LFU':1290,'CLOCK':20,'GCLOCK':712},
   'metadata_scope':'Conceptual packed C policy arrays only; same existing page-to-slot lookup and source tags excluded. No implementation or allocated production memory.',
   'access_costs':{'LRU':'one age write on hit; existing loader scans up to capacity on miss',
                 'SLRU':'constant-time linked queues; frame check plus queue relinks on hit/promotion',
                 'LFU':'one per-frame frequency increment, recency write; capacity scan on miss and decay sweep',
                 'CLOCK':'reference bit on hit; hand scan on miss',
                 'GCLOCK':'frame check and capped counter on hit; hand decrements on miss'},
   'native142_loader_reference':{'tag':807,'royal':2205},
   'native_reference_source':'evidence/native-4k/native-replay-summary.json',
   'limits':['Input records visible tiles, not exact production decoded-RAM access order.',
             'No warm frontend cache, scene reads, hardware rendering, decompression time or FPS modeled.',
             'CLOCK and GCLOCK tested; adaptive CLOCK-Pro with ghost history was not implemented.',
             'No policy showed a strong Royal gain; no production change recommended.'],
   'modes':res}
 target=directory/'page-replacement.json';target.write_text(json.dumps(report,indent=2)+'\n')
 print(target)
