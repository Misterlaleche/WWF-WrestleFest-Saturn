#include "wf_music_elf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t elf[131072],ram[0x24000];
static void copy(uint32_t a,uint16_t v,void*u){(void)u;assert(a>=0x25c5c000u&&a<0x25c80000u&&!(a&1));unsigned off=a-0x25c5c000u;ram[off]=(uint8_t)(v>>8);ram[off+1]=(uint8_t)v;}
int main(int argc,char**argv){
 FILE*f=NULL;assert(argc==2&&fopen_s(&f,argv[1],"rb")==0);size_t n=fread(elf,1,sizeof elf,f);fclose(f);
 wf_music_image_t image;assert(wf_music_elf_image(elf,n,&image));assert(image.prepare>=WF_MUSIC_MODULE_BASE+32&&image.run<0x5c7c000);
 memset(ram,0xa5,sizeof ram);
 assert(wf_music_elf_copy(&image,copy,NULL));assert(!memcmp(ram,image.segments[0].data_be,image.segments[0].file_bytes));
 for(unsigned i=1;i<3;i++)for(unsigned j=0;j<image.segments[i].memory_bytes;j++)assert(!ram[image.segments[i].address-WF_MUSIC_MODULE_BASE+j]);
 uint8_t abi=elf[256+5];elf[256+5]=1;assert(!wf_music_elf_image(elf,n,&image));elf[256+5]=abi;
 uint8_t old=elf[19];elf[19]=4;assert(!wf_music_elf_image(elf,n,&image));elf[19]=old;
 old=elf[60];elf[60]=255;assert(!wf_music_elf_image(elf,n,&image));elf[60]=old;
 assert(!wf_music_elf_image(elf,65536,&image));assert(wf_music_elf_image(elf,n,&image));
 puts("SH2 module ELF: code/private/shared bounds, header, CPU and uncached copy PASS");return 0;
}
