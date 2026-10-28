"""Compare hardware Python policy with the actual extracted C human_pad."""
from pathlib import Path
import argparse,ctypes,hashlib,importlib.util,itertools,json,os,random,subprocess,tempfile
parser=argparse.ArgumentParser();parser.add_argument('--report',type=Path);args=parser.parse_args()
ROOT=Path(__file__).resolve().parents[2];BASE=ROOT.parents[1];HERE=Path(__file__).resolve().parent
source=(HERE/'full_flow_human_tag_probe.c').read_text();start=source.index('static const wf_actor_raw_t *human_target');end=source.index('wf_status_t human_tag_runtime_frame',start)
policy=BASE/'validation/local-20261024/hardware-input/pad_policy.py';spec=importlib.util.spec_from_file_location('native_pad_policy',policy);py=importlib.util.module_from_spec(spec);spec.loader.exec_module(py)
bridge='''#include <stdint.h>
#include <string.h>
#include "wf_runtime.h"
static unsigned strategy=3,period=4,on=2,reach=8,tolerance=4,bias=0,mash=1;
static unsigned fresh_samples,pin_samples;
'''+source[start:end]+'''
unsigned oracle(const uint8_t*actors,uint32_t frame,unsigned distance,unsigned style,unsigned pulse){wf_runtime_t r={0};memcpy(r.actors,actors,sizeof r.actors);r.g.frame_counter=frame;reach=distance;strategy=style;period=pulse;on=pulse/2;return human_pad(&r);}
'''
rng=random.Random(0x3c4a);groups={};count=0
def put(a,off,v,n=2):a[off:off+n]=(v&((1<<(n*8))-1)).to_bytes(n,'big')
def states():
 for i in range(4096):
  actors=[bytearray(rng.randbytes(0x10c))for _ in range(9)]
  for a in actors:a[0]=rng.choice((0,0x80,0xff));a[0x33]=rng.choice((0,1,4,5,0x81,0x85,0xff))
  h=actors[0];h[0x21]=rng.choice((0,1,2,4,5,6,7,0xff));h[0x61]=rng.choice((0,0x15,0x16,0x48,0x4a,0x52,0x53,0x7b,0xff));put(h,0x7a,rng.choice((0,0xdeadbeef,0x1c05b1,*[0x1c05b0+j*0x10c|tag for j in range(9)for tag in(0,0x40000000,0xff000000)])),4)
  yield '4096-biased-fields-aliases',actors,rng.choice((0,1,2,3,4,5,7,8,0x7fffffff,0x80000000,0xffffffff))
 for dx,dy,frame in itertools.product((-105,-104,-103,-25,-24,-23,-9,-8,0,8,9,23,24,25,103,104,105),(-33,-32,-31,-5,-4,0,4,5,23,24,25),range(8)):
  actors=[bytearray(0x10c)for _ in range(9)];h,t=actors[0],actors[4];put(h,6,0xfff0);put(h,10,0xfff0);put(t,6,0xfff0+dx);put(t,10,0xfff0+dy);t[0]=0x80;t[0x33]=1;h[0x21]=frame%3
  yield 'bounds-WORD-wrap-and-input-pulse',actors,frame
 for (state,selector),sub,frame in itertools.product(((0,0),(1,0),(2,0),(4,0),(5,0),(5,0x15),(5,0x16),(6,0)),(5,6,7,8,9,10),(0,1,2,3,4,7,8,9,10,15)):
  actors=[bytearray(0x10c)for _ in range(9)];h,t=actors[0],actors[4]
  h[0x21]=state;h[0x61]=selector;t[0]=0x80;t[0x33]=1;t[0x21]=4;t[0x65]=sub
  yield 'near-grounded-rival-pin-priority-and-grapple-mash',actors,frame
with tempfile.TemporaryDirectory(prefix='wf-human-policy-',ignore_cleanup_errors=os.name=='nt')as td:
 t=Path(td);(t/'oracle.c').write_text(bridge);lib=t/'policy.so';subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-I'+str(ROOT/'include'),str(t/'oracle.c'),'-o',str(lib)],check=True);native=ctypes.CDLL(str(lib));native.oracle.argtypes=[ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint];native.oracle.restype=ctypes.c_uint
 for group,actors,frame in states():
  for distance,style,pulse in itertools.product((8,24),(3,4),(4,8)):
   before=b''.join(actors);want=native.oracle(ctypes.create_string_buffer(before),frame,distance,style,pulse);got=py.human_tag(actors,frame,reach=distance,strategy=style,period=pulse);assert b''.join(actors)==before,'Python mutated actor input';assert got==want,(count,group,frame,distance,style,pulse,hex(got),hex(want));count+=1;groups[group]=groups.get(group,0)+1
result={'status':'PASS','cases':count,'groups':groups,'python_sha256':hashlib.sha256(policy.read_bytes()).hexdigest(),'C_policy_source_sha256':hashlib.sha256((HERE/'full_flow_human_tag_probe.c').read_bytes()).hexdigest(),'extracted_functions_sha256':hashlib.sha256(source[start:end].encode()).hexdigest(),'bridge_sha256':hashlib.sha256(bridge.encode()).hexdigest(),'compared':'Exact active-low byte; readonly inputs; actual C human_target/human_pad body extracted verbatim','limits':['Controller equivalence only, not engine equivalence or native SH2 timing','All fixtures are isolated controller tests; no combat runtime is rewritten']}
report=args.report or BASE/'validation/local-20261024/human-tag-policy-reach-native.json';report.write_text(json.dumps(result,indent=2)+'\n');print('PASS',count,'C versus hardware Python normal pad bytes, unchanged actor input')
