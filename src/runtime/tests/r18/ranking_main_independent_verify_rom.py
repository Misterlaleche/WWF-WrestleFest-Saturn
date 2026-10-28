"""Independent frozen ranking65C0 review: new seed and adversarial fixtures."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess,sys,zipfile
from capstone import Cs,CS_ARCH_M68K,CS_MODE_BIG_ENDIAN,CS_MODE_M68K_000
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args()
out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);frozen=out/'frozen/src/runtime';assert not frozen.exists();frozen.mkdir(parents=True)
shutil.copytree(ROOT/'include',frozen/'include')
for name in('src/wf_r18_ranking.c','src/wf_r18_ranking_glyph.c','src/wf_r18_records_aux.c','src/wf_r17_m68k_shadow.c','tests/r18/ranking_matrix_verify_rom.py','tests/r18/ranking_capture_manifest.json','tests/r18/ranking_snapshot_stage9b_guard.bin','audit/runtime18-cd/ROM68K.BIN'):
 dest=frozen/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
owner=frozen/'src/wf_r18_ranking.c';owner_hash=hashlib.sha256(owner.read_bytes()).hexdigest()
assert owner_hash=='75d0e37bf15d937eedb429c5066f1281ea030186415f821f35fe3bd7af8e3b69'
body=(frozen/'tests/r18/ranking_matrix_verify_rom.py').read_text()
start=body.index(' def fixtures():');end=body.index(' for idx,(label,',start)
extras=''' def fixtures():
  # Full physical WORD reads, asymmetrical high BYTE and all four ports.
  for high in(0,0x7f,0x80,0xff):
   for low in range(256):
    slot=(low//64+high)%4;word=(high<<8)|low;mem=seed();regs=[rng.getrandbits(32)for _ in range(15)];regs[1]=(regs[1]&0xffff0000)|slot
    yield f'extra-physical-word-{high}-{low}-{slot}',0x6ce8,mem,regs,(word,),False
  # Every NEW-marker pattern with eleven equal scores. Sorting is stable
  # within NEW/old groups and must move all six raw bytes, not just scores.
  for flags in range(2048):
   mode=flags%4;mem=seed();mem[0x1c0161]=mode;base=0x104 if mode&1 else 0xc2
   for row in range(11):
    mem[0x1c0000+base+row*6:0x1c0000+base+row*6+6]=bytes((77,0x40+row,0x5c if flags>>row&1 else 0x5b,row%4,0x80+row,0xf0-row))
   yield f'extra-stable-ties-{flags}',0x683e,mem,None,(0xffff,),False
  # WORD candidate clamp and BYTE TAG score are deliberately distinct.
  for mode,candidate,player,last in itertools.product((0,1,0xfe,0xff),(0,99,100,101,0x8000,0xff63,0xffff),range(4),(0,1,99,255)):
   mem=seed();mem[0x1c0161]=mode;put(mem,0x1c0164,candidate);put(mem,0x1c05b0+player*0x10c+0xc4,candidate)
   mem[0x1c0140 if mode&1 else 0x1c00fe]=last;regs=[rng.getrandbits(32)for _ in range(15)];regs[0]=(regs[0]&0xffff0000)|player;regs[8]=0x1c05b0+player*0x10c
   yield f'extra-qualify-width-{mode}-{candidate}-{player}-{last}',0x67b2,mem,regs,(0xffff,),True
  # Blink consumes the full BYTE for its increment but only low nibble
  # for its save/clear/restore decision, including FF->00 wrap.
  for flags in range(256):
   slot=flags%4;rank=(flags//4)%11;mem=seed();put(mem,0x1c00ba+slot*2,rank);mem[0x1c1cd4+slot]=flags
   regs=[rng.getrandbits(32)for _ in range(15)];regs[1]=(regs[1]&0xffff0000)|slot
   yield f'extra-blink-byte-{flags}',0x68f6,mem,regs,(0xffff,),False
  for slot,rank,char,repeat in itertools.product(range(4),(0,2,3,10),(0x31,0x4a,0x4b,0x014b,0xffff),(7,0xffff)):
   mem=seed();mem[0x1c0161]=(char+slot)&0xff;put(mem,0x1c00ba+slot*2,rank);put(mem,0x1c00a2+slot*2,char);put(mem,0x1c00aa+slot*2,repeat);put(mem,0x1c00b2+slot*2,slot%3);put(mem,0x1c0146+slot*2,slot)
   regs=[rng.getrandbits(32)for _ in range(15)];regs[0]=(regs[0]&0xffff0000)|rank;regs[1]=(regs[1]&0xffff0000)|slot
   ports=((0xfffe,0xffef),(0xfffd,0xffdf),(0xfff0,0xffcf),(0xffff,0xffcc))[slot]
   yield f'extra-initials-wrap-{slot}-{rank}-{char}-{repeat}',0x6b6e,mem,regs,ports,False
  # All four new participants complete all three initials without an
  # invented rank reset, frame clock or input source substitution.
  for mode in(0,1,2,0xff):
   mem=seed();mem[0x1c0161]=mode;put(mem,0x1c0164,77)
   for slot in range(4):put(mem,0x1c0092+slot*4,0x1c05b0+slot*0x10c,4);put(mem,0x1c05b0+slot*0x10c+0xc4,77)
   tape=((0xffff,0xffef)*4+(0xffff,0xffff)*4)*3
   yield f'extra-whole-four-{mode}',0x65c0,mem,None,tape,False
'''
# Leave each pending request inert for extra polls. ACK stays absent for
# three explicit completions, then the same request receives actual tapeACK.
extra=body[:start]+extras+body[end:]
needle="   before=(bytes(s),ram.raw,list(dr.d),list(m.a));assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==1;assert before==(bytes(s),ram.raw,list(dr.d),list(m.a))"
assert needle in extra
extra=extra.replace(needle,needle+'''
   for delayed in range(3):
    assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==1
    assert before==(bytes(s),ram.raw,list(dr.d),list(m.a))
   if r.kind==3:
    for delayed in range(3):
     unchanged=(ram.raw,list(dr.d),list(m.a),s.depth,s.saved_a2,s.pc)
     assert n.wf_r18_ranking_complete(ctypes.byref(s),ctypes.byref(ctx),0,0)==3
     assert unchanged==(ram.raw,list(dr.d),list(m.a),s.depth,s.saved_a2,s.pc)
     assert n.wf_r18_ranking_step(ctypes.byref(s),ctypes.byref(ctx))==1
     assert unchanged==(ram.raw,list(dr.d),list(m.a),s.depth,s.saved_a2,s.pc)
''',1)
(frozen/'tests/r18/ranking_independent_extra_derived.py').write_text(extra)
hashes={str(x.relative_to(frozen)):hashlib.sha256(x.read_bytes()).hexdigest()for x in frozen.rglob('*')if x.is_file()}
with zipfile.ZipFile(out/'frozen-sources.zip','w',zipfile.ZIP_DEFLATED)as z:
 for name in hashes:z.write(frozen/name,name)
rom=(frozen/'audit/runtime18-cd/ROM68K.BIN').read_bytes();decoder=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
expected=set()
for start,end in((0x65c0,0x69c2),(0x69c8,0x6b42),(0x6b6e,0x6cfa),(0x7720,0x7744)):
 expected.update(i.address for i in decoder.disasm(rom[start:end],start))
reports={}
env=os.environ.copy()
env['PYTHONPATH']=os.pathsep.join(str(Path(x).resolve())for x in env.get('PYTHONPATH','').split(os.pathsep)if x)
for name,script,seed in(('author-new-seed','ranking_matrix_verify_rom.py','0x9b65c0'),('independent-extra','ranking_independent_extra_derived.py','0x68f665c0')):
 with(out/(name+'.log')).open('w')as log:r=subprocess.run([sys.executable,str(frozen/'tests/r18'/script),'--seed',seed,'--report',str(out/(name+'.json'))],cwd=frozen,env=env,stdout=log,stderr=subprocess.STDOUT)
 assert r.returncode==0,(name,r.returncode,(out/(name+'.log')).read_text()[-3000:])
 q=json.loads((out/(name+'.json')).read_text());assert q['status']=='PASS'
 assert q['source_sha256']==owner_hash
 reports[name]={'cases':q['cases'],'failure_prefixes':q['failure_prefixes'],'boundaries':q['requests'],'ACKs':q['ack_events']};print('PASS',name,reports[name],flush=True)
coverage=set()
bus_pcs=set()
for name in reports:
 q=json.loads((out/(name+'.json')).read_text());coverage.update(int(x,16)for x in q['coverage'])
 # The base runner executes each requested bus instruction directly with
 # execute(1), outside run_to's trace loop. Its recorded READ/WRITE PCs are
 # also executed original instructions, with state compared afterwards.
 bus_pcs.update(pc for pc,width in q['read_widths']+q['write_widths'])
coverage.update(bus_pcs)
missing=sorted(expected-coverage)
assert not missing,('uncovered main instructions',list(map(hex,missing)))
result={'status':'PASS','owner_sha256':owner_hash,'reports':reports,'main_code_PC_count':len(expected),'covered_main_code_PC_count':len(expected&coverage),'bus_instruction_PCs':list(map(hex,sorted(bus_pcs))),'missing_main_PCs':list(map(hex,missing)),'sources_sha256':hashes,'archive_sha256':hashlib.sha256((out/'frozen-sources.zip').read_bytes()).hexdigest(),'limits':['Raw16KB/D0-D7/A0-A6/FG16KB+board regions compared at every effect boundary and return; stack/CCR/timing excluded','Glyph and records auxiliaries execute actual native C. Other closed scene dependencies execute intact ROM and supply opaque responses; this does not review native implementations or hardware IRQ execution','ACK tape is explicit; delayed ACK controls do not synthesize an IRQ implementation','No global combat probe or hardware/global-equivalence claim']}
(out/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k!='sources_sha256'},indent=2))
