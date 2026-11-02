"""Generate profile-guided control blocks from the pinned MIT core and ROM."""
import argparse,hashlib,json,re
from pathlib import Path

def generate(repo,rom_path,out_path):
 repo,rom_path,out_path=[Path(p).resolve() for p in (repo,rom_path,out_path)]
 rom=rom_path.read_bytes();base=(repo/'src/audio/native/reference_driver/z80.c').read_text()
 meta=json.loads((repo/'src/audio/native/generated/wf_z80_blocks.json').read_text())
 if hashlib.sha256(rom).hexdigest()!=meta['rom_sha256']:raise ValueError('Unsupported control ROM')
 if hashlib.sha256(base.encode()).hexdigest()!=meta['source_sha256']:raise ValueError('Pinned Z80 source changed')
 selected={int(p):b for p,b in meta['blocks'].items()}
 # Replace the primary bit-count loop by its exhaustive256-entry truth table.
 # Sequencer-only memory adapter: ROM/RAM accesses have no I/O side effects.
 mem='static inline uint8_t wf_memory_read(z80 *z,uint16_t address){\n wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;\n if(address<0xc000)return v->rom[address];\n if(address<0xc800)return v->ram[address-0xc000];\n return z->read_byte(z->userdata,address);\n}\nstatic inline void wf_memory_write(z80 *z,uint16_t address,uint8_t value){\n if(address>=0xc000&&address<0xc800){((wf_music_seq_t *)z->userdata)->ram[address-0xc000]=value;return;}\n z->write_byte(z->userdata,address,value);\n}\n'
 base=base.replace('return z->read_byte(z->userdata, addr);','return wf_memory_read(z,addr);').replace('z->write_byte(z->userdata, addr, val);','wf_memory_write(z,addr,val);')
 base=base.replace('z->read_byte(z->userdata, addr + 1)','wf_memory_read(z,addr+1)').replace('z->read_byte(z->userdata, addr)','wf_memory_read(z,addr)').replace('z->write_byte(z->userdata, addr, val & 0xFF)','wf_memory_write(z,addr,val & 0xFF)').replace('z->write_byte(z->userdata, addr + 1, val >> 8)','wf_memory_write(z,addr+1,val >> 8)')
 base=mem+base
 parity_start=base.index('static inline bool parity(uint8_t val) {')
 parity_end=base.index('\n}',parity_start)+2
 parity='static const uint8_t wf_parity[256]={'+','.join(str(int(bin(i).count('1')%2==0)) for i in range(256))+'};\nstatic inline bool parity(uint8_t val){return wf_parity[val]!=0;}'
 base=base[:parity_start]+parity+base[parity_end:]
 # Lex C before matching braces/case labels, preserving positions.
 lex=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:' '*len(m[0]),base,flags=re.S)
 start=base.index('void exec_opcode(z80* const z, uint8_t opcode) {',base.index('// executes a non-prefixed opcode'))
 sw=lex.index('switch (opcode)',start);left=lex.index('{',sw);depth=1;i=left+1;labels=[]
 while depth:
  if lex[i]=='{':depth+=1
  elif lex[i]=='}':depth-=1
  elif depth==1:
   m=re.match(r'case\s+(0x[0-9A-Fa-f]+)\s*:',lex[i:])
   if m:labels.append((int(m[1],16),i,i+len(m[0])));i+=len(m[0])-1
  i+=1
 right=i-1
 cases={}
 for k,(op,a,b) in enumerate(labels):
  end=labels[k+1][1] if k+1<len(labels) else right
  text=base[b:end];text=re.sub(r'//[^\n]*|/\*.*?\*/','',text,flags=re.S).strip()
  if 'default:' in text:text=text[:text.index('default:')]
  cases[op]=text
 for k in reversed(range(len(labels)-1)):
  op=labels[k][0]
  if not cases[op]:cases[op]=cases[labels[k+1][0]]
 # Original executor uses these immutable timing facts.
 timing_match=re.search(r'cyc_00\[256\] = \{(.*?)\};',base,re.S);timings=[int(x) for x in re.findall(r'\d+',timing_match[1])]
 head='''/* Generated from pinned MIT Z80 core and user ROM. General interpreter fallback remains. */
 #include "wf_music_seq.h"
 #ifdef __GNUC__
 #define WF_SMALL __attribute__((optimize("Os")))
 #else
 #define WF_SMALL
 #endif
 #ifdef __GNUC__
 
 #endif
 '''+re.sub(r'void (exec_opcode(?:_ddfd|_cb|_ed|_dcb)?|z80_step)\(',r'WF_SMALL void \1(',base.replace('#undef GET_BIT',''))+'''
 #ifdef __GNUC__
 
 #endif
 static bool wf_prefixed_fast(z80 *z)
 {
  wf_music_seq_t *v=(wf_music_seq_t *)z->userdata;uint16_t pc=z->pc;
  if(pc>=0xbffcu||z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted)return false;
  unsigned prefix=v->rom[pc];if(prefix!=0xdd&&prefix!=0xfd)return false;
  unsigned op=v->rom[pc+1],length=2;uint16_t *iz=prefix==0xdd?&z->ix:&z->iy;
  bool memory=(op>=0x70&&op<=0x77&&op!=0x76)||op==0x36||op==0x46||op==0x4e||op==0x56||op==0x5e||op==0x66||op==0x6e||op==0x7e;
  if(memory)length=op==0x36?4:3;
  else if(op==0x21)length=4;
  else if(op!=0x23&&op!=0xe1&&op!=0xe5&&op!=0x09)return false;
  z->pc=(uint16_t)(pc+length);z->cyc+=cyc_ddfd[op];z->r=(uint8_t)((z->r&128u)|((z->r+2u)&127u));
  if(memory){
   uint16_t address=(uint16_t)(*iz+(int8_t)v->rom[pc+2]);z->mem_ptr=address;
   if(op==0x36)wb(z,address,v->rom[pc+3]);
   else if(op>=0x70&&op<=0x77){
    uint8_t value=op==0x70?z->b:op==0x71?z->c:op==0x72?z->d:op==0x73?z->e:op==0x74?z->h:op==0x75?z->l:z->a;wb(z,address,value);
   }else{
    uint8_t value=rb(z,address);
    switch(op){case 0x46:z->b=value;break;case 0x4e:z->c=value;break;case 0x56:z->d=value;break;case 0x5e:z->e=value;break;case 0x66:z->h=value;break;case 0x6e:z->l=value;break;default:z->a=value;break;}
   }
  }else if(op==0x21)*iz=(uint16_t)(v->rom[pc+2]|(v->rom[pc+3]<<8));
  else if(op==0x23)++*iz;
  else if(op==0xe1)*iz=popw(z);
  else if(op==0xe5)pushw(z,*iz);
  else addiz(z,iz,get_bc(z));
  return true;
 }
 unsigned wf_z80_native_step(z80 *z,unsigned budget,uint32_t limit)
 {
  unsigned done=0;
  again:
  if(done&&z->pc==0x3a)return done;
  if(done>=budget||(int32_t)(limit-(uint32_t)z->cyc)<=0){if(done)return done;goto generic;}
  if(z->nmi_pending||(z->int_pending&&z->iff1)||z->iff_delay||z->halted)goto generic;
  switch(z->pc){
 '''
 chunks=[]
 for pc,block in sorted(selected.items()):
  n=len(block);worst=sum(timings[rom[a]] for a in block)+8;chunks.append(f'case 0x{pc:04x}:\n if(budget-done<{n}u||(int32_t)(limit-(uint32_t)z->cyc)<{worst})goto generic;\n')
  for at in block:
   op=rom[at];text=cases.get(op)
   if not text:raise ValueError(f'No body {op:x} at{at:x}')
   imm=at+1
   # At most one immediate fetch in each non-prefixed opcode case.
   text=text.replace('nextw(z)',f'(z->pc+=2, (uint16_t)0x{rom[imm]+rom[imm+1]*256:04x}u)')
   text=text.replace('nextb(z)',f'(z->pc+=1, (uint8_t)0x{rom[imm]:02x}u)')
   chunks.append(f' z->pc=0x{at+1:04x};z->cyc+={timings[op]};z->r=(uint8_t)((z->r&128u)|((z->r+1u)&127u));\n do{{{text}}}while(0);\n')
  last=block[-1];op=rom[last]
  finish=op in (0x32,0x22) and rom[last+1]+256*rom[last+2]>=(0xc7ff if op==0x22 else 0xc800)
  dynamic='get_bc(z)' if op==0x02 else ('get_de(z)' if op==0x12 else ('get_hl(z)' if op==0x36 or 0x70<=op<=0x77 else None))
  chunks.append((' process_interrupts(z);' if op==0xfb else '')+f'done+={n}u;'+('return done;' if finish else ('if('+dynamic+'>=0xc800u)return done;goto again;' if dynamic else 'goto again;'))+'\n')
 text=head+''.join(chunks)+'''default:break;
  }
 generic:if(wf_prefixed_fast(z))return done+1u;z80_step(z);return done+1u;
 }
 '''
 text+='\nbool wf_z80_native_compatible'+ '(const uint8_t *p,size_t n){\n if(!p||n<0xc000)return false;\n uint32_t crc=0xffffffffu;for(unsigned i=0;i<0xc000;i++){crc^=p[i];for(unsigned b=0;b<8;b++)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}\n return (crc^0xffffffffu)==0x6270a5c0u;\n}\n'
 out_path.parent.mkdir(parents=True,exist_ok=True);out_path.write_text(text)
 return {'blocks':len(selected),'instructions':sum(map(len,selected.values())),'sha256_lf':hashlib.sha256(text.encode()).hexdigest()}

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo',type=Path,default=Path(__file__).resolve().parents[3]);p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();print(json.dumps(generate(a.repo,a.rom,a.out),indent=2))
