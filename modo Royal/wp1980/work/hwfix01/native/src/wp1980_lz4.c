/* WP1980 independent decoder of the documented LZ4 block format.
 * Format reference: github.com/lz4/lz4/doc/lz4_Block_format.md.
 * The encoder runs on the host; Saturn only performs bounded byte/word copies. */
#include "wp1980_lz4.h"

static inline void literal_copy(uint8_t *dst,const uint8_t *src,size_t n){
    if((((uintptr_t)dst^(uintptr_t)src)&3u)==0u){
        while(n&&((uintptr_t)dst&3u)){*dst++=*src++;--n;}
        while(n>=4u){
            __builtin_memcpy(__builtin_assume_aligned(dst,4),__builtin_assume_aligned(src,4),4);
            dst+=4;src+=4;n-=4;
        }
    }
    while(n>=4u){
        dst[0]=src[0];dst[1]=src[1];dst[2]=src[2];dst[3]=src[3];
        dst+=4;src+=4;n-=4;
    }
    while(n--)*dst++=*src++;
}
static inline void match_copy(uint8_t *dst,unsigned distance,size_t n){
    const uint8_t *src=dst-distance;
    if(distance==1u){
        uint8_t byte=*src;uint32_t word=(uint32_t)byte*0x01010101u;
        while(n&&((uintptr_t)dst&3u)){*dst++=byte;--n;}
        while(n>=4u){__builtin_memcpy(__builtin_assume_aligned(dst,4),&word,4);dst+=4;n-=4;}
        while(n--)*dst++=byte;
        return;
    }
    if(distance>=4u&&(((uintptr_t)dst^(uintptr_t)src)&3u)==0u){
        while(n&&((uintptr_t)dst&3u)){*dst++=*src++;--n;}
        while(n>=4u){
            __builtin_memcpy(__builtin_assume_aligned(dst,4),__builtin_assume_aligned(src,4),4);
            dst+=4;src+=4;n-=4;
        }
    }
    /* Forward bytes intentionally permit distance2/3 and longer overlap. */
    while(n--)*dst++=*src++;
}
int wp1980_lz4_decode(const uint8_t *source,size_t source_bytes,
                      uint8_t *output,size_t output_bytes){
    if(!source||!output||!source_bytes||source_bytes>65536u||output_bytes>65536u)return -1;
    const uint8_t *p=source,*end=source+source_bytes;
    uint8_t *out=output,*limit=output+output_bytes;
    while(p<end){
        unsigned token=*p++;size_t literals=token>>4;
        if(literals==15u){
            unsigned extra;
            do{if(p==end||literals>(size_t)(limit-out))return -1;extra=*p++;literals+=extra;}while(extra==255u);
        }
        if(literals>(size_t)(end-p)||literals>(size_t)(limit-out))return -1;
        literal_copy(out,p,literals);p+=literals;out+=literals;
        if(p==end)return out==limit?0:-1;
        if(end-p<2)return -1;
        unsigned distance=(unsigned)p[0]|(unsigned)p[1]<<8;p+=2;
        if(!distance||distance>(size_t)(out-output))return -1;
        size_t length=(token&15u)+4u;
        if((token&15u)==15u){
            unsigned extra;
            do{if(p==end||length>(size_t)(limit-out))return -1;extra=*p++;length+=extra;}while(extra==255u);
        }
        if(length>(size_t)(limit-out))return -1;
        match_copy(out,distance,length);out+=length;
    }
    return -1; /* The final sequence must end after literals, never a match. */
}
