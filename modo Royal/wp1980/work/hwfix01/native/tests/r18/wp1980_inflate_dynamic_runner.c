/* Sanitized batch driver: independent zlib/hand-built test vectors arrive
 * from the Python encoder. Caller buffers have input boundaries and canaries. */
#include "wp1980_inflate_dynamic.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned read32(FILE *file)
{
    unsigned char bytes[4];assert(fread(bytes,1,4,file)==4);
    return (unsigned)bytes[0]<<24|(unsigned)bytes[1]<<16|(unsigned)bytes[2]<<8|bytes[3];
}
int main(int argc,char **argv)
{
    if(argc!=2)return 2;
    FILE *file=fopen(argv[1],"rb");assert(file);
    unsigned cases=read32(file),checks=0;
    for(unsigned index=0;index<cases;index++){
        unsigned packed_size=read32(file),output_size=read32(file),valid=read32(file);
        assert(packed_size<=131072u&&output_size<=65536u);
        unsigned char *packed=malloc(packed_size?packed_size:1),*expected=malloc(output_size?output_size:1);
        assert(packed&&expected);
        assert(fread(packed,1,packed_size,file)==packed_size);
        if(valid)assert(fread(expected,1,output_size,file)==output_size);
        for(unsigned alignment=0;alignment<4;alignment++){
            size_t allocation=(size_t)packed_size+alignment;
            unsigned char *source=malloc(allocation?allocation:1u);
            unsigned char *dest=malloc((size_t)output_size+36u);
            assert(source&&dest);memcpy(source+alignment,packed,packed_size);
            memset(dest,0xa5,(size_t)output_size+36u);
            unsigned char *output=dest+16u+((alignment+1u)&3u);
            int status=wp1980_inflate_dynamic(source+alignment,packed_size,output,output_size);
            if((status==0)!=valid||(valid&&memcmp(output,expected,output_size))){
                fprintf(stderr,"case%u align%u in%u out%u valid%u status%d\n",index,alignment,packed_size,output_size,valid,status);
                return 1;
            }
            for(unsigned char *p=dest;p<output;p++)assert(*p==0xa5);
            for(unsigned char *p=output+output_size;p<dest+output_size+36u;p++)assert(*p==0xa5);
            ++checks;free(source);free(dest);
        }
        free(packed);free(expected);
    }
    assert(fgetc(file)==EOF);fclose(file);
    unsigned char one[8]={3,0};
    assert(wp1980_inflate_dynamic(NULL,2,one,0)!=0);
    assert(wp1980_inflate_dynamic(one,2,NULL,0)!=0);
    assert(wp1980_inflate_dynamic(one,65537u,one+4,0)!=0);
    assert(wp1980_inflate_dynamic(one,2,one+4,65537u)!=0);
    assert(wp1980_inflate_dynamic(one,2,one,0)!=0);
    printf("PASS: %u vectors, %u aligned exact/rejection/canary checks\n",cases,checks+5u);
    return 0;
}
