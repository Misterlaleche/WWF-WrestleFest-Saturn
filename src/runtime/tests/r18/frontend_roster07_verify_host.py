"""Prove ordinary roster feedback using real frontend owners and cursor flags."""
from pathlib import Path
import ctypes,hashlib,importlib.util,json,os,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];BASE=ROOT.parents[1];OUT=BASE/'validation/local-20261024/frontend-roster07-host';OUT.mkdir(exist_ok=True)
policy=BASE/'validation/local-20261024/hardware-input/pad_policy.py';spec=importlib.util.spec_from_file_location('pad',policy);py=importlib.util.module_from_spec(spec);spec.loader.exec_module(py)
C_POLICY='''static uint8_t roster_level(const uint8_t*r,unsigned clock){
 if(r[0x15e]&8u)return 255u;
 unsigned at=(r[0x5b0+0x33]&1u)?0x6bcu:0x5b0u,target=at==0x5b0u?0u:1u;
 if(!(r[at]&128u)||(r[at+0x33]&1u)||clock%8u>=4u)return 255u;
 unsigned cursor=r[at+0x23];if(cursor==target)return 239u;
 if(cursor>=5u)return 247u;
 return cursor<target?254u:253u;
}
'''
make=(ROOT/'Makefile').read_text();core=re.search(r'^CORE=(.*)$',make,re.M)[1].split();host=re.search(r'^R18_HOST=(.*)$',make,re.M)[1].split();probe='tests/r18/full_flow_combat_probe.c'
with tempfile.TemporaryDirectory(prefix='wf-roster07-',ignore_cleanup_errors=os.name=='nt')as td:
 frozen=Path(td)
 for name in('src','include','generated'):shutil.copytree(ROOT/name,frozen/name)
 for name in(probe,'tests/r18/frontend_driver_host.c'):
  p=frozen/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,p)
 p=frozen/'tests/r18/frontend_driver_host.c';s=p.read_text();at=s.index('static wf_status_t read_bus');s=s[:at]+C_POLICY+'''static uint8_t cached_roster_level(host_t*x){
 static unsigned last=0xffffffffu;static uint8_t level=255u;static uint32_t old=0xffffffffu;
 if(last!=x->frames){last=x->frames;level=roster_level(x->ram,x->frames);
  uint32_t state=((uint32_t)x->ram[0x5d3]<<24)|((uint32_t)x->ram[0x5e3]<<16)|((uint32_t)x->ram[0x6df]<<8)|x->ram[0x6ef];
  if(state!=old){printf("ROSTER clock=%u primary cursor=%u confirmed=%u partner cursor=%u confirmed=%u pad=%02x\\n",x->frames,x->ram[0x5d3],x->ram[0x5e3]&1u,x->ram[0x6df],x->ram[0x6ef]&1u,level);old=state;}
 }
 return level;
}
'''+s[at:]
 old='''    if((x->frames&3u)==1u)*v&=~0x10u;
    if(!x->royal&&(x->ram[0x5e3]&1u)&&!(x->ram[0x6ef]&1u)&&(x->frames&3u)==2u)*v&=~1u;''';assert s.count(old)==1
 new='''    if(!x->royal){uint8_t normal=cached_roster_level(x);uint8_t frontend=(uint8_t)((normal&0xf3u)|((normal&4u)<<1)|((normal&8u)>>1));*v&=0xff00u|frontend;}
    else if((x->frames&3u)==1u)*v&=~0x10u;'''
 s=s.replace(old,new);p.write_text(s)
 hashes={str(p.relative_to(frozen)):hashlib.sha256(p.read_bytes()).hexdigest()for p in frozen.rglob('*')if p.is_file()}
 binary=OUT/'probe.exe';command=['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',*core,*host,probe,'-o',str(binary)];subprocess.run(command,cwd=frozen,check=True)
 env={k:v for k,v in os.environ.items()if not k.startswith('WF_CAPTURED_HANDOFF')}
 with(OUT/'host.log').open('w')as log:r=subprocess.run([str(binary),'audit/runtime18-cd/ROM68K.BIN','audit/runtime18-cd/SPRITES.PAK','audit/runtime18-cd/R18RING.BIN','audit/runtime18-cd/HOTSPR.BIN','1','idle','0',str(OUT)],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
 text=(OUT/'host.log').read_text();assert r.returncode==0,text[-4000:];assert 'actors=0,7,6,3'in text
 states=re.findall(r'ROSTER .*',text);assert any('primary cursor=0 confirmed=1 partner cursor=0 confirmed=0'in x for x in states);assert any('partner cursor=1 confirmed=1'in x for x in states)
 (frozen/'policy.c').write_text('#include <stdint.h>\n'+C_POLICY+'\nunsigned oracle(const uint8_t*r,unsigned c){return roster_level(r,c);}\n');lib=frozen/'policy.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror',str(frozen/'policy.c'),'-o',str(lib)],check=True);native=ctypes.CDLL(str(lib));native.oracle.argtypes=[ctypes.c_void_p,ctypes.c_uint]
 count=0
 for primary in(0,1):
  for partner in(0,1):
   for cursor in range(10):
    for clock in range(16):
     ram=bytearray(0x4000);ram[0x5b0]=ram[0x6bc]=128;ram[0x5e3]=primary;ram[0x6ef]=partner;ram[(0x5b0 if not primary else 0x6bc)+0x23]=cursor
     before=bytes(ram);assert py.roster_tag07(ram,clock)==native.oracle(ctypes.create_string_buffer(before),clock);assert bytes(ram)==before;count+=1
 (OUT/'report.json').write_text(json.dumps({'status':'PASS','normal_pad_comparisons':count,'roster_states':states,'source_sha256':hashes,'python_sha256':hashlib.sha256(policy.read_bytes()).hexdigest(),'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'limits':['HOST complete frontend/paidF8C, not a native capture','Test bus caches one normal pad level per original VBlank, never per actor read','Both physical direction adapters preserve frontend UP/DOWN ordering; target0/7 uses RIGHT only in observed trajectory','No production owners or engine state changed; real64E0/navigation/confirmation/assignment execute']},indent=2)+'\n')
 print(text[-3000:]);print('PASS',count,'pure C/Python policy comparisons and original frontend cursor0->confirm->partner1->confirm->IDs0/7')
