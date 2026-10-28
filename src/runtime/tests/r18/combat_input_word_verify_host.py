"""Compile the actual HOST input adapter; verify full physical words/guards."""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path);a=p.parse_args()
source=root/'tests/r18/full_flow_combat_probe.c';text=source.read_text()
start=text.index('static wf_status_t combat_input(');brace=text.index('{',start);depth=1;end=brace+1
while depth:
 depth+=(text[end]=='{')-(text[end]=='}');end+=1
body=text[start:end]
test='''
#include "wf_gameplay_bindings.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static wf_gameplay_binding_context_t combat_gc;
'''+body+'''
int main(void){
 unsigned cases=0;
 for(unsigned level=0;level<256;level++){
  memset(&combat_gc,0x5a,sizeof combat_gc);combat_gc.input_hw_low[0]=(uint8_t)level;
  wf_gameplay_binding_context_t before=combat_gc;
  const uint32_t tags[]={0,0x40000000u,0x80000000u,0xff000000u};
  for(unsigned tag=0;tag<4;tag++)for(unsigned port=0;port<4;port++){
   uint16_t word=0x6a35;
   assert(combat_input(NULL,(0x140020u+port*2u)|tags[tag],&word,NULL)==WF_OK);
   assert(word==(port?0xffffu:(0xff00u|level)));
   assert(!memcmp(&before,&combat_gc,sizeof before));cases++;
  }
 }
 const uint32_t bad[]={0,0x14001e,0x140021,0x140023,0x140025,0x140027,0x140028,0xffffff};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++){
  uint16_t word=0x6a35;assert(combat_input(NULL,bad[i],&word,NULL)==WF_BAD_STATE);
  assert(word==0x6a35);cases++;
 }
 assert(combat_input(NULL,0x140020,NULL,NULL)==WF_BAD_STATE);cases++;
 printf("PASS %u physical WORD/guard controls, no state mutation\\n",cases);return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='wf-input-word-',ignore_cleanup_errors=os.name=='nt')as t:
 path=Path(t)/'adapter.c';path.write_text(test);exe=Path(t)/'adapter.exe'
 subprocess.run(['cc','-std=c11','-O2','-UNDEBUG','-Wall','-Wextra','-Werror','-Iinclude',str(path),'-o',str(exe)],cwd=root,check=True)
 q=subprocess.run([str(exe)],capture_output=True,text=True);print(q.stdout,end='');print(q.stderr,end='');assert q.returncode==0,q.returncode
 if a.report:a.report.write_text(json.dumps({'status':'PASS','controls':4105,'adapter_sha256':hashlib.sha256(body.encode()).hexdigest(),'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'limits':'Validation adapter only, one physical pad, actual24bit bus aliases; original engine controls are checked separately.'},indent=2)+'\n')
