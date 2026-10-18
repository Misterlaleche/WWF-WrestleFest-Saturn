#!/usr/bin/env python3
"""Validated with Capstone 5.0.7. Default is read-only byte-for-byte generated-source audit."""
from capstone import *
import re, pathlib,hashlib,argparse
p=argparse.ArgumentParser(description="Reproduce static native participant C from the SHA1-pinned ROM; no CPU interpreter is linked.");p.add_argument("rom");p.add_argument("--write",action="store_true");args=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1];rom=pathlib.Path(args.rom).read_bytes();assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
c=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
ranges=[(0x7506,0x7548),(0x75ee,0x7680),(0x7680,0x76a2),(0x76aa,0x76e8),(0x76e8,0x76f8),(0x7720,0x7744),(0x776c,0x77d0),(0x77d8,0x7814),(0x782c,0x7874),(0x1079e,0x10830),(0x9132,0x9398),(0x9398,0x94ca),(0x94ca,0x9556),(0x9556,0x956c),(0x956c,0x9578),(0x9578,0x95ae),(0x95ae,0x961c),(0x961c,0x9658),(0x9658,0x96a0),(0x96a0,0x96c6),(0x96c6,0x970c),(0x972c,0x989a)]
public={0x7506,0x75ee,0x7720,0x782c,0x1079e,0x9132}
def fn(a):return 'wf_participant_%x'%a
def reg(s):return 'm->%s[%s]'%(s[0],s[1])
def num(s):return str(int(s.replace('$','0x'),0))
def split(s):return re.split(r', (?![^()]*\))',s)
def addr(s):
 s=s.strip()
 if re.fullmatch(r'\$[\da-f]+\.l',s):return num(s[:-2])+'u'
 if re.fullmatch(r'\(a\d\)\+?',s):return reg(s[1:3])
 mt=re.fullmatch(r'(-?\$[\da-f]+)?\((a\d)(?:, (d\d)\.w)?\)',s)
 if mt:
  off= num(mt[1]) if mt[1] else '0';return '('+reg(mt[2])+' + '+off+ (' + (int16_t)'+reg(mt[3]) if mt[3] else '')+')'
 raise ValueError(('addr',s))
def read(s,w):
 if s.startswith('#'):return num(s[1:])+'u'
 if re.fullmatch('[da][0-7]',s):return reg(s)
 if s.endswith(')+'):return 'postread(m,&%s,%d)'%(reg(s[1:3]),w)
 return 'wf_participant_read(m,%s,%d)'%(addr(s),w)
def write(s,w,v):
 if re.fullmatch('d[0-7]',s):return 'put(&%s,%s,%d);'%(reg(s),v,w)
 if re.fullmatch('a[0-6]',s):return '%s=%s;'%(reg(s),v)
 if s.endswith(')+'):return 'postwrite(m,&%s,%s,%d);'%(reg(s[1:3]),v,w)
 return 'wf_participant_write(m,%s,%s,%d);'%(addr(s),v,w)
def regs(s):
 r=[]
 for part in s.split('/'):
  if '-' in part:
   p,q=part.split('-');r.extend(p[0]+str(k) for k in range(int(p[1]),int(q[1])+1))
  else:r.append(part)
 return r
outs=[]
for start,end in ranges:
 targets={int(split(j.op_str)[-1][1:],16) for j in c.disasm(rom[start:end],start) if j.mnemonic.split('.')[0] in ('bra','beq','bne','bcs','bcc','bpl','dbra')}
 lines=['%svoid %s(wf_participant_machine_t *m){'%('' if start in public else 'static ',fn(start)),' uint32_t stack[32];unsigned sp=0; (void)stack;(void)sp;']
 for i in c.disasm(rom[start:end],start):
  mn=i.mnemonic.split('.')[0];sz=i.mnemonic.split('.')[-1];w={'b':1,'w':2,'l':4}.get(sz,4);ops=split(i.op_str)
  x=''
  if mn=='movem':
   if ops[1]=='-(a7)':x=' '.join('stack[sp++]=%s;'%reg(r) for r in regs(ops[0]))
   else:x=' '.join('%s=stack[--sp];'%reg(r) for r in reversed(regs(ops[1])))
  elif mn=='rts':x='return;'
  elif mn=='lea':x=write(ops[1],4,addr(ops[0]))
  elif mn=='movea':x=write(ops[1],4,read(ops[0],w))
  elif mn in ('move','moveq'):
   if ops[1]=='ccr':v=int(ops[0][2:],16);x='m->z=%d;m->n=%d;m->c=%d;' % (bool(v&4),bool(v&8),bool(v&1))
   else:
    val=read(ops[0],w)
    if mn=='moveq':w=4;val='(uint32_t)(int32_t)(int8_t)('+val+')'
    x='{uint32_t v=%s; %s flags(m,v,%d);}'%(val,write(ops[1],w,'v'),w)
  elif mn in ('adda','suba'):x='%s %s= %s;'%(reg(ops[1]),'+' if mn=='adda' else '-',read(ops[0],w))
  elif mn in ('add','addi','addq','sub','subi','subq','and','andi','or','ori','eor'):
   op='+' if mn.startswith('add') else '-' if mn.startswith('sub') else '&' if mn.startswith('and') else '|' if mn.startswith('or') else '^'
   x='{uint32_t s=%s,v=%s,r=(v %s s)&mask(%d); %s flags(m,r,%d); %s}'%(read(ops[0],w),read(ops[1],w),op,w,write(ops[1],w,'r'),w,'m->c=(v&mask(%d))<(s&mask(%d));'%(w,w) if op=='-' else 'm->c=((uint64_t)(v&mask(%d))+(s&mask(%d)))>mask(%d);'%(w,w,w) if op=='+' else '')
  elif mn in ('cmp','cmpi','cmpa'):
   x='{uint32_t s=%s&mask(%d),v=%s&mask(%d);flags(m,v-s,%d);m->c=v<s;}'%(read(ops[0],w),w,read(ops[1],w),w,w)
  elif mn in ('btst','bset','bclr'):
   bw=4 if re.fullmatch('d[0-7]',ops[1]) else 1
   x='{uint32_t v=%s,b=1u<<(%s%%%d); m->z=(v&b)==0; %s}'%(read(ops[1],bw),read(ops[0],4),bw*8,write(ops[1],bw,'v|b' if mn=='bset' else 'v&~b') if mn!='btst' else '')
  elif mn in ('clr','tst','not'):
   val='0' if mn=='clr' else read(ops[0],w)
   if mn=='not':val='~('+val+')'
   x='{uint32_t v=%s; %s flags(m,v,%d);}'%(val,write(ops[0],w,'v') if mn!='tst' else '',w)
  elif mn in ('asl','lsl','lsr','asr'):
   sh=read(ops[0],4);val=read(ops[1],w)
   expr='v<<s' if mn in ('asl','lsl') else '(uint32_t)((int16_t)v>>s)' if mn=='asr' and w==2 else 'v>>s'
   x='{unsigned s=%s;uint32_t v=%s&mask(%d),r=%s; %s flags(m,r,%d);m->c=s?((v>>(%s))&1):0;}'%(sh,val,w,expr,write(ops[1],w,'r'),w,str(w*8)+'-s' if mn in ('asl','lsl') else 's-1')
  elif mn=='mulu':x='{uint32_t v=(%s&65535u)*(%s&65535u);%s flags(m,v,4);}'%(read(ops[0],2),read(ops[1],2),write(ops[1],4,'v'))
  elif mn=='divu':x='{uint32_t s=%s&65535u,v=%s;%s flags(m,v/s,2);}'%(read(ops[0],2),read(ops[1],4),write(ops[1],4,'((v%s)<<16)|(v/s)'))
  elif mn=='swap':x='{uint32_t v=%s;v=(v<<16)|(v>>16);%s flags(m,v,4);}'%(read(ops[0],4),write(ops[0],4,'v'))
  elif mn in ('bsr','jsr'):x=fn(int(ops[0].replace('$','').replace('.l',''),16))+'(m);'
  elif mn in ('bra','beq','bne','bcs','bcc','bpl'):
   dest=int(ops[0][1:],16);cond={'bra':'1','beq':'m->z','bne':'!m->z','bcs':'m->c','bcc':'!m->c','bpl':'!m->n'}[mn];x='if(%s) goto L%x;'%(cond,dest)
  elif mn=='dbra':x='put(&%s,%s-1,2);if((uint16_t)%s!=65535u)goto L%s;'%(reg(ops[0]),reg(ops[0]),reg(ops[0]),ops[1][1:])
  elif mn=='nop':x=';'
  else:raise ValueError((hex(i.address),mn,i.op_str))
  lines.append('%s/* $%x: %s %s */\n if(m->status!=WF_OK)return;\n %s'%('L%x: '%i.address if i.address in targets else '',i.address,i.mnemonic,i.op_str,x))
 lines.append('}')
 outs.append('\n'.join(lines))
pre='''/* Native static translation of canonical ROM SHA1 a726c46f350723ea73158bc78055d17d418b14e2.
 * Each C label maps to its ROM instruction; there is no opcode decoder or dynamic CPU dispatch.
 * Bus access is explicit raw actor/global state and staged native FG0 transport. */
#include "wf_r17_participant_helpers.h"
static uint32_t mask(unsigned w){return w==4?0xffffffffu:w==2?65535u:255u;}
static void put(uint32_t *p,uint32_t v,unsigned w){*p=(*p&~mask(w))|(v&mask(w));}
static void flags(wf_participant_machine_t*m,uint32_t v,unsigned w){v&=mask(w);m->z=v==0;m->n=(v>>(w*8-1))&1;m->c=0;}
static void postwrite(wf_participant_machine_t*m,uint32_t*a,uint32_t v,unsigned w){wf_participant_write(m,*a,v,w);*a+=w;}
'''
pre+='\n'.join('%svoid %s(wf_participant_machine_t *);'%('' if a in public else 'static ',fn(a)) for a,b in ranges)+'\n'
files={'src/wf_r17_participant_native.c':pre+'\n\n'.join(outs)+'\n'}
tables=[(0x76a2,0x76aa),(0x77d0,0x77d8),(0x7814,0x782c),(0x970c,0x972c),(0x989a,0x98ba),(0x10830,0x10864)]
s='/* Direct canonical main68k.bin data; numeric addresses are ROM authority. */\n'
for a,e in tables:s+='static const uint8_t table_%x[]={%s};\n'%(a,','.join('0x%02x'%x for x in rom[a:e]))
s+='static const struct {uint32_t address;size_t length;const uint8_t*data;} rom_tables[]={\n'
for a,e in tables:s+='{0x%x,%d,table_%x},\n'%(a,e-a,a)
s+='};\n';files['src/wf_r17_participant_tables.inc']=s
for path,content in files.items():
 if args.write:(root/path).write_text(content)
 else:assert (root/path).read_text()==content, 'generated source differs: '+path
for a,e in ranges+tables:print('$%06X..$%06X SHA256 %s'%(a,e,hashlib.sha256(rom[a:e]).hexdigest()))
print('PASS',len(outs),'native functions and all six ROM table spans match canonical generation')
