#include "wp0_rice.h"
#include <string.h>

static uint16_t be16(const uint8_t *p)
{ return (uint16_t)(((unsigned)p[0]<<8)|p[1]); }
static uint32_t be32(const uint8_t *p)
{ return ((uint32_t)be16(p)<<16)|be16(p+2); }
typedef struct { const uint8_t *p,*end; uint32_t word; unsigned available; } bits_t;

static inline bool take_bits(bits_t *b,unsigned n,uint32_t *out)
{
    while(b->available<n){
        if(b->p==b->end)return false;
        b->word=(b->word<<8)|*b->p++;b->available+=8;
    }
    b->available-=n;
    *out=(b->word>>b->available)&((1u<<n)-1u);
    return true;
}
static inline bool rice_value(bits_t *b,unsigned k,uint32_t max,uint32_t *out)
{
    uint32_t q=0,bit,rem=0;
    do{
        if(!take_bits(b,1,&bit))return false;
        if(!bit&&++q>(max>>k))return false;
    }while(!bit);
    if(k&&!take_bits(b,k,&rem))return false;
    *out=(q<<k)|rem;return *out<=max;
}
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
        bits_t bits={p+order*2u,src+bytes,0,0};
        for(unsigned i=order;i<n;i++){
            uint32_t u;
            if(!rice_value(&bits,k,order==1?131070u:262140u,&u))return false;
            int32_t residual=(int32_t)(u>>1);
            if(u&1u)residual=-residual-1;
            const int32_t predictor=order==1?out[i-1]:2*(int32_t)out[i-1]-out[i-2];
            const int32_t value=predictor+residual;
            if(value<INT16_MIN||value>INT16_MAX)return false;
            out[i]=(int16_t)value;
        }
        /* Canonical packing ends on this byte; only zero bit padding follows. */
        if(bits.p!=bits.end||bits.available>7u||
           (bits.word&((1u<<bits.available)-1u)))return false;
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
