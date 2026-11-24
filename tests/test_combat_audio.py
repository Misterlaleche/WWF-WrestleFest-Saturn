#!/usr/bin/env python3
"""Trace native combat through the real Jo descriptor path (no console hardware)."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
s=(root/'sdk/jo_engine/audio.c').read_text()
names=['jo_audio_channel_status','jo_audio_channel_slot','jo_audio_pan','jo_audio_key_off','jo_audio_wait_samples','jo_audio_start_slots','jo_audio_flush','jo_audio_sync','jo_audio_program_slot','jo_audio_mixer','jo_audio_key_on','jo_audio_channel_on','jo_audio_channel_off','jo_audio_stop_channels_in','jo_audio_is_channel_playing','jo_audio_play_sound_on_channel','jo_audio_reserve_resident','jo_audio_write_resident','jo_audio_stop_channel']
def extract(name):
 m=re.search(r'(?m)^(?:static[^\n]*?\s+)?(?:bool|void|unsigned int|unsigned short)\s+'+name+r'\([^;]*?\)\s*\{',s)
 if not m:raise ValueError(name)
 d=1;j=m.end()
 while d:d+=(s[j]=='{')-(s[j]=='}');j+=1
 return s[m.start():j]
pre='''#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#define JO_NULL NULL
#define JO_ZERO(x) ((x)=0)
#define JO_DIV_BY_2(x) ((x)/2u)
#define JO_MULT_BY_2(x) ((x)*2u)
#define __jo_force_inline inline
#include "jo/audio.h"
#include "jo/sega_saturn.h"
#include "internal/sega_saturn.h"
#undef JO_SOUND_RAM
#undef JO_SCSP_SLOT
#undef JO_SCSP_COMMON
static unsigned char sound_ram[0x80000] __attribute__((aligned(32)));
static unsigned short slots[32][16], common[32];
#define JO_SOUND_RAM ((uintptr_t)sound_ram)
#define JO_SCSP_SLOT(s,r) (slots[(s)][(r)/2])
#define JO_SCSP_COMMON(r) (common[(r)/2])
#define JO_AUDIO_PCM_AREA_START 0x1000u
#define JO_AUDIO_PCM_AREA_END 0x80000u
#define JO_AUDIO_COPIED_ROOM 0x8000u
#define JO_AUDIO_ALIGN(n) (((n)+3u)&~3u)
#define JO_AUDIO_CACHE_ENTRIES 64u
#define JO_AUDIO_SLOT_REGISTERS 12
#define JO_AUDIO_PENDING_SLOTS 2u
#define JO_AUDIO_KEY_OFF_WAIT 96u
#define JO_AUDIO_MAX_SAMPLES 65535u
#define JO_AUDIO_LOOP_VBLANKS 0x3fffffffu
'''
a=s.index('typedef struct\n');b=s.index('static jo_audio_cache_entry',a)
pre+=s[a:b]
pre+='''static jo_audio_cache_entry __jo_audio_cache[64];
static unsigned __jo_audio_resident_start=0x80000u,__jo_audio_alloc_next=0x1000u;
static jo_audio_channel __jo_audio_channels[14];
static unsigned short __jo_audio_pending_regs[2][12],__jo_audio_direct_mix[2],__jo_audio_effect_mix[2];
static unsigned char __jo_audio_pending_slot[2];
static unsigned __jo_audio_pending_count;
unsigned __jo_core_vblank_count;
static unsigned jo_core_get_refresh_rate(void){return 60;}
static void jo_dma_copy(const void*s,void*d,unsigned n){memcpy(d,s,n);}
static unsigned jo_audio_upload_pcm(const void*d,unsigned n){(void)d;(void)n;assert(0);return 0;}
'''
post='''
void wf_test_driver_reset(void){
 memset(sound_ram,0xa5,sizeof sound_ram);memset(slots,0,sizeof slots);memset(common,0,sizeof common);
 memset(__jo_audio_channels,0,sizeof __jo_audio_channels);memset(__jo_audio_cache,0,sizeof __jo_audio_cache);
 __jo_audio_resident_start=0x80000u-65568u;__jo_audio_alloc_next=0x1000;__jo_audio_pending_count=0;__jo_core_vblank_count=0;
}
unsigned wf_test_driver_register(unsigned slot,unsigned reg){assert(slot<32&&reg<0x20);return slots[slot][reg/2u];}
unsigned wf_test_driver_pcm_be16(unsigned at){assert(at+1u<sizeof sound_ram);return sound_ram[at]*256u+sound_ram[at+1u];}
'''

def verify_rom_traces(rom, traces):
 """Execute unchanged arcade code, including the real $2052 DIP/strobe gate.

 The oracle observes the hardware write instruction at $206C. It neither
 substitutes the callback nor predicts sounds from a duplicated state table.
 """
 import machine68k as mus
 machine=mus.Machine(mus.CPUType.M68000,2048)
 cpu,mem=machine.cpu,machine.mem
 mem.w_block(0,rom)
 source=0x1c05b0;target=source+0x10c
 for line in traces:
  _,kind,*values=line.split()
  values=list(map(int,values));a,b,c,d=values[:4]
  expected=values[10:] if kind=='dispatch' else values[4:]
  mem.w_block(0x1c0000,bytes(0x4000));mem.w_block(0x140000,bytes(0x40))
  cpu.w_sr(0x2700)
  for reg in range(15):cpu.w_reg(reg,0)
  cpu.w_sp(0x1c3ff0);mem.w32(0x1c3ff0,0x70000)
  cpu.w_reg(mus.Register.A0,source);cpu.w_reg(mus.Register.A1,target)
  mem.w16(0x1c007c,1)
  if kind=='collision':
   mem.w16(0x1c007c,a);mem.w16(0x1c0066,b)
   for ptr,state,profile,x in ((source,0x8005,7,0x280),(target,0x8000,1,0x270)):
    for offset,value in ((0,0x8000),(0x20,state),(0x4c,profile),(6,x),(10,0x160),(14,0x140)):
     mem.w16(ptr+offset,value)
   pc=0x24062
  elif kind=='selector':
   mem.w16(source+0x4c,7);mem.w16(target+0x4c,(b<<8)|1)
   mem.w16(target+0x2e,0x8000);mem.w8(target+0x33,16)
   cpu.w_reg(mus.Register.D1,1)
   cpu.w_reg(mus.Register.A2,0x24ef6+7*7)
   pc=int.from_bytes(rom[0x24398+a*4:0x2439c+a*4],'big')
  elif kind=='landing':
   for offset,value in ((0x1c,0x8004),(0x20,0x8004),(0x64,a),(0x24,b),
                        (0x22,1),(0x36,d),(6,0x280),(10,0x160),(14,0x140)):
    mem.w16(source+offset,value)
   mem.w8(source+0x33,4 if c else 0)
   pc=0x1c03e
  elif kind=='dispatch':
   domain,subtype,latched,frame,timer,contact,flags,mode,scene,prior=values[:10]
   for offset,value in ((0x1c,domain|(0x8000 if latched else 0)),(0x20,0x8000|domain),
                        (0x64,subtype),(0x44,subtype if domain==8 else prior),
                        (0x24,frame),(0x22,timer),(0x36,contact),
                        (6,0x280),(10,0x160),(14,0x140)):
    mem.w16(source+offset,value)
   mem.w8(source+0x33,flags);mem.w8(0x1c0161,mode);mem.w16(0x1c007e,scene)
   mem.w32(source+0x26,target);mem.w32(target+0x26,source)
   pc=0x1c03e
  else:raise AssertionError(kind)
  cpu.w_pc(pc);observed=[]
  for instruction in range(10000):
   pc=cpu.r_pc()
   if pc==0x70000:break
   if pc==0x206c:observed.append(cpu.r_reg(mus.Register.D0)&0xffff)
   cpu.execute(1)
  else:raise AssertionError(('ROM did not return',line,hex(cpu.r_pc())))
  assert observed==expected,('native/unchanged ROM audio mismatch',line,observed)
 print(f'PASS: {len(traces)} combat traces match original 68000 hardware writes at $206C')

if __name__=='__main__':
 import argparse, os, subprocess, tempfile, concurrent.futures
 parser=argparse.ArgumentParser();parser.add_argument('--baseline',type=Path)
 parser.add_argument('--assets',type=Path,help='Packaged SFX/ROM/WAV directory (default: source/cd)')
 parser.add_argument('--rom-oracle',action='store_true');args=parser.parse_args()
 production=args.baseline.resolve() if args.baseline else root
 assets=(args.assets or production/'cd').resolve()
 with tempfile.TemporaryDirectory(prefix='wf-combat-audio-') as tmp:
  work=Path(tmp);driver=work/'driver.c';driver.write_text(pre+'\n'.join(extract(n) for n in names)+post)
  flags=['-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-pointer-to-int-cast','-ffunction-sections','-fdata-sections','-DWF_JO_ENGINE','-DWF_JO_AUDIO_HOST','-DJO_COMPILE_WITH_AUDIO_SUPPORT=','-DJO_COMPILE_WITH_FS_SUPPORT=']
  flags+=['-I'+str(production/p) for p in ['core/include','core/src','port','sdk/jo_engine']]
  sources=[production/p for p in dict.fromkeys(re.findall(r'core/(?:src|generated)/\w+\.c',(production/'core_sources.mk').read_text()))]
  sources += [production/'port/wf_jo_audio.c',root/'tests/test_combat_audio.c',driver]
  def compile(source):
   obj=work/(source.stem+'.o')
   subprocess.run(['cc',*flags,'-c',str(source),'-o',str(obj)],check=True)
   return str(obj)
  with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile,sources))
  subprocess.run(['cc','-Wl,--gc-sections',*objects,'-o',str(work/'test')],check=True)
  result=subprocess.run([str(work/'test'),str(assets/'SFX.DAT')],capture_output=True,text=True,
                        env=dict(os.environ,WF_TEST_CD=str(assets)))
  if result.returncode:
   print(result.stdout,end='');print(result.stderr,end='');result.check_returncode()
  lines=result.stdout.splitlines();traces=[line for line in lines if line.startswith('TRACE ')]
  for line in lines:
   if not line.startswith('TRACE '):print(line)
  if args.rom_oracle:verify_rom_traces((assets/'ROM68K.BIN').read_bytes(),traces)

