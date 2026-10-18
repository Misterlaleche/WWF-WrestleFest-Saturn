#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r17_contract.h"
static unsigned char rom[0x100000];
static unsigned be(unsigned a,unsigned n){unsigned v=0;while(n--)v=(v<<8)|rom[a++];return v;}
int main(int argc,char**argv){FILE*f;size_t size;wf_runtime_t rt;unsigned character,pose,x,y,mirror,record,table,seed;wf_status_t status;if(argc!=8)return 2;f=fopen(argv[1],"rb");if(!f)return 3;size=fread(rom,1,sizeof rom,f);fclose(f);character=(unsigned)strtoul(argv[2],0,0);pose=(unsigned)strtoul(argv[3],0,0);x=(unsigned)strtoul(argv[4],0,0);y=(unsigned)strtoul(argv[5],0,0);mirror=(unsigned)strtoul(argv[6],0,0);seed=(unsigned)strtoul(argv[7],0,0);
 table=0x38f14+(int16_t)be(0x38f14+2*character,2);record=be(0x38fb8+4*character,4)+be(table+2*pose,2);if(record>=size)return 4;
 memset(&rt,0,sizeof rt);rt.m68k_regs.valid=1;for(unsigned i=0;i<8;i++)rt.m68k_regs.d[i]=seed+i*0x11111111u;wf_set_be16(&rt.actors[0],WF_A_CHARACTER,(uint16_t)character);wf_set_be16(&rt.actors[0],WF_A_MIRROR,(uint16_t)(pose|(mirror?0x8000:0)));wf_set_be16(&rt.actors[0],WF_A_SCREEN_X,(uint16_t)x);wf_set_be16(&rt.actors[0],WF_A_SCREEN_Y,(uint16_t)y);
 status=wf_r17_render_record_shadow(&rt,&rt.actors[0],rom+record,size-record);printf("%u %08x %08x %08x\n",(unsigned)status,rt.m68k_regs.d[6],rt.m68k_regs.d[7],record);return 0;}
