"""Execute exact main frontend_step/fail bodies with controlled boundary errors."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/"build/main-frontend-fault-tests"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=ROOT/'port/main.c')
parser.add_argument('--label', default='current')
parser.add_argument('--output', type=Path)
args = parser.parse_args()
source = args.source.read_text()

def function(name):
    start = re.search(r'static (?:void|int) '+name+r'\([^;]*?\)\s*\{', source)
    assert start, name
    end = start.end()
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start.start():end]

diagnostic = re.search(r'typedef struct \{.*?\} wf_jo_diagnostic_t;', source, re.S).group()
code = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include "wf_jo_frontend.h"
''' + diagnostic + r'''
static wf_jo_frontend_t frontend;
static wf_runtime_t rt;
static volatile wf_jo_diagnostic_t wf_jo_diagnostic;
static unsigned frontend_state,failed,running,selected_royal,loaded,loading_text_visible;
static char loading_file[16];
static uint32_t wf_jo_load_diagnostic[3];
static wf_status_t run_status,flush_status;
static unsigned video_ready,cleared,printed,prepared;
static unsigned emergency_calls,drain_calls,pause_allowed;
static wf_status_t drain_status;
static jmp_buf drain_escape;
static void __attribute__((unused)) wf_jo_audio_emergency_stop(void){++emergency_calls;}
#define JO_KEY_START 1
static void wf_jo_assets_set_loading_observer(void *p){assert(!p);assert(emergency_calls && "mute must precede observers");}
static wf_status_t wf_jo_video_drain_overlap(void *p,void *u){assert(!p&&!u);assert(emergency_calls && "mute must precede drain");++drain_calls;return drain_status;}
static int jo_is_pad1_key_down(unsigned key){(void)key;return 0;}
static void jo_goto_boot_menu(void){assert(!"unexpected reset");}
static void __jo_core_pause(void){assert(pause_allowed&&emergency_calls);longjmp(drain_escape,1);}
static void wf_jo_video_diagnostics(void){++video_ready;}
static void jo_clear_screen(void){assert(video_ready);++cleared;}
static void jo_printf(int x,int y,const char *s,...){(void)x;(void)y;(void)s;assert(video_ready&&cleared);++printed;}
/* UI contents are tested by the CD diagnostic suite; this boundary must run
 * only after fail() has drained/restored the text display. */
static void loading_cd_summary(unsigned row,unsigned detail){assert(row==20&&detail==1&&video_ready&&cleared);}
wf_status_t wf_jo_frontend_run(wf_jo_frontend_t *f){assert(f==&frontend);return run_status;}
wf_status_t wf_jo_frontend_flush(wf_jo_frontend_t *f){assert(f==&frontend);return flush_status;}
static int prepare_gameplay(void){++prepared;return 1;}
''' + function('fail') + '\n' + function('frontend_step') + r'''
static void reset(void){
 memset(&frontend,0,sizeof frontend);memset(&rt,0,sizeof rt);
 memset((void*)&wf_jo_diagnostic,0,sizeof wf_jo_diagnostic);
 frontend.driver.pc=0x52be;frontend.driver.phase=WF_R18_DRIVER_MODE_PLATE;
 rt.g.frame_counter=929;rt.g.rng_state=0x78b7d0ee;
 frontend_state=1;failed=0;running=1;loaded=1;loading_text_visible=1;
 run_status=flush_status=drain_status=WF_OK;video_ready=cleared=printed=prepared=0;
 emergency_calls=drain_calls=pause_allowed=0;
}
static void fatal_case(unsigned route,uint32_t owner,uint32_t dependency,
                       uint32_t pc,unsigned phase,uint32_t driver_dep,
                       uint32_t want_owner,uint32_t want_dependency){
 reset();frontend.blocked_pc=owner;frontend.blocked_dependency=dependency;
 frontend.driver.pc=pc;frontend.driver.phase=phase;frontend.driver.blocked_dependency=driver_dep;
 frontend_state=route;if(route==1)run_status=WF_BAD_STATE;else flush_status=WF_BAD_STATE;
 wf_jo_frontend_t old=frontend;wf_runtime_t old_rt=rt;
 assert(frontend_step()==0&&failed==1&&running==0&&frontend_state==route);
 assert(wf_jo_diagnostic.fault_owner==want_owner);
 assert(wf_jo_diagnostic.fault_dependency==want_dependency);
 assert(wf_jo_diagnostic.ticks==929&&wf_jo_diagnostic.rng==0x78b7d0ee);
 assert(!memcmp(&old,&frontend,sizeof old)&&!memcmp(&old_rt,&rt,sizeof rt));
 assert(video_ready==1&&cleared==1&&printed&&!prepared&&!loading_text_visible);
 assert(emergency_calls==1&&drain_calls==1);
}
int main(void){
 fatal_case(1,0,0,0x52be,3,0,0x52be,0x53540002);
 fatal_case(2,0,0,0x52be,3,0,0x52be,0x53540002);
 fatal_case(1,0x135b0,0xf4c2,0x52be,7,0x83a4,0x135b0,0xf4c2);
 fatal_case(2,0x135b0,0xf4c2,0x52be,7,0x83a4,0x135b0,0xf4c2);
 fatal_case(1,0,0,0x52be,7,0x83a4,0x52be,0x83a4);
 fatal_case(1,0,0,0x52be,3,0x83a4,0x52be,0x53540002);
 fatal_case(1,0,0,0,3,0,0x46524e54,0x53540002);
 reset();frontend_state=2;flush_status=WF_BLOCKED;
 assert(frontend_step()==1&&!failed&&running&&frontend_state==2&&!video_ready&&!prepared);
 reset();frontend_state=2;flush_status=WF_BLOCKED;frontend.blocked_pc=0x456;frontend.blocked_dependency=0x789;
 assert(frontend_step()==0&&failed&&wf_jo_diagnostic.fault_owner==0x456&&wf_jo_diagnostic.fault_dependency==0x789);
 reset();run_status=WF_BLOCKED;
 assert(frontend_step()==0&&failed&&wf_jo_diagnostic.fault_owner==0x52be&&wf_jo_diagnostic.fault_dependency==0x53540001);
 reset();rt.g.mode_0161=1;
 assert(frontend_step()==1&&!failed&&frontend_state==0&&selected_royal==1&&prepared==1&&!video_ready);
 assert(!emergency_calls && "healthy frontend must not silence audio");
 /* A new error after the first must not replace its location, tick or RNG. */
 reset();fail(0x1f914,0x1c1688);
 wf_jo_diagnostic_t first=wf_jo_diagnostic;
 unsigned first_printed=printed;
 rt.g.frame_counter=123456;rt.g.rng_state=0;
 fail(0xdead,0xbeef);
 assert(!memcmp((const void*)&wf_jo_diagnostic,&first,sizeof first));
 assert(drain_calls==1&&printed==first_printed&&emergency_calls==2);
 /* The early loader can fail before the adapter initializes. */
 reset();loaded=0;fail(0x18cd10,1);
 assert(emergency_calls==1&&drain_calls==1&&failed&&!running&&printed);
 /* Drain failure can never leave sound active or replace the first fault. */
 reset();drain_status=WF_BLOCKED;pause_allowed=1;
 if(!setjmp(drain_escape))fail(0x1f914,0x1c1688);
 assert(emergency_calls==1&&drain_calls==1&&failed&&!running);
 assert(!video_ready&&!cleared&&!printed);
 assert(wf_jo_diagnostic.fault_owner==0x1f914&&wf_jo_diagnostic.fault_dependency==0x1c1688);
 assert(wf_jo_diagnostic.ticks==929&&wf_jo_diagnostic.rng==0x78b7d0ee);
 puts("PASS 14 cases: 11 frontend boundaries + repeated fault + cold load + blocked drain; mute precedes dependencies");
}
'''
out = (args.output or BASE/'evidence'/args.label).resolve()
out.mkdir(parents=True, exist_ok=True)
(out/'test.c').write_text(code)
command = ['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
           '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
           '-I'+str(ROOT/'port'),'-I'+str(ROOT/'core/include'),str(out/'test.c'),'-o',str(out/'test')]
subprocess.run(command,check=True)
result = subprocess.run([str(out/'test')],capture_output=True,text=True,
                        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
(out/'run.log').write_text(result.stdout+result.stderr)
(out/'report.json').write_text(json.dumps({'source_sha256':hashlib.sha256(args.source.read_bytes()).hexdigest(),
 'returncode':result.returncode,'command':command,'output':result.stdout+result.stderr},indent=2)+'\n')
print(result.stdout+result.stderr)
raise SystemExit(result.returncode != 0)
