#!/usr/bin/env python3
"""Inject deterministic register-read timing into the production helper body.
Only volatile register reads are mechanically replaced with a modeled read.
The independent oracle requires the returned count to have existed during the
read interval, including 16-bit rollover. No game clocks/deadlines change.
"""
from pathlib import Path
import hashlib,json,re,subprocess
root=Path(__file__).resolve().parents[1]
source=(root/'src/runtime/src/wf_r18_scsp.c').read_text()
body=re.search(r'static uint16_t stable_frt_count\(void\)\n\{.*?\n\}',source,re.S).group()
original=body
body=re.sub(r'    volatile cpu_ioregs_t \*const regs=.*?;\n','',body)
body=body.replace('regs->frch','read_high()').replace('regs->frcl','read_low()')
text=r'''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint16_t counter;
static unsigned advance_high,advance_low,reads,total_advance;
static uint16_t read_high(void){uint16_t v=counter>>8;counter=(uint16_t)(counter+advance_high);total_advance+=advance_high;reads++;assert(reads<100);return v;}
static uint16_t read_low(void){uint16_t v=counter&255u;counter=(uint16_t)(counter+advance_low);total_advance+=advance_low;reads++;assert(reads<100);return v;}
static uint16_t old_frt_count(void){uint16_t high=read_high(),low=read_low();return (uint16_t)((high<<8)|low);}
'''+body+r'''
static void reset(uint16_t start){counter=start;reads=total_advance=0;}
int main(void){
 unsigned tests=0,old_torn=0,stable_retries=0;
 for(advance_high=0;advance_high<=2;advance_high++)for(advance_low=0;advance_low<=2;advance_low++)for(unsigned start=0;start<65536u;start++){
  reset((uint16_t)start);uint16_t old=old_frt_count();if((uint16_t)(old-start)>total_advance)old_torn++;
  reset((uint16_t)start);uint16_t stable=stable_frt_count();assert((uint16_t)(stable-start)<=total_advance);if(reads>3)stable_retries++;tests++;
 }
 advance_high=1;advance_low=0;
 reset(0x12ff);uint16_t old=old_frt_count();assert(old==0x1200);
 reset(0x12ff);uint16_t stable=stable_frt_count();assert(stable==0x1302);
 printf("PASS cases=%u old_torn_samples=%u stable_invalid_samples=0 retry_cases=%u example_old=%04x example_stable=%04x\n",tests,old_torn,stable_retries,old,stable);
 return 0;
}
'''
file=root/'ring-test/frt_rollover_generated.c';file.write_text(text)
exe=file.with_suffix('')
cmd=['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(file),'-o',str(exe)]
subprocess.run(cmd,check=True)
result=subprocess.run([str(exe)],check=True,text=True,capture_output=True)
(root/'ring-test/frt-rollover-result.json').write_text(json.dumps({'production_helper_sha256':hashlib.sha256(original.encode()).hexdigest(),'test_method':'Exact helper body extracted from production proposal; register reads replaced with deterministic counter accessors; independent interval oracle exhausts all65536 counts and nine inter-read advance schedules','compile_command':cmd,'stdout':result.stdout},indent=2)+'\n')
print(result.stdout,end='')
