#include "wf_native_elf.h"
#include <assert.h>
#include <stdio.h>
static uint8_t elf[65536];
int main(int argc,char **argv)
{
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb");assert(f);
    size_t n=fread(elf,1,sizeof elf,f);assert(!ferror(f));assert(!fclose(f));
    wf_native_segment_t s[3];wf_native_image_t image;
    assert(wf_native_elf_image(elf,n,s,&image));
    assert(image.count==3 && s[0].address==0 && (s[2].address==0x70000||s[2].address==0x10000));
    if(s[2].address==0x10000){
        uint32_t a=s[1].address;s[1].address=0x5000;
        assert(!wf_native_image_valid(&image));s[1].address=a;
        uint32_t mem=s[0].memory_bytes;s[0].memory_bytes=0x3002;
        assert(!wf_native_image_valid(&image));s[0].memory_bytes=mem;
    }
    assert(!wf_native_elf_image(elf,51,s,&image));
    uint8_t old=elf[28];elf[28]=255;assert(!wf_native_elf_image(elf,n,s,&image));elf[28]=old;
    old=elf[19];elf[19]=42;assert(!wf_native_elf_image(elf,n,s,&image));elf[19]=old;
    assert(!wf_native_elf_image(elf,8192,s,&image));
    assert(wf_native_elf_image(elf,n,s,&image));
    puts("ELF loader: bounds, CPU, vectors and reserved RAM PASS");
    return 0;
}
