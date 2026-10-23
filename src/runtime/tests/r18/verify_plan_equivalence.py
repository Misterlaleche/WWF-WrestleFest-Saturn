#!/usr/bin/env python3
"""Production planner vs pinned5ab045c, including collisions, stale frames,
all16-bit sourceIDs, culling edges, duplicates, command/tile capacity limits."""
import ctypes,random,subprocess,tempfile,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[2];rng=random.Random(0x5ab045c)
deps=['src/wf_r18_metasprite.c','src/wf_actor.c','src/wf_saturn_yaul_backend.c','src/wf_saturn_memory.c','src/wf_saturn_render_bridge.c','src/wf_vdp1_plan.c']
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);old=root/'tests/r18/fixtures/wf_r18_present_5ab.c'
 assert hashlib.sha256(old.read_bytes()).hexdigest()=='c9e40e9cd73247846b7b5db0be692903dbd43acb5bb1c2c9604b2dc1f2df3267'
 libs=[]
 for name,source in [('old',old),('current',root/'src/wf_r18_present.c')]:
  lib=t/(name+'.so');subprocess.run(['cc','-shared','-fPIC','-O2','-std=c11','-Wall','-Wextra','-Werror','-Iinclude','-DWF_PRESENT_SOURCE="'+str(source)+'"','tests/r18/plan_equivalence_bridge.c',*deps,'-o',str(lib)],cwd=root,check=True)
  native=ctypes.CDLL(str(lib));native.plan_case.argtypes=[ctypes.POINTER(ctypes.c_int32),ctypes.c_uint,ctypes.POINTER(ctypes.c_uint16),ctypes.POINTER(ctypes.c_uint8),ctypes.POINTER(ctypes.c_uint)];libs.append(native)
 cases=[]
 # Every16-bit ID must survive hashing, including same-low9bit collisions.
 for base in range(0,65536,128):cases.append([(base+i,100,100,i&1,(i>>1)&1,i&15) for i in range(128)])
 for i in range(256):
  ids=[(i+j*512)&65535 for j in range(256)];cases.append([(v,100,100,0,0,0) for v in ids])
 cases += [[(i,100,100,0,0,0) for i in range(257)],[(42,100,100,0,0,0)]*513,[],[(1,-16,0,0,0,0),(2,-15,-15,1,1,15),(3,319,239,0,1,9),(4,320,0,0,0,0),(5,0,240,0,0,0)]]
 for i in range(768):
  palette=[rng.randrange(65536) for _ in range(rng.randrange(1,257))]
  cases.append([(rng.choice(palette),rng.randrange(-32,352),rng.randrange(-32,272),rng.randrange(2),rng.randrange(2),rng.randrange(16)) for _ in range(rng.randrange(600))])
 for i,pieces in enumerate(cases):
  source=(ctypes.c_int32*(len(pieces)*6))(*(v for p in pieces for v in p));results=[]
  for lib in libs:
   tiles=(ctypes.c_uint16*512)();cmds=(ctypes.c_uint8*(512*11))();counts=(ctypes.c_uint*3)();assert lib.plan_case(source,len(pieces),tiles,cmds,counts)==0
   results.append((list(counts),bytes(tiles)[:counts[1]*4],bytes(cmds)[:counts[2]*11]))
  assert results[0]==results[1],('planner mismatch',i,results[0][0],results[1][0])
 print('PASS',len(cases),'planner equivalence cases vs5ab045c; all65536IDs, collisions, consecutive frames, exact slots/order/fields and capacity failures')
