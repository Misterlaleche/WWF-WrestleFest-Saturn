#!/usr/bin/env python3
"""Execute the production CD transport with a write-zero-to-clear MMIO device.

C++ register proxies model hardware reads/writes; the extracted C functions are
unchanged except the Saturn pointer-width cast at the DMA alignment boundary.
No generated ROM is used: the transfer case reads cd/ROM68K.BIN.
"""
from pathlib import Path
import argparse
import hashlib
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    match = re.search(r"^(?:static\s+)?(?:__jo_force_inline\s+)?(?:int|bool|void)\s+"
                      + re.escape(name) + r"\([^;]*?\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError("Missing production function: " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end].replace("(unsigned int)buf", "(uintptr_t)buf")


PREFIX = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#define JO_ZERO(x) ((x)=0)
#define JO_NULL nullptr
#define JO_MAX(a,b) ((a)>(b)?(a):(b))
#define JO_MULT_BY_8(x) ((x)*8u)
#define __jo_force_inline inline
#define JO_SECTOR_SIZE 2048u
#define JO_FS_MAX_FILES 256
#define JO_OPEN_MAX 1
static unsigned hirq=0x41, cr[4], written[4], commands[256], sr=0x30;
static unsigned frames, frame_step, hirq_reads, irq_errors, read_mask;
static unsigned answer, response_status=0x0100, response_command=0x51;
static unsigned inject_event, services, clock_every;
static bool acknowledge=true, complete_flags=true;
static size_t fifo_at;
static std::vector<unsigned char> payload;
struct Register {
    int index;
    operator unsigned int() const {
        if(index==0) {
            ++hirq_reads;
            if((sr&0xf0)!=0xf0 && frame_step) frames+=frame_step;
            return hirq;
        }
        if(index>=1 && index<=4) {
            if((sr&0xf0)!=0xf0) ++irq_errors;
            read_mask|=1u<<(index-1);
            if(index==4 && inject_event) { hirq|=inject_event; inject_event=0; }
            return cr[index-1];
        }
        return 0;
    }
    unsigned int operator=(unsigned int value) {
        if(index==0) { hirq &= value; return value; }
        assert(index>=1 && index<=4);
        if((sr&0xf0)!=0xf0) ++irq_errors;
        written[index-1]=value;
        if(index==4) {
            unsigned command=written[0]>>8;
            ++commands[command]; read_mask=0;
            cr[0]=command==response_command?response_status:0x0100;
            cr[1]=cr[2]=cr[3]=0;
            if(command==0x51) cr[3]=answer;
            if(command==0x63) { assert(written[3]<=answer); answer-=written[3]; }
            if(acknowledge) hirq|=1;
            if(complete_flags) hirq|=0x42;
        }
        return value;
    }
};
static Register reg_hirq={0},reg_cr1={1},reg_cr2={2},reg_cr3={3},reg_cr4={4},reg_info={5},reg_data={6};
#define JO_CD_HIRQ reg_hirq
#define JO_CD_CR1 reg_cr1
#define JO_CD_CR2 reg_cr2
#define JO_CD_CR3 reg_cr3
#define JO_CD_CR4 reg_cr4
#define JO_CD_INFO reg_info
#define JO_CD_DATA reg_data
static unsigned jo_core_get_vblank_count(){return frames;}
static int jo_core_get_refresh_rate(){return 60;}
static unsigned __jo_core_disable_interrupts(){unsigned old=sr;sr|=0xf0;return old;}
static void __jo_core_restore_interrupts(unsigned old){sr=old;}
static void __jo_core_pause(){}
static void __jo_dma_read_port32(const volatile void *port,void *out,unsigned bytes){
    assert(port==&reg_data && fifo_at+bytes<=payload.size());
    memcpy(out,payload.data()+fifo_at,bytes);fifo_at+=bytes;
}
static void cooperate(){
    assert((sr&0xf0)!=0xf0);++services;
    if(clock_every && services%clock_every==0) ++frames;
}
'''

SUFFIX = r'''
#define CHECK(c,message) do{if(!(c)){fprintf(stderr,"FAIL %s: %s (queries=%u fields=%u hirq_reads=%u)\n",argv[1],message,commands[0x51],frames,hirq_reads);return 1;}}while(0)
int main(int argc,char **argv){
    assert(argc==3);__jo_cd_initialized=true;
    const char *name=argv[1];
    if(!strcmp(name,"empty_poll")){
        for(unsigned i=0;i<10000;++i) CHECK(jo_cd_sector_count()==0,"empty result");
        CHECK(commands[0x51]==1,"empty polls must not flood command 0x51");
    }else if(!strcmp(name,"events")){
        hirq|=0x404;
        CHECK(jo_cd_sector_count()==0,"initial empty");
        CHECK(jo_cd_sector_count()==0 && commands[0x51]==1,"stale events must be acknowledged before query");
        hirq|=4;CHECK(jo_cd_sector_count()==0 && commands[0x51]==2,"CSCT must wake polling");
        hirq|=0x400;CHECK(jo_cd_sector_count()==0 && commands[0x51]==3,"SCDQ must wake polling");
        ++frames;CHECK(jo_cd_sector_count()==0 && commands[0x51]==4,"VBlank fallback must wake polling");
    }else if(!strcmp(name,"arrival_race")){
        inject_event=4;CHECK(jo_cd_sector_count()==0,"first response precedes arrival");
        answer=1;CHECK(jo_cd_sector_count()==1 && commands[0x51]==2,"arrival during response must not be cleared afterwards");
    }else if(!strcmp(name,"new_range")){
        CHECK(jo_cd_sector_count()==0,"initial empty");
        CHECK(jo_cd_start_read(1000,8)==0,"new range");
        CHECK(jo_cd_sector_count()==0 && commands[0x51]==2,"new synchronous range must reset throttle");
        CHECK(__jo_fs_stream_read_async(2000,8)==0,"new asynchronous range");
        CHECK(jo_cd_stream_async_flush()==0,"async setup finishes");
        CHECK(jo_cd_sector_count()==0 && commands[0x51]==3,"new async range must reset throttle");
    }else if(!strcmp(name,"irq_response")){
        CHECK(jo_cd_command(0x5100,0,0,0,0,nullptr)==0,"successful command");
        CHECK(sr==0x30 && irq_errors==0,"command and full response must be atomic; restore original SR");
        CHECK(read_mask==15,"consume all response registers even without output pointer");
    }else if(!strcmp(name,"wait") || !strcmp(name,"reject")){
        response_command=0x40;response_status=!strcmp(name,"wait")?0x8100:0xff00;complete_flags=false;
        CHECK(jo_cd_command(0x4000,0,0,0,0x40,nullptr)<0,"nonaccepted command must fail");
        CHECK(hirq_reads<10,"inspect WAIT/REJECT before waiting for absent ESEL");
        CHECK(sr==0x30 && irq_errors==0,"restore SR on nonaccepted response");
    }else if(!strcmp(name,"cmok_timeout")){
        acknowledge=false;
        CHECK(jo_cd_command(0x5100,0,0,0,0,nullptr)<0,"missing CMOK must time out");
        CHECK(sr==0x30 && irq_errors==0,"restore SR after missing acknowledgement");
    }else if(!strcmp(name,"initial_cmok_timeout")){
        hirq=0;
        CHECK(jo_cd_command(0x5100,0,0,0,0,nullptr)<0,"initial busy controller must time out");
        CHECK(sr==0x30 && commands[0x51]==0,"do not issue while CMOK is absent");
    }else if(!strcmp(name,"selector_timeout")){
        complete_flags=false;frame_step=1;
        CHECK(jo_cd_command(0x4000,0,0,0,0x40,nullptr)<0,"missing selector completion must time out");
        CHECK(sr==0x30 && irq_errors==0 && frames>10,"selector wait must allow interrupts after consuming response");
    }else if(!strcmp(name,"no_clock_timeout") || !strcmp(name,"field_timeout") || !strcmp(name,"moving_clock")){
        alignas(4) unsigned char buffer[2048];
        __jo_fs_stream_service=cooperate;
        if(!strcmp(name,"field_timeout")) frame_step=10;
        if(!strcmp(name,"moving_clock")) clock_every=50;
        CHECK(jo_cd_read_sectors(1000,1,buffer,sizeof buffer,1001)<0,"empty drive must time out");
        CHECK(services>1,"empty waits must keep cooperative audio service");
        CHECK(sr==0x30,"restore SR after sector timeout");
        if(!strcmp(name,"no_clock_timeout")) CHECK(commands[0x51]==1,"clockless fallback must not issue repeated queries");
        if(!strcmp(name,"field_timeout")) CHECK(frames<=500,"live VBlank deadline must bound elapsed wait");
        if(!strcmp(name,"moving_clock")) CHECK(frames>=300 && frames<=302,"spin fallback resets when VBlank progresses");
    }else if(!strcmp(name,"real_payload")){
        FILE *file=fopen(argv[2],"rb");CHECK(file,"real ROM asset is required");
        payload.resize(8192);CHECK(fread(payload.data(),1,payload.size(),file)==payload.size(),"read real asset");fclose(file);
        alignas(4) unsigned char buffer[8192];answer=4;
        CHECK(jo_cd_read_sectors(1000,4,buffer,sizeof buffer,1004)==0,"normal transfer");
        CHECK(!memcmp(buffer,payload.data(),sizeof buffer),"transport must preserve real ROM bytes");
        CHECK(fifo_at==8192 && commands[0x51]==1 && commands[0x63]==1 && commands[0x06]==1,"one complete transfer");
    }else return 2;
    printf("PASS %s: queries=%u fields=%u services=%u\n",name,commands[0x51],frames,services);return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset-dir', type=Path, default=ROOT / 'cd')
    parser.add_argument('--case', action='append')
    args = parser.parse_args()
    source = (ROOT / 'sdk/jo_engine/fs.c').read_text()
    constants = '\n'.join(line for line in source[:source.index('typedef struct')].splitlines()
                          if re.match(r'^#\s*define JO_(CD_|FS_FILENAME)', line)
                          and not re.match(r'^#\s*define JO_CD_(HIRQ|CR[1-4]|DATA|INFO)\s', line))
    begin = source.index('typedef struct\n{\n    unsigned short')
    end = source.index('#include "internal/cd_stream_async.h"', begin)
    declarations = source[begin:end] + '#include "internal/cd_stream_async.h"\n'
    names = ['jo_cd_command', 'jo_cd_end_transfer', 'jo_cd_reset_partition',
             'jo_cd_sector_count', 'jo_cd_init', 'jo_cd_stop_read',
             'jo_cd_start_read', 'jo_cd_read_sectors', '__jo_fs_stream_read_async',
             '__jo_fs_stream_stop_async']
    body = '\n'.join(function(source, name) for name in names)
    program = (PREFIX + constants + '\n#undef JO_CD_TIMEOUT\n#define JO_CD_TIMEOUT 64u\n'
               '#undef JO_CD_SECTOR_TIMEOUT\n#define JO_CD_SECTOR_TIMEOUT 128u\n'
               + declarations + '\nstatic unsigned __jo_fs_stat_vblanks,__jo_fs_stat_bytes;\n'
               + body + SUFFIX)
    cases = args.case or ['empty_poll', 'events', 'arrival_race', 'new_range',
                         'irq_response', 'wait', 'reject', 'cmok_timeout',
                         'initial_cmok_timeout', 'selector_timeout',
                         'no_clock_timeout', 'field_timeout', 'moving_clock', 'real_payload']
    failures = 0
    with tempfile.TemporaryDirectory(prefix='wf-cd-protocol-') as directory:
        cpp, binary = Path(directory) / 'protocol.cpp', Path(directory) / 'protocol'
        cpp.write_text(program)
        subprocess.run(['c++', '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-Wno-unused-function', '-Wno-unused-variable',
                        '-I'+str(ROOT/'sdk/jo_engine'), str(cpp), '-o', str(binary)], check=True)
        for case in cases:
            result = subprocess.run([str(binary), case, str(args.asset_dir/'ROM68K.BIN')],
                                    capture_output=True, text=True, timeout=10)
            print((result.stdout + result.stderr).strip())
            failures += result.returncode != 0
    print(f'{len(cases)-failures}/{len(cases)} CD protocol cases passed')
    if 'real_payload' in cases:
        print('Real asset SHA256:', hashlib.sha256((args.asset_dir/'ROM68K.BIN').read_bytes()).hexdigest())
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
