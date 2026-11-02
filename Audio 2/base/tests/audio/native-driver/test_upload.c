#include "wf_sfx_upload.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static wf_native_mail_t mail;
static uint8_t ram[0x80000],*pack;
static uint32_t bytes,clock_us;
static unsigned held,binds,stops,corrupt,fail_at;
static bool take(void *u){(void)u;assert(!held);held=1;return true;}
static void leave(void *u){(void)u;assert(held);held=0;}
static void fence(void *u){(void)u;}
static uint32_t now(void *u){(void)u;return clock_us;}
static void idle(void *u){
 (void)u;++clock_us;assert(held);
 if(mail.request!=mail.ack){
  if(mail.command==WF_NATIVE_SFX_BIND){++binds;assert(!memcmp(ram+0x14000,pack,bytes));}
  else{assert(mail.command==WF_NATIVE_SFX_STOP);++stops;}
  mail.result=0;mail.ack=mail.request;
 }
}
static void write16(uint32_t a,uint16_t v,void *u){
 (void)u;assert(held);assert(a>=0x14000&&a+1<0x80000&&!(a&1));ram[a]=(uint8_t)(v>>8);ram[a+1]=(uint8_t)v;
}
static bool read_pack(uint32_t a,void *dst,size_t n,void *u){
 (void)u;assert(n<=1024&&a<=bytes&&n<=bytes-a);
 if(fail_at&&a>=fail_at)return false;
 memcpy(dst,pack+a,n);if(corrupt&&a>=32)((uint8_t *)dst)[0]^=1;return true;
}
int main(int argc,char **argv){
 FILE *f=NULL;long length;wf_native_host_t h={0};
 assert(argc==2&&fopen_s(&f,argv[1],"rb")==0);fseek(f,0,SEEK_END);length=ftell(f);assert(length>0);rewind(f);
 bytes=(uint32_t)length;pack=malloc(bytes);assert(pack&&fread(pack,1,bytes,f)==bytes);fclose(f);
 h.mail=&mail;h.ready=true;h.timeout_us=100000;
 h.ops.enter_owner=take;h.ops.leave_owner=leave;h.ops.barrier=fence;h.ops.write16=write16;h.ops.idle=idle;h.ops.now_us=now;
 mail.magic_hi=0x5746;mail.magic_lo=0x4e31;mail.abi=WF_NATIVE_ABI;mail.state=WF_NATIVE_READY;mail.reserved[2]=WF_NATIVE_SFX_PROFILE;
 assert(wf_sfx_upload(&h,bytes,read_pack,NULL));assert(binds==1&&stops==1&&!held);
 corrupt=1;assert(!wf_sfx_upload(&h,bytes,read_pack,NULL));assert(binds==1&&!held&&ram[0x14000]==0);
 corrupt=0;fail_at=4096;assert(!wf_sfx_upload(&h,bytes,read_pack,NULL));assert(binds==1&&!held);
 fail_at=0;mail.reserved[2]=0;assert(!wf_sfx_upload(&h,bytes,read_pack,NULL));assert(binds==1);
 free(pack);puts("uploader CRC, I/O failure, owner, profile and publish-last: OK");return 0;
}
