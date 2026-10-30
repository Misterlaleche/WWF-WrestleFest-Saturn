/* Execute immutable original68000 bytes from1AFC to1B1E. No routine is
 * stubbed. Alternating input-port VBlank levels are the only board timing
 * fixture; it observes the original high/low wait loops directly. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m68k.h"
#define INIT_BYTES 106664u
#define INIT_RAM 128u
#define INIT_REGS (128u+16384u)
#define INIT_TEXT (INIT_REGS+32u)
#define INIT_SPRITES (INIT_TEXT+8192u)
#define INIT_FGMAP (INIT_SPRITES+8192u)
#define INIT_BGMAP (INIT_FGMAP+4096u)
#define INIT_PALETTE (INIT_BGMAP+4096u)
static uint8_t rom[0x80000],ram[0x4000],devices[0x1c0000],initial[INIT_BYTES];
static unsigned steps,unmapped_reads,unmapped_writes,rom_writes,vblank_reads;
static int device(unsigned a)
{
    return (a>=0x80000u&&a<0x84000u)||(a>=0xc0000u&&a<0xc4000u)||
        (a>=0x140000u&&a<0x140028u)||(a>=0x180000u&&a<0x190000u);
}
unsigned int m68k_read_memory_8(unsigned int a)
{
    a&=0xffffffu;
    if(a<sizeof rom)return rom[a];
    if(a>=0x1c0000u&&a<0x1c4000u)return ram[a-0x1c0000u];
    if(a==0x140026u)return (++vblank_reads&1u)?4u:0u;
    if(device(a))return devices[a];
    ++unmapped_reads;return 0xffu;
}
unsigned int m68k_read_memory_16(unsigned int a){return (m68k_read_memory_8(a)<<8)|m68k_read_memory_8(a+1u);}
unsigned int m68k_read_memory_32(unsigned int a){return (m68k_read_memory_16(a)<<16)|m68k_read_memory_16(a+2u);}
void m68k_write_memory_8(unsigned int a,unsigned int v)
{
    a&=0xffffffu;
    if(a<sizeof rom){++rom_writes;return;}
    if(a>=0x1c0000u&&a<0x1c4000u){ram[a-0x1c0000u]=(uint8_t)v;return;}
    if(device(a)){devices[a]=(uint8_t)v;return;}
    ++unmapped_writes;
}
void m68k_write_memory_16(unsigned int a,unsigned int v){m68k_write_memory_8(a,v>>8);m68k_write_memory_8(a+1u,v);}
void m68k_write_memory_32(unsigned int a,unsigned int v){m68k_write_memory_16(a,v>>16);m68k_write_memory_16(a+2u,v);}
static uint32_t be32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static int load(const char *path,void *out,size_t bytes)
{
    FILE *f=fopen(path,"rb");if(!f)return 0;
    int ok=fread(out,1,bytes,f)==bytes&&fgetc(f)==EOF;fclose(f);return ok;
}
static void hook(void){++steps;}
static void put32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int output(const char *path)
{
    FILE *f=fopen(path,"wb");if(!f)return 0;
    static const unsigned spans[][2]={{0xc0000u,0x4000u},{0x80000u,0x4000u},{0x180000u,0x4000u},{0x186000u,0x800u},{0x188000u,0x800u}};
    int ok=1;
    for(unsigned i=0;i<5u;++i)if(fwrite(devices+spans[i][0],1,spans[i][1],f)!=spans[i][1])ok=0;
    for(unsigned i=0;i<8u;++i){
        uint8_t b[4];put32(b,m68k_get_reg(NULL,(m68k_register_t)(M68K_REG_D0+i)));
        if(fwrite(b,1,sizeof b,f)!=sizeof b)ok=0;
    }
    if(fwrite(ram+0x5b0u,1,9u*0x10cu,f)!=9u*0x10cu||
       fwrite(ram+0x5cu,1,4u,f)!=4u||fwrite(ram+0x80u,1,4u,f)!=4u||
       fwrite(ram+0x15fcu,1,2u,f)!=2u)ok=0;
    fclose(f);return ok;
}
int main(int argc,char **argv)
{
    if(argc!=5||!load(argv[1],rom,sizeof rom)||!load(argv[2],initial,sizeof initial))return 2;
    memcpy(ram,initial+INIT_RAM,sizeof ram);
    if(!load(argv[3],ram+0x5b0u,9u*0x10cu))return 2;
    put32(ram+0x80u,8474u);put32(ram+0x5cu,0xb8a86c74u);
    ram[0x16c4u]=0;ram[0x16c5u]=3;
    memcpy(devices+0xc0000u,initial+INIT_TEXT,8192u);
    memcpy(devices+0xc2000u,initial+INIT_SPRITES,8192u);
    memcpy(devices+0x80000u,initial+INIT_FGMAP,4096u);
    memcpy(devices+0x82000u,initial+INIT_BGMAP,4096u);
    memcpy(devices+0x180000u,initial+INIT_PALETTE,0x4000u);
    memcpy(devices+0x186000u,initial+INIT_PALETTE+0x6000u,0x800u);
    memcpy(devices+0x188000u,initial+INIT_PALETTE+0x8000u,0x800u);
    m68k_init();m68k_set_cpu_type(M68K_CPU_TYPE_68000);m68k_pulse_reset();
    m68k_set_reg(M68K_REG_SR,0x2700u);m68k_set_reg(M68K_REG_SP,0x1c3ff0u);
    for(unsigned i=0;i<8u;++i)m68k_set_reg((m68k_register_t)(M68K_REG_D0+i),be32(initial+INIT_REGS+i*4u));
    m68k_set_reg(M68K_REG_PC,0x1afcu);m68k_set_instr_hook_callback(hook);
    unsigned loops=0;
    while(m68k_get_reg(NULL,M68K_REG_PC)!=0x1b1eu&&steps<100000u&&loops++<1000000u)m68k_execute(1);
    unsigned pc=m68k_get_reg(NULL,M68K_REG_PC);
    int ok=pc==0x1b1eu&&!unmapped_reads&&!unmapped_writes&&!rom_writes&&vblank_reads==196u&&output(argv[4]);
    printf("{\"pass\":%s,\"stop_pc\":%u,\"instructions\":%u,\"vblank_port_reads\":%u,\"unmapped_reads\":%u,\"unmapped_writes\":%u,\"rom_writes\":%u}\n",
        ok?"true":"false",pc,steps,vblank_reads,unmapped_reads,unmapped_writes,rom_writes);
    return ok?0:1;
}
