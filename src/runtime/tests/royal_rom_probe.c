#include "wf_r17_royal_9132.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned char initial[0x6040],out[0x6040];
static wf_runtime_t rt;
static FILE*trace_file;
static uint16_t be16(unsigned o){return ((unsigned)initial[o]<<8)|initial[o+1];}
static wf_status_t input(wf_runtime_t*r,uint32_t a,uint16_t*v,void*u){(void)r;(void)u;*v=be16(0x6000+a-0x140020);return WF_OK;}
static wf_status_t batch(wf_runtime_t*r,const wf_r17_fg0_write_t*w,size_t n,void*u){(void)r;(void)u;for(size_t j=0;j<n;j++){if(trace_file)fprintf(trace_file,"%x %x %u\n",w[j].addr,w[j].value,w[j].width);for(unsigned i=0;i<w[j].width;i++)out[0x2000+w[j].addr-0xc0000+i]=(uint8_t)(w[j].value>>(8*(w[j].width-1-i)));}return WF_OK;}
int main(int argc,char**argv){
 wf_participant_machine_t*m=calloc(1,sizeof(*m));wf_r17_participant_io_t io={0};FILE*f;wf_status_t status;
 unsigned ranges[][2]={{0x5b0,0xf1c},{0x1576,0x15d2},{0x16a8,0x16c0},{0x66,0x68},{0x7c,0x7e},{0x160,0x164},{0x167b,0x167c},{0x169e,0x16a0}};
 if(argc!=3&&argc!=4)return 2;
 if(argc==4)trace_file=fopen(argv[3],"w");f=fopen(argv[1],"rb");if(!f)return 2;if(fread(initial,1,sizeof initial,f)!=sizeof initial)return 2;fclose(f);memcpy(out,initial,sizeof out);
 wf_participant_init(m,&rt,input,NULL);for(unsigned k=0;k<sizeof(ranges)/sizeof(ranges[0]);k++)for(unsigned a=ranges[k][0];a<ranges[k][1];a++)wf_participant_write(m,0x1c0000+a,initial[a],1);
 if(m->status!=WF_OK){fprintf(stderr,"init failed %x\n",m->failed_address);return 3;}
 rt.m68k_regs.valid=1;for(unsigned i=0;i<8;i++)rt.m68k_regs.d[i]=((uint32_t)be16(0x6008+4*i)<<16)|be16(0x600a+4*i);
 io.input_word=input;io.fg0_batch=batch;status=wf_r17_owner_9132(&rt,NULL,0,&io);if(status!=WF_OK){fprintf(stderr,"owner status%d addr%x\n",status,io.blocked_dependency_rom);return 4;}
 for(unsigned k=0;k<sizeof(ranges)/sizeof(ranges[0]);k++)for(unsigned a=ranges[k][0];a<ranges[k][1];a++)out[a]=(uint8_t)wf_participant_read(m,0x1c0000+a,1);
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)out[0x6008+4*i+j]=(uint8_t)(rt.m68k_regs.d[i]>>(24-j*8));
 f=fopen(argv[2],"wb");if(!f)return 2;fwrite(out,1,sizeof out,f);fclose(f);if(trace_file)fclose(trace_file);free(m);return 0;
}
