#include "wf_sfx_upload.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *source,*sound_ram;
static uint32_t source_bytes;
static unsigned bulk_reads;

bool wf_native_host_command(wf_native_host_t *h,uint16_t cmd,uint16_t arg,uint16_t *value)
{(void)h;(void)cmd;(void)arg;(void)value;return true;}
static bool read_pack(uint32_t at,void *dst,size_t bytes,void *user)
{
 (void)user;
 assert(at<=source_bytes&&bytes<=source_bytes-at);
 if(bytes==32768u){assert((at&2047u)==0u);assert(((uintptr_t)dst&31u)==0u);bulk_reads++;}
 memcpy(dst,source+at,bytes);return true;
}
static void write_word(uint32_t at,uint16_t value,void *user)
{
 (void)user;assert(at>=0x14000u&&at+2u<=0x14000u+source_bytes);
 sound_ram[at-0x14000u]=(uint8_t)(value>>8);
 sound_ram[at-0x14000u+1u]=(uint8_t)value;
}
static bool own(void *user){(void)user;return true;}
static void release(void *user){(void)user;}
static void barrier(void *user){(void)user;}
static uint32_t now_us(void *user){(void)user;return 0;}
static void idle(void *user){(void)user;}

int main(int argc,char **argv)
{
 assert(argc==2);
 FILE *file=fopen(argv[1],"rb");assert(file);
 assert(!fseek(file,0,SEEK_END));long size=ftell(file);assert(size>32&&size<=0x6c000);
 source_bytes=(uint32_t)size;
 assert(!fseek(file,0,SEEK_SET));
 source=malloc(source_bytes);sound_ram=calloc(source_bytes,1u);
 assert(source&&sound_ram&&fread(source,1,source_bytes,file)==source_bytes);
 fclose(file);
 wf_native_mail_t mail={0};mail.reserved[2]=0x4d46u;
 wf_native_host_t host={0};host.ready=true;host.mail=&mail;
 host.ops.write16=write_word;host.ops.enter_owner=own;host.ops.leave_owner=release;
 host.ops.barrier=barrier;host.ops.now_us=now_us;host.ops.idle=idle;
 uint8_t *raw=malloc(32768u+31u);assert(raw);
 uint8_t *scratch=(uint8_t *)(((uintptr_t)raw+31u)&~(uintptr_t)31u);
 assert(wf_sfx_upload_bulk(&host,source_bytes,read_pack,NULL,scratch,32768u));
 assert(bulk_reads>=12u);
 assert(memcmp(source,sound_ram,source_bytes)==0);
 source[source_bytes-2u]^=1u;
 assert(!wf_sfx_upload_bulk(&host,source_bytes,read_pack,NULL,scratch,32768u));
 assert(sound_ram[0]==0&&sound_ram[1]==0);
 free(raw);free(source);free(sound_ram);
 return 0;
}
