#!/usr/bin/env python3
from rom_oracle import *
import json,collections
rom=load_rom('/workspace/scratch/8e35fd0cc989/project_sources/01-wwfwfest.zip')
rows={}
for ident in range(12):
 ids=[i for i in range(12) if i!=ident][:3];ids.insert(2,ident)
 o=Oracle(rom,ids,0x12345678);seen=collections.defaultdict(set)
 def f(u,addr,size,data):
  pose=o.r16(ACTORS[2]+4)
  seen[pose].add(tuple(u.reg_read(r) for r in DREGS[4:]))
 o.u.hook_add(UC_HOOK_CODE,f,begin=0xa732,end=0xa732)
 o.run();rows[ident]={pose:[list(t) for t in values] for pose,values in seen.items()}
 print(ident,dict(seen))
(HERE/'composer_rng_context.json').write_text(json.dumps(rows,indent=2)+'\n')
