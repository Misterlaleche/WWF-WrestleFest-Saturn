#include "wp0_rice.h"
#include <string.h>

static uint16_t be16(const uint8_t *p)
{ return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static uint32_t be32(const uint8_t *p)
{ return ((uint32_t)be16(p)<<16)|be16(p+2); }
/* The SH-2 has no general register-count shift instruction. Keep unary
 * decoding at the MSB so every unary step is a constant one-bit shift.
 * Specialize the eight k values used by the unchanged title stream once
 * per block; generic parameters and both predictors remain supported. */
static inline __attribute__((always_inline)) bool decode_residuals(
    const uint8_t *p,const uint8_t *end,int16_t *out,unsigned n,
    unsigned order,unsigned k)
{
    uint32_t bits=0;
    unsigned available=0;
    const uint32_t maximum=order==1?131070u:262140u;
    const uint32_t max_q=maximum>>k;
    int32_t previous=out[order-1];
    int32_t older=order==2?out[0]:0;
    for(unsigned i=order;i<n;++i){
        uint32_t q=0;
        for(;;){
            if(!available){
                if(p==end)return false;
                bits=(uint32_t)*p++<<24;available=8;
            }
            const uint32_t one=bits&0x80000000u;
            bits<<=1;--available;
            if(one)break;
            if(++q>max_q)return false;
        }
        uint32_t remainder=0;
        if(k){
            while(available<k){
                if(p==end)return false;
                /* k<=16 and available<k, so the two-byte append uses
                 * a defined1..16-bit shift and leaves at most31 bits.
                 * Amortize the SH-2 variable shift across two input bytes. */
                if((size_t)(end-p)>=2u){
                    bits|=(uint32_t)be16(p)<<(16u-available);
                    p+=2;available+=16;
                }else{
                    bits|=(uint32_t)*p++<<(24u-available);available+=8;
                }
            }
            remainder=bits>>((32u-k)&31u);
            bits<<=k;available-=k;
        }
        const uint32_t u=(q<<k)|remainder;
        if(u>maximum)return false;
        int32_t residual=(int32_t)(u>>1);
        if(u&1u)residual=-residual-1;
        const int32_t predictor=order==1?previous:2*previous-older;
        const int32_t value=predictor+residual;
        if(value<INT16_MIN||value>INT16_MAX)return false;
        out[i]=(int16_t)value;older=previous;previous=value;
    }
    /* Every consumed bit was shifted out. Unconsumed padding stays at MSB. */
    return p==end && available<=7u && bits==0;
}
#define RICE_FAST(K) \
static __attribute__((noinline)) bool decode_k##K( \
    const uint8_t *p,const uint8_t *end,int16_t *out,unsigned n) \
{ return decode_residuals(p,end,out,n,1u,K); }
RICE_FAST(4)
RICE_FAST(5)
RICE_FAST(6)
RICE_FAST(7)
RICE_FAST(8)
RICE_FAST(9)
RICE_FAST(10)
RICE_FAST(11)
#undef RICE_FAST

bool wp0_rice_decode_block(const uint8_t *src,size_t bytes,int16_t *out,
                          size_t capacity,unsigned *samples)
{
    if(!src||!out||!samples||bytes<WP0_RICE_BLOCK_HEADER)return false;
    const unsigned n=be16(src),payload=be16(src+2),order=src[4],k=src[5];
    if(!n||n>WP0_PCM_BLOCK||n>capacity||payload!=bytes-8u||
       src[6]||src[7]||order>2u||k>16u)return false;
    const uint8_t *p=src+8;
    if(order==0){
        if(k||payload!=n*2u)return false;
        for(unsigned i=0;i<n;i++)out[i]=(int16_t)be16(p+i*2u);
    }else{
        if(n<order||payload<order*2u)return false;
        for(unsigned i=0;i<order;i++)out[i]=(int16_t)be16(p+i*2u);
        const uint8_t *start=p+order*2u,*end=src+bytes;
        bool valid;
        if(order==1u){
            switch(k){
#define CASE(K) case K:valid=decode_k##K(start,end,out,n);break;
                CASE(4) CASE(5) CASE(6) CASE(7)
                CASE(8) CASE(9) CASE(10) CASE(11)
#undef CASE
                default:valid=decode_residuals(start,end,out,n,order,k);break;
            }
        }else valid=decode_residuals(start,end,out,n,order,k);
        if(!valid)return false;
    }
    *samples=n;return true;
}
bool wp0_rice_open(wp0_rice_stream_t *s,wp0_audio_read_fn read,
                   void *user,uint32_t file_bytes)
{
    uint8_t h[WP0_RICE_HEADER_BYTES];
    if(!s||!read||file_bytes<sizeof h||!read(0,h,sizeof h,user))return false;
    if(be32(h)!=0x57525030u||be16(h+4)!=1u||
       be16(h+6)!=WP0_PCM_BLOCK||be32(h+8)!=WP0_PCM_RATE||
       !be32(h+12)||be32(h+12)>UINT32_MAX-WP0_PCM_BLOCK||
       be32(h+16)!=(be32(h+12)+WP0_PCM_BLOCK-1u)/WP0_PCM_BLOCK||
       be32(h+20)!=file_bytes||be32(h+28))return false;
    memset(s,0,sizeof *s);s->read=read;s->user=user;
    s->file_bytes=file_bytes;s->total_samples=be32(h+12);
    s->block_count=be32(h+16);s->pcm_fnv1a=be32(h+24);s->offset=sizeof h;
    return true;
}
bool wp0_rice_next(wp0_rice_stream_t *s,int16_t out[WP0_PCM_BLOCK],unsigned *samples)
{
    if(!s||!out||!samples||s->next_block>=s->block_count||
       s->offset>s->file_bytes||s->file_bytes-s->offset<8u)return false;
    uint8_t *p=s->staging;
    if(!s->read(s->offset,p,8,s->user))return false;
    const unsigned payload=be16(p+2);
    if(payload>sizeof s->staging-8u||payload>s->file_bytes-s->offset-8u||
       !s->read(s->offset+8u,p+8,payload,s->user))return false;
    unsigned n;
    if(!wp0_rice_decode_block(p,8u+payload,out,WP0_PCM_BLOCK,&n))return false;
    const uint32_t left=s->total_samples-s->decoded_samples;
    if(n!=(left>WP0_PCM_BLOCK?WP0_PCM_BLOCK:left))return false;
    s->offset+=8u+payload;s->next_block++;s->decoded_samples+=n;
    if(s->next_block==s->block_count&&
       (s->offset!=s->file_bytes||s->decoded_samples!=s->total_samples))return false;
    *samples=n;return true;
}
