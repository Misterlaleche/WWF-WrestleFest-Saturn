/* Exact opcode admission/state tests against untouched original scalar Z80.
 * Host semantic test only; native performance is measured separately. */
#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned wf_z80_scalar_step(z80 *,unsigned,uint32_t);
extern void wf_ref_z80_init(z80 *);
extern void wf_ref_z80_step(z80 *);
static uint8_t rom[65536];
static wf_music_seq_t left,right;
static uint64_t cases;
static uint32_t io_left,io_right;
static uint8_t read_byte(void *user,uint16_t a)
{
 wf_music_seq_t *v=user;
 if(a<0xc000)return rom[a];
 if(a<0xc800)return v->ram[a-0xc000];
 uint32_t *p=v==&left?&io_left:&io_right;*p=(*p^a)*16777619u;
 return (uint8_t)(a^(a>>8)^0xa5);
}
static void write_byte(void *user,uint16_t a,uint8_t value)
{(void)user;(void)a;(void)value;assert(!"unexpected write");}
static void normalize(z80 *z)
{z->read_byte=0;z->write_byte=0;z->port_in=0;z->port_out=0;z->userdata=0;}
static void one(unsigned opcode,unsigned a,unsigned value,unsigned flags,uint16_t pc,uint16_t hl)
{
 z80 *z=&left.cpu;memset(z,0,sizeof *z);wf_ref_z80_init(z);
 left.rom=rom;z->read_byte=read_byte;z->write_byte=write_byte;z->userdata=&left;
 z->pc=pc;z->sp=0xc7fe;z->cyc=0x123400;z->mem_ptr=0x5678;z->r=(uint8_t)value;
 z->a=a;z->b=value;z->c=value^0x59;z->d=value;z->e=value^0xc3;z->h=hl>>8;z->l=hl;
 z->sf=(flags>>7)&1;z->zf=(flags>>6)&1;z->yf=(flags>>5)&1;z->hf=(flags>>4)&1;
 z->xf=(flags>>3)&1;z->pf=(flags>>2)&1;z->nf=(flags>>1)&1;z->cf=flags&1;
 rom[pc]=opcode;
 uint16_t imm=(uint16_t)(pc+1);
 if(imm<0xc000)rom[imm]=value;else if(imm<0xc800)left.ram[imm-0xc000]=value;
 uint16_t imm2=(uint16_t)(pc+2);
 if(imm2<0xc000)rom[imm2]=value^0x78;else if(imm2<0xc800)left.ram[imm2-0xc000]=value^0x78;
 right=left;right.cpu.userdata=&right;io_left=io_right=2166136261u;
 wf_ref_z80_step(&left.cpu);
 unsigned took=wf_z80_scalar_step(&right.cpu,1,0x200000);
 if(took!=1){fprintf(stderr,"RED opcode%02x was not admitted, count=%u\n",opcode,took);exit(2);}
 z80 l=left.cpu,r=right.cpu;normalize(&l);normalize(&r);
 if(memcmp(&l,&r,sizeof l)||memcmp(left.ram,right.ram,sizeof left.ram)||io_left!=io_right){
  fprintf(stderr,"FAIL op=%02x a=%02x operand=%02x flags=%02x pc=%04x hl=%04x io=%08x/%08x\n",opcode,a,value,flags,pc,hl,io_left,io_right);exit(3);
 }
 ++cases;
}
int main(void)
{
 static const unsigned ops[]={0x01,0x04,0x0e,0x38,0x56,0x82,0x90,0xba,0xc6};
 for(unsigned i=0;i<sizeof rom;i++)rom[i]=(uint8_t)(i^(i>>8));
 for(unsigned i=0;i<sizeof left.ram;i++)left.ram[i]=(uint8_t)(i*17+13);
 for(unsigned o=0;o<sizeof ops/sizeof ops[0];o++){
  for(unsigned a=0;a<256;a++)for(unsigned v=0;v<256;v++){
   one(ops[o],a,v,(a^v),0x1000,0xc000u+((a+v)&0x7ff));
   one(ops[o],a,v,(a^v)^255u,0xbfff,0xbfffu+((a+v)%0x402));
  }
  printf("PASS original opcode%02x CPU/flags/cycles/R/RAM/I-O cases=%"PRIu64"\n",ops[o],cases);
 }
 for(unsigned address=0;address<65536;address++)one(0x56,0x5a,address,0xa5,0x1000,(uint16_t)address);
 for(unsigned kind=0;kind<6;kind++){
  memset(&right,0,sizeof right);right.rom=rom;right.cpu.userdata=&right;right.cpu.pc=0x1000;
  right.cpu.cyc=10;rom[0x1000]=0x82;
  unsigned budget=1,limit=20;
  if(kind==0)budget=0;if(kind==1)limit=10;if(kind==2)right.cpu.nmi_pending=1;
  if(kind==3){right.cpu.int_pending=1;right.cpu.iff1=1;}
  if(kind==4)right.cpu.iff_delay=1;if(kind==5)right.cpu.halted=1;
  left=right;assert(wf_z80_scalar_step(&right.cpu,budget,limit)==0);assert(!memcmp(&left,&right,sizeof left));++cases;
 }
 printf("PASS exact new scalar admissions total=%"PRIu64"\n",cases);return 0;
}
