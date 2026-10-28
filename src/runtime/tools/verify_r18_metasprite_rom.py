#!/usr/bin/env python3
"""Differential D1FC execution: native record parser versus original ROM.
No ROM code hooks or intercepted calls. Compares D6/D7, A2 record count,
and every WP26-consumed sprite field; unused word high bytes are normalized.
"""
import argparse,ctypes,hashlib,json,random,subprocess,tempfile,sys
from pathlib import Path
from wf_r18_rom_graphics import Rom,be16
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN,UC_HOOK_MEM_READ_UNMAPPED
from unicorn.m68k_const import *
p=argparse.ArgumentParser();p.add_argument('--rom',type=Path,required=True);p.add_argument('--cases',type=int,default=300);p.add_argument('--action-pose-links',action='store_true');p.add_argument('--paired-all',action='store_true');p.add_argument('--ring-route-poses',action='store_true');p.add_argument('--royal-exit-poses',action='store_true');p.add_argument('--paired-position',choices=('center','upper-left','lower-right','overflow'),default='center');p.add_argument('--report',type=Path);a=p.parse_args();rom=Rom(a.rom);root=Path(__file__).resolve().parents[1]
assert sum((a.action_pose_links,a.paired_all,a.ring_route_poses,a.royal_exit_poses))<=1,'Select only one exhaustive mode'
assert a.paired_all or a.paired_position=='center','Paired positions require --paired-all'
CB=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_size_t)
class Record(ctypes.Structure):_fields_=[('bytes',ctypes.c_uint8*16)]
class CacheEntry(ctypes.Structure):_fields_=[('address',ctypes.c_uint32),('end',ctypes.c_uint32),('header',ctypes.c_uint16),('skip_prefix',ctypes.c_uint8),('valid',ctypes.c_uint8),('rows',(ctypes.c_uint8*6)*32)]
class SetCacheEntry(ctypes.Structure):_fields_=[('base',ctypes.c_uint32),('rel',ctypes.c_uint16),('valid',ctypes.c_uint8),('reserved',ctypes.c_uint8)]
class PoseCacheEntry(ctypes.Structure):_fields_=[('pose',ctypes.c_uint16),('offset',ctypes.c_uint16),('set',ctypes.c_uint8),('valid',ctypes.c_uint8)]
class Cache(ctypes.Structure):_fields_=[('entries',CacheEntry*32),('buckets',ctypes.c_uint8*64),('links',ctypes.c_uint8*32),('next',ctypes.c_uint8),('wrestler_sets',SetCacheEntry*12),('pose_offsets',PoseCacheEntry*32),('word_tag',ctypes.c_uint32*64),('word_data',ctypes.c_uint16*64)]
class Ctx(ctypes.Structure):_fields_=[('read',CB),('user',ctypes.c_void_p),('palette',ctypes.POINTER(ctypes.c_uint8)),('records',ctypes.POINTER(Record)),('capacity',ctypes.c_size_t),('count',ctypes.c_size_t),('d6',ctypes.c_uint32),('d7',ctypes.c_uint32),('cache',ctypes.POINTER(Cache))]
source=ctypes.create_string_buffer(rom.p)
read_count=0
profile_warm=False
warm_read_sites={}
@CB
def read(user,at,dst,n):
 global read_count
 read_count+=1
 if profile_warm:warm_read_sites[(at,n)]=warm_read_sites.get((at,n),0)+1
 if at+n>len(rom.p):return 1
 ctypes.memmove(dst,ctypes.addressof(source)+at,n);return 0
rng=random.Random(0xd1fc18);cases=[]
for s,_,_,n in rom.active_sets():
 for pose in range(n):
  ent=rom.pose_entry(s,pose)
  if ent not in (None,65534,65535):cases.append((s,pose,be16(rom.p,ent)))
# Ensure each record class and flags is present, then sample across all sets.
selected=[]
for kind in range(4):
 for flag in (0,256,512,768):
  candidates=[c for c in cases if c[2]>>14==kind and c[2]&768==flag]
  if candidates:selected.extend(rng.sample(candidates,min(5,len(candidates))))
if a.action_pose_links:
 tables=json.loads((root.parent.parent/'validation/local-20261026/action-animation-table-audit.json').read_text())
 inventory=json.loads((root.parent.parent/'validation/local-20261026/action-selector-inventory.json').read_text())
 linked={(character,int(pose,16)&0x7fff)
         for selector,item in enumerate(tables['action_selectors'])
         for character in inventory['selectors'][selector]['characters_in_generated_choice_matrix']
         for pose in item['poses_raw']
         if (character,int(pose,16)&0x7fff)!=(5,0x20f)}
 pose_audit=json.loads((root.parent.parent/'validation/local-20261026/action-pose-link-audit.json').read_text())
 assert pose_audit['status']=='PASS' and pose_audit['rom_sha1']==hashlib.sha1(rom.p).hexdigest()
 assert pose_audit['generated_table_sha256']==hashlib.sha256((root/'generated/wf_animation_tables.c').read_bytes()).hexdigest()
 assert pose_audit['runtime_descriptor_source_sha256']==hashlib.sha256((root/'src/wf_r17_actions_r11.c').read_bytes()).hexdigest()
 linked.update((item['character'],int(item['pose'],16))
               for item in pose_audit['additional_pose_pairs'])
 linked.update((item['character'],int(item['pose'],16))
               for item in pose_audit['action79_new_pose_pairs'])
 linked.update((item['character'],int(item['pose'],16))
               for item in pose_audit['action1e_1f_new_pose_pairs'])
 linked.update((item['character'],int(item['pose'],16))
               for item in pose_audit['action1e_1f_capture_new_pose_pairs'])
 linked.update((item['character'],int(item['pose'],16))
               for item in pose_audit['action2f_capture_new_pose_pairs'])
 linked.update((item['character'],int(item['pose'],16))
               for item in pose_audit['action2d_capture_new_pose_pairs'])
 assert len(linked)==1178,len(linked)
 selected=[case for case in cases if (case[0],case[1]) in linked for _ in (0,1)]
 assert len(selected)==2*len(linked),(len(selected),len(linked))
elif a.paired_all:
 paired=[case for case in cases if case[0]<12 and case[1]<565 and case[2]>>14==2]
 assert len(paired)==904,len(paired)
 selected=[(s,pose,h,partner,mirror) for s,pose,h in paired
           for partner in range(12) for mirror in range(2)]
elif a.ring_route_poses:
 ring_descriptors=(0x115f2,0x1160a,0x11622,0x1163a,0x1981e,0x199fc)
 ring_poses=set()
 for address in ring_descriptors:
  kind,n=be16(rom.p,address+4),be16(rom.p,address+6)
  assert kind in (1,2) and 0<n<=8,(hex(address),kind,n)
  ring_poses.update(be16(rom.p,address+8+2*n+2*i)&0x7fff for i in range(n))
 assert len(ring_poses)==26,len(ring_poses)
 selected=[case for case in cases if case[0]<12 and case[1] in ring_poses for _ in (0,1)]
 assert len(selected)==624,len(selected)
elif a.royal_exit_poses:
 royal_descriptors=(0x115f2,0x1a348,0x1a354)
 royal_poses={0,0x68,0x1d3} # neutral, captured entry, and explicit $1A38E pose
 for address in royal_descriptors:
  kind,n=be16(rom.p,address+4),be16(rom.p,address+6)
  assert kind in (1,2) and 0<n<=8,(hex(address),kind,n)
  royal_poses.update(be16(rom.p,address+8+2*n+2*i)&0x7fff for i in range(n))
 assert len(royal_poses)==10,len(royal_poses)
 selected=[case for case in cases if case[0]<12 and case[1] in royal_poses for _ in (0,1)]
 assert len(selected)==240,len(selected)
else:
 selected+=rng.sample(cases,min(a.cases,len(cases)))
if not a.paired_all:selected=[(*case,None,None) for case in selected]
counts={};blocked=[];cold_reads=warm_reads=0
with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as tmp:
 lib=Path(tmp)/'meta.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-I'+str(root/'include'),str(root/'src/wf_r18_metasprite.c'),'-o',str(lib)],check=True);native=ctypes.CDLL(str(lib));native.wf_r18_metasprite_emit.argtypes=[ctypes.POINTER(Ctx)]+[ctypes.c_uint16]*5
 native.wf_r18_metasprite_cache_reset.argtypes=[ctypes.POINTER(Cache)];native.wf_r18_metasprite_cache_reset.restype=None
 assert ctypes.sizeof(Cache)<=8*1024
 for i,(s,pose,h,assigned_partner,assigned_mirror) in enumerate(selected):
  partner=rng.randrange(12) if assigned_partner is None else assigned_partner
  mirror=i%2 if assigned_mirror is None else assigned_mirror
  x,y=({'center':(160,120),'upper-left':(0,0),'lower-right':(0x15f,0x187),'overflow':(65535,65535)}[a.paired_position]
       if a.paired_all else rng.choice([(150,100),(0,0),(1000,100),(100,1000),(65535,65535),(rng.randrange(65536),rng.randrange(65536))]))
  seed=rng.getrandbits(32)
  palette=bytes(rng.randrange(16) for _ in range(256));pal=(ctypes.c_uint8*256).from_buffer_copy(palette);records=(Record*1024)();ctx=Ctx(read,None,pal,records,1024,0,seed,(seed+1)&0xffffffff,None)
  result=native.wf_r18_metasprite_emit(ctypes.byref(ctx),s,pose|(mirror<<15),x,y,partner)
  cache=Cache();native.wf_r18_metasprite_cache_reset(ctypes.byref(cache))
  for repeat in range(2):
   cached_records=(Record*1024)();cached=Ctx(read,None,pal,cached_records,1024,0,seed,(seed+1)&0xffffffff,ctypes.pointer(cache));read_count=0
   profile_warm=repeat==1
   cached_result=native.wf_r18_metasprite_emit(ctypes.byref(cached),s,pose|(mirror<<15),x,y,partner)
   profile_warm=False
   if repeat==0:cold_reads+=read_count
   else:warm_reads+=read_count
   assert (cached_result,cached.d6,cached.d7,cached.count)==(result,ctx.d6,ctx.d7,ctx.count),('cache context',i,repeat,s,pose,hex(h))
   assert bytes(cached_records)[:cached.count*16]==bytes(records)[:ctx.count*16],('cache records',i,repeat,s,pose,hex(h))
  if result:
   blocked.append((s,pose,hex(h)));assert ctx.d6==seed and ctx.d7==(seed+1)&0xffffffff and ctx.count==0;continue
  u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000);u.mem_map(0,0x200000);u.mem_write(0,rom.p)
  def mirror_read(cpu,access,address,size,value,user):
   physical=address&0xffffff;page=address&~0xfff;lowpage=physical&~0xfff
   if lowpage>=len(rom.p):return False
   cpu.mem_map(page,0x1000);cpu.mem_write(page,rom.p[lowpage:lowpage+0x1000]);return True
  u.hook_add(UC_HOOK_MEM_READ_UNMAPPED,mirror_read)
  def w(at,v,n=2):u.mem_write(at,(v&((1<<(8*n))-1)).to_bytes(n,'big'))
  actor=0x1c05b0;other=0x1c06b0;w(actor+2,s);w(actor+4,pose|(mirror<<15));w(actor+0x14,x);w(actor+0x16,y);w(actor+0x26,other,4);w(other+2,partner);u.mem_write(0x1c1610,palette)
  for j in range(8):u.reg_write(UC_M68K_REG_D0+j,(seed+j*0x11111111)&0xffffffff)
  u.reg_write(UC_M68K_REG_D6,seed);u.reg_write(UC_M68K_REG_D7,(seed+1)&0xffffffff);u.reg_write(UC_M68K_REG_A0,actor);u.reg_write(UC_M68K_REG_A2,0xc2000);u.reg_write(UC_M68K_REG_A7,0x1d0000);w(0x1d0000,0x70000,4)
  try:u.emu_start(0xd1fc,0x70000,count=500000)
  except Exception as e:raise RuntimeError((i,s,pose,hex(h),partner,mirror,x,y,hex(u.reg_read(UC_M68K_REG_PC)))) from e
  assert u.reg_read(UC_M68K_REG_PC)==0x70000,('ROM instructionlimit',s,pose,hex(h))
  wanted=(u.reg_read(UC_M68K_REG_D6),u.reg_read(UC_M68K_REG_D7),(u.reg_read(UC_M68K_REG_A2)-0xc2000)//16)
  got=(ctx.d6,ctx.d7,ctx.count)
  assert got==wanted,(i,s,pose,hex(h),partner,mirror,x,y,'register/count',got,wanted)
  raw=bytes(u.mem_read(0xc2000,ctx.count*16))
  for j in range(ctx.count):
   native_bytes=bytes(records[j].bytes);expected=raw[j*16:j*16+16]
   for off in (0,1,3,5,7,9,10,11):assert native_bytes[off]==expected[off],(i,s,pose,hex(h),partner,mirror,x,y,'record',j,off,native_bytes.hex(),expected.hex())
  key=(h>>14,h&768);counts[key]=counts.get(key,0)+1
 if sys.platform=='win32':
  assert ctypes.windll.kernel32.FreeLibrary(ctypes.c_void_p(native._handle))
print('PASS',sum(counts.values()),'direct-ROM cases; classes',counts,'blocked',len(blocked),blocked[:20])
assert warm_reads<cold_reads,('decoded-body cache did not reduce reads',cold_reads,warm_reads)
print('PASS cold/warm cache equivalence for',len(selected),'cases;',ctypes.sizeof(Cache),'bytes; reader calls',cold_reads,'cold ->',warm_reads,'warm')
warm_top=sorted(warm_read_sites.items(),key=lambda item:(-item[1],item[0]))[:16]
if a.action_pose_links:print('Warm ROM read hotspots',[(hex(at),n,count) for (at,n),count in warm_top[:8]])
if a.action_pose_links:
 assert not blocked,blocked
 print('PASS all',len(linked),'selected action character/pose pairs in both mirror directions')
if a.paired_all:
 assert not blocked,blocked
 print('PASS all',len(paired),'paired poses with all twelve partner sets and both mirror directions')
if a.ring_route_poses:
 assert not blocked,blocked
 print('PASS all',len(ring_poses),'ring-route poses on all twelve wrestler sets and both mirror directions')
if a.royal_exit_poses:
 assert not blocked,blocked
 print('PASS all',len(royal_poses),'Royal-exit poses on all twelve wrestler sets and both mirror directions')
if a.report:
 report={'status':'PASS','mode':'action-pose-links' if a.action_pose_links else 'paired-all' if a.paired_all else 'ring-route-poses' if a.ring_route_poses else 'royal-exit-poses' if a.royal_exit_poses else 'sampled',
         'rom_sha1':hashlib.sha1(rom.p).hexdigest(),'direct_rom_cases':sum(counts.values()),
         'verifier_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
         'metasprite_source_sha256':hashlib.sha256((root/'src/wf_r18_metasprite.c').read_bytes()).hexdigest(),
         'pose_link_audit_sha256':hashlib.sha256((root.parent.parent/'validation/local-20261026/action-pose-link-audit.json').read_bytes()).hexdigest() if a.action_pose_links else None,
         'sprite_classes':{f'{kind}:{flag}':n for (kind,flag),n in counts.items()},
         'blocked_cases':blocked,'cache_bytes':ctypes.sizeof(Cache),
         'cold_rom_reads':cold_reads,'warm_rom_reads':warm_reads,
         'warm_rom_read_top':[{'address':f'{at:06X}','bytes':n,'calls':count}
                              for (at,n),count in warm_top],
         'action_character_pose_pairs':len(linked) if a.action_pose_links else None,
         'paired_poses':len(paired) if a.paired_all else None,
         'paired_position':a.paired_position if a.paired_all else None,
         'ring_route_descriptors':[f'{address:06X}' for address in ring_descriptors] if a.ring_route_poses else None,
         'ring_route_poses':[f'{pose:04X}' for pose in sorted(ring_poses)] if a.ring_route_poses else None,
         'royal_exit_descriptors':[f'{address:06X}' for address in royal_descriptors] if a.royal_exit_poses else None,
         'royal_exit_poses':[f'{pose:04X}' for pose in sorted(royal_poses)] if a.royal_exit_poses else None,
         'limits':['Action mode covers generated matrix links, statically possible alternates, ROM-indexed $79 and both $1E/$1F object-release descriptors in both mirror directions.',
                   'Each paired mode run covers all 904 paired pose entries, twelve partner sets and both mirror directions at its reported screen position.',
                   'Ring-route mode covers the six ROM descriptors used by $68/$69/$6B on all twelve wrestler sets in both mirror directions; a seeded partner and screen position vary by case.',
                   'Royal-exit mode covers three $7A ROM descriptors, explicit visual pose $01D3, neutral pose and captured entry pose on all twelve wrestler sets in both mirror directions.',
                   'Action mode uses one seeded partner and one of several screen positions per case.',
                   'Callback-selected alternate poses, game scheduling, and Saturn VDP1 output require separate evidence.']}
 a.report.parent.mkdir(parents=True,exist_ok=True)
 a.report.write_text(json.dumps(report,indent=2)+'\n')
