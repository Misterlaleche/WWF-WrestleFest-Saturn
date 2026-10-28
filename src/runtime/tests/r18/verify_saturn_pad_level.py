#!/usr/bin/env python3
"""Run the actual Saturn controls() body against sampled SMPC level/edge states.

The pinned Yaul digital_port object stores current at pressed, current&changed
at held, and previous&changed at released. The HOST fixture models those
semantics with named fields; it does not assume HOST/SH-2 bitfield ordering.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
entry = root / 'src/wf_saturn_entry.c'
source = entry.read_text()
start = source.index('static uint8_t controls(const smpc_peripheral_digital_t *sample)')
brace = source.index('{', start)
end = brace + 1
depth = 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
controls = source[start:end]
fixture = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct { struct { unsigned right,left,down,up,a,b,c,start; } button; } buttons_t;
typedef struct { unsigned connected; buttons_t pressed,held,released; } smpc_peripheral_digital_t;
static smpc_peripheral_digital_t pad;
'''
fixture += controls
fixture += r'''
static unsigned checks;
static uint8_t previous;
static void fill(buttons_t *out,uint8_t bits){
 out->button.right=(bits>>0)&1;out->button.left=(bits>>1)&1;
 out->button.up=(bits>>2)&1;out->button.down=(bits>>3)&1;
 out->button.a=(bits>>4)&1;out->button.b=(bits>>5)&1;
 out->button.c=(bits>>6)&1;out->button.start=(bits>>7)&1;
}
static int sample(uint8_t current,uint8_t expected,const char *name){
 uint8_t changed=previous^current;
 /* Native SDK digital_port: pressed=current; held=current&changed;
  * released=previous&changed. Both current/previous are active-high. */
 fill(&pad.pressed,current);fill(&pad.held,current&changed);
 fill(&pad.released,previous&changed);previous=current;
 uint8_t got=controls(&pad);++checks;
 if(got!=expected){fprintf(stderr,"%s: expected %02x, got %02x\n",name,expected,got);return 1;}
 return 0;
}
int main(void){
 pad.connected=1;
 if(sample(0,255,"idle"))return 1;
 for(unsigned bit=0;bit<8;bit++){
  uint8_t active=(uint8_t)(1u<<bit);
  uint8_t expected=(uint8_t)~active;
  if(sample(active,expected,"initial press"))return 1;
  for(unsigned frame=0;frame<120;frame++)
   if(sample(active,expected,"continued held level"))return 1;
  if(sample(0,255,"release edge"))return 1;
  if(sample(0,255,"continued idle"))return 1;
 }
 for(unsigned active=0;active<256;active++){
  if(sample((uint8_t)active,(uint8_t)~active,"combined buttons"))return 1;
 }
 pad.connected=0;
 if(sample(255,255,"disconnected with stale fields"))return 1;
 pad.connected=1;
 if(sample(255,0,"reconnected held level"))return 1;
 if(sample(0,255,"release after reconnect"))return 1;
 printf("PASS %u level/edge samples: all buttons,120-frame holds, release, combinations, disconnect\n",checks);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='wf-smpc-level-') as directory:
    directory = Path(directory)
    c = directory / 'controls.c'
    executable = directory / 'controls'
    c.write_text(fixture)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2',
                    str(c), '-o', str(executable)], check=True)
    result = subprocess.run([str(executable)], text=True, capture_output=True)
report = {'result': 'pass' if result.returncode == 0 else 'fail',
          'controls_source_sha256': hashlib.sha256(controls.encode()).hexdigest(),
          'tested_source': str(entry), 'stdout': result.stdout, 'stderr': result.stderr,
          'SDK_semantics': 'pressed=current; held=current&(current^previous); released=previous&(current^previous)',
          'scope': 'Actual controls() function, HOST named-field mock; native SMPC session checked separately'}
text = json.dumps(report, indent=2) + '\n'
if a.output:
    a.output.write_text(text)
print(text, end='')
raise SystemExit(result.returncode)
