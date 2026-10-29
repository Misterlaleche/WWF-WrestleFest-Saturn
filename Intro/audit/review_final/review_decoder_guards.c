#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool old_decode(const uint8_t *,size_t,int16_t *,size_t,unsigned *);
bool wp0_rice_decode_block(const uint8_t *,size_t,int16_t *,size_t,unsigned *);

static unsigned be16(const uint8_t *p)
{ return (unsigned)p[0]*256u+p[1]; }

int main(int argc,char **argv)
{
    assert(argc==2);
    FILE *f=fopen(argv[1],"rb");assert(f);
    unsigned cases=0,accepted=0,rejected=0,expected_pcm=0;
    for(;;){
        uint8_t header[8];size_t got=fread(header,1,sizeof header,f);
        if(!got){assert(feof(f));break;}
        assert(got==sizeof header);
        size_t bytes=((size_t)be16(header)<<16)|be16(header+2);
        unsigned capacity=be16(header+4),expected=be16(header+6);
        assert(bytes<65544u&&capacity<=1024u);
        uint8_t *data=malloc(bytes?bytes:1u);assert(data);
        assert(fread(data,1,bytes,f)==bytes);
        size_t output_bytes=(capacity+2u)*sizeof(int16_t);
        int16_t *old_out=malloc(output_bytes),*new_out=malloc(output_bytes);
        assert(old_out&&new_out);
        for(unsigned i=0;i<capacity+2u;i++)old_out[i]=new_out[i]=0x5555;
        old_out[0]=new_out[0]=12345;
        old_out[capacity+1u]=new_out[capacity+1u]=23456;
        unsigned old_n=0xC0FFEE11u,new_n=0xC0FFEE11u;
        bool old_ok=old_decode(data,bytes,old_out+1,capacity,&old_n);
        bool new_ok=wp0_rice_decode_block(data,bytes,new_out+1,capacity,&new_n);
        if(old_ok!=new_ok||old_n!=new_n||memcmp(old_out,new_out,output_bytes)){
            fprintf(stderr,"Differential mismatch in case %u, input %zu bytes\n",cases,bytes);
            return 2;
        }
        assert(new_out[0]==12345&&new_out[capacity+1u]==23456);
        assert(!new_ok||new_n<=capacity);
        if(!new_ok)assert(new_n==0xC0FFEE11u);
        if(expected==0)assert(!new_ok);
        else if(expected!=0xFFFFu){
            assert(new_ok&&new_n==expected);
            for(unsigned i=0;i<expected;i++){
                uint8_t pair[2];assert(fread(pair,1,2,f)==2);
                assert(new_out[i+1u]==(int16_t)be16(pair));
            }
            expected_pcm++;
        }
        cases++;accepted+=new_ok;rejected+=!new_ok;
        free(data);free(old_out);free(new_out);
    }
    assert(fclose(f)==0);
    uint8_t bytes[8]={0};int16_t out=123;unsigned n=456;
    assert(!wp0_rice_decode_block(NULL,8,&out,1,&n));
    assert(!wp0_rice_decode_block(bytes,8,NULL,1,&n));
    assert(!wp0_rice_decode_block(bytes,8,&out,1,NULL));
    assert(!wp0_rice_decode_block(bytes,SIZE_MAX,&out,1,&n));
    assert(out==123&&n==456);
    printf("{\"pass\":true,\"cases\":%u,\"accepted\":%u,\"rejected\":%u,\"independent_pcm_cases\":%u,\"null_and_size_max_cases\":4,\"full_partial_output_comparison\":true}\n",cases,accepted,rejected,expected_pcm);
    return 0;
}
