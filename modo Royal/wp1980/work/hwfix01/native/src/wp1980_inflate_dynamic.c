/* Independent WP1980 implementation of RFC1951, sections3.2.2--3.2.7.
 * Format authority: https://www.rfc-editor.org/rfc/rfc1951
 * Fast tables contain reversed canonical codes; longer codes use bounded
 * canonical-range lookup. No third-party decompressor source is included. */
#include "wp1980_inflate_dynamic.h"
#include <string.h>

enum {LITERAL_ROOT=9,DISTANCE_ROOT=6,MAX_BITS=15};
typedef struct {
    const uint8_t *p,*end;
    uint32_t bits;
    unsigned count;
} reader_t;
typedef struct {
    uint16_t count[16],first[16],offset[16];
    uint16_t *fast,*symbols;
    unsigned root,max;
} tree_t;

static const uint8_t reverse_nibble[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
static const uint16_t length_base[29]={3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
static const uint8_t length_extra[29]={0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
static const uint16_t distance_base[30]={1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const uint8_t length_order[19]={16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};

static unsigned reverse16(unsigned value)
{
    return ((unsigned)reverse_nibble[value&15u]<<12)|
           ((unsigned)reverse_nibble[(value>>4)&15u]<<8)|
           ((unsigned)reverse_nibble[(value>>8)&15u]<<4)|
           reverse_nibble[(value>>12)&15u];
}
static inline int take(reader_t *r,unsigned n,unsigned *out)
{
    while(r->count<n){
        if(r->p==r->end)return -1;
        r->bits|=(uint32_t)*r->p++<<r->count;r->count+=8u;
    }
    *out=(unsigned)(r->bits&((1u<<n)-1u));
    r->bits>>=n;r->count-=n;return 0;
}

/* Incomplete code-length trees are invalid. Literal/distance trees may have
 * one length1 code; an empty distance tree is valid until a match uses it. */
static int build_tree(tree_t *t,const uint8_t *lengths,unsigned symbols,
                      unsigned root,unsigned allow_single,unsigned allow_empty)
{
    uint16_t cursor[16],next[16];
    unsigned maximum=0,used=0,code=0,total=0;
    memset(t->count,0,sizeof t->count);
    for(unsigned symbol=0;symbol<symbols;symbol++){
        unsigned length=lengths[symbol];
        if(length>MAX_BITS)return -1;
        if(length){++t->count[length];++used;if(length>maximum)maximum=length;}
    }
    int left=1;
    for(unsigned length=1;length<=MAX_BITS;length++){
        left=(left<<1)-(int)t->count[length];if(left<0)return -1;
    }
    if(!used){if(!allow_empty)return -1;}
    else if(left&&!(allow_single&&used==1u&&maximum==1u))return -1;
    t->root=root;t->max=maximum;
    memset(t->fast,0,(size_t)(1u<<root)*sizeof *t->fast);
    for(unsigned length=1;length<=MAX_BITS;length++){
        code=(code+t->count[length-1u])<<1;
        t->first[length]=(uint16_t)code;
        t->offset[length]=cursor[length]=(uint16_t)total;
        next[length]=(uint16_t)code;total+=t->count[length];
    }
    for(unsigned symbol=0;symbol<symbols;symbol++){
        unsigned length=lengths[symbol];if(!length)continue;
        t->symbols[cursor[length]++]=(uint16_t)symbol;
        code=next[length]++;
        if(length<=root){
            unsigned reversed=reverse16(code)>>(16u-length);
            uint16_t entry=(uint16_t)((length<<9)|symbol);
            for(unsigned index=reversed;index<(1u<<root);index+=1u<<length)
                t->fast[index]=entry;
        }
    }
    return 0;
}

static inline int decode(reader_t *r,const tree_t *t,unsigned *symbol)
{
    while(r->count<t->root&&r->p!=r->end){
        r->bits|=(uint32_t)*r->p++<<r->count;r->count+=8u;
    }
    unsigned entry=t->fast[r->bits&((1u<<t->root)-1u)];
    if(entry){
        unsigned length=entry>>9;
        if(r->count<length)return -1;
        r->bits>>=length;r->count-=length;*symbol=entry&511u;return 0;
    }
    while(r->count<t->max&&r->p!=r->end){
        r->bits|=(uint32_t)*r->p++<<r->count;r->count+=8u;
    }
    unsigned reversed=reverse16(r->bits);
    for(unsigned length=t->root+1u;length<=t->max&&length<=r->count;length++){
        unsigned code=reversed>>(16u-length);
        unsigned index=code-t->first[length];
        if(index<t->count[length]){
            *symbol=t->symbols[t->offset[length]+index];
            r->bits>>=length;r->count-=length;return 0;
        }
    }
    return -1;
}

static void match_copy(uint8_t *out,unsigned distance,unsigned length)
{
    const uint8_t *copy=out-distance;
    if(distance==1u){
        uint8_t value=*copy;uint32_t word=(uint32_t)value*0x01010101u;
        while(length&&((uintptr_t)out&3u)){*out++=value;--length;}
        while(length>=4u){
            __builtin_memcpy(__builtin_assume_aligned(out,4),&word,4);
            out+=4;length-=4u;
        }
        while(length--)*out++=value;
        return;
    }
    if(distance>=4u&&(((uintptr_t)out^(uintptr_t)copy)&3u)==0u){
        while(length&&((uintptr_t)out&3u)){*out++=*copy++;--length;}
        while(length>=4u){
            __builtin_memcpy(__builtin_assume_aligned(out,4),
                             __builtin_assume_aligned(copy,4),4);
            out+=4;copy+=4;length-=4u;
        }
    }
    if(distance>=4u)while(length>=4u){
        out[0]=copy[0];out[1]=copy[1];out[2]=copy[2];out[3]=copy[3];
        out+=4;copy+=4;length-=4u;
    }
    /* Forward copying is required for overlapping distance2/3 sequences. */
    while(length--)*out++=*copy++;
}

int wp1980_inflate_dynamic(const uint8_t *source,size_t source_bytes,
                           uint8_t *output,size_t output_bytes)
{
    if(!source||!output||!source_bytes||source_bytes>65536u||output_bytes>65536u)return -1;
    uintptr_t src_address=(uintptr_t)source,dst_address=(uintptr_t)output;
    if(src_address<=dst_address?dst_address-src_address<source_bytes:
       src_address-dst_address<output_bytes)return -1;
    reader_t r={source,source+source_bytes,0,0};
    uint8_t *out=output,*end=output+output_bytes;
    uint16_t literal_fast[1u<<LITERAL_ROOT],distance_fast[1u<<DISTANCE_ROOT];
    uint16_t literal_symbols[288],distance_symbols[32];
    uint8_t lengths[320];
    tree_t literal={.fast=literal_fast,.symbols=literal_symbols};
    tree_t distance={.fast=distance_fast,.symbols=distance_symbols};
    unsigned final=0;
    do{
        unsigned type;
        if(take(&r,1,&final)||take(&r,2,&type))return -1;
        if(type==0u){
            unsigned skip=r.count&7u,ignored,length,inverse;
            if(take(&r,skip,&ignored)||take(&r,16,&length)||take(&r,16,&inverse))return -1;
            if((length^inverse)!=65535u||length>(size_t)(end-out))return -1;
            while(length--){unsigned byte;if(take(&r,8,&byte))return -1;*out++=(uint8_t)byte;}
            continue;
        }
        if(type==1u){
            for(unsigned i=0;i<288u;i++)lengths[i]=(uint8_t)(i<144u?8u:i<256u?9u:i<280u?7u:8u);
            if(build_tree(&literal,lengths,288,LITERAL_ROOT,0,0))return -1;
            memset(lengths,5,32);
            if(build_tree(&distance,lengths,32,DISTANCE_ROOT,0,0))return -1;
        }else if(type==2u){
            unsigned lit_count,dist_count,code_count;
            if(take(&r,5,&lit_count)||take(&r,5,&dist_count)||take(&r,4,&code_count))return -1;
            lit_count+=257u;dist_count+=1u;code_count+=4u;
            if(lit_count>286u)return -1;
            memset(lengths,0,19);
            for(unsigned i=0;i<code_count;i++){
                unsigned value;if(take(&r,3,&value))return -1;
                lengths[length_order[i]]=(uint8_t)value;
            }
            if(build_tree(&literal,lengths,19,7,0,0))return -1;
            unsigned count=lit_count+dist_count,index=0;
            while(index<count){
                unsigned symbol,extra,repeat,value;
                if(decode(&r,&literal,&symbol))return -1;
                if(symbol<16u){lengths[index++]=(uint8_t)symbol;continue;}
                if(symbol==16u){
                    if(!index||take(&r,2,&extra))return -1;
                    repeat=extra+3u;value=lengths[index-1u];
                }else if(symbol==17u){
                    if(take(&r,3,&extra))return -1;
                    repeat=extra+3u;value=0;
                }else if(symbol==18u){
                    if(take(&r,7,&extra))return -1;
                    repeat=extra+11u;value=0;
                }else return -1;
                if(repeat>count-index)return -1;
                while(repeat--)lengths[index++]=(uint8_t)value;
            }
            if(!lengths[256])return -1;
            if(build_tree(&literal,lengths,lit_count,LITERAL_ROOT,1,0)||
               build_tree(&distance,lengths+lit_count,dist_count,DISTANCE_ROOT,1,1))return -1;
        }else return -1;
        for(;;){
            unsigned symbol;
            if(decode(&r,&literal,&symbol))return -1;
            if(symbol<256u){if(out==end)return -1;*out++=(uint8_t)symbol;continue;}
            if(symbol==256u)break;
            if(symbol>285u)return -1;
            unsigned index=symbol-257u,extra,length,back;
            if(take(&r,length_extra[index],&extra))return -1;
            length=length_base[index]+extra;
            if(decode(&r,&distance,&index)||index>=30u)return -1;
            unsigned bits=index<4u?0u:(index>>1)-1u;
            if(take(&r,bits,&extra))return -1;
            back=distance_base[index]+extra;
            if(back>(size_t)(out-output)||length>(size_t)(end-out))return -1;
            match_copy(out,back,length);out+=length;
        }
    }while(!final);
    return out==end&&r.p==r.end&&r.count<8u?0:-1;
}
