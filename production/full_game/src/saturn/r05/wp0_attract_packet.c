/* Derived from supplied session14 R05 reference/source; original retained read-only.
 * Silent fixed-attract integration, 2026-11-02. See PROVENANCE.md. */
/* WFA5 is defined by source/tools/attract_native.py HEADER/decode_packet.
 * The first pass validates every byte range and record before the dispatch
 * pass can invoke an application callback. No hardware state is assumed. */
#include "wp0_attract_packet.h"

/* validate rejects a non-4-aligned packet before its first scalar read. Every
 * section before PCM starts at4-alignment and advances by a multiple of4;
 * be16 offsets are even and be32 offsets multiples of4. After optional PCM,
 * only byte padding is inspected. The dispatch pass preserves those offsets.
 * Builtin memcpy keeps the loads alias-safe; assume_aligned supplies only the
 * alignment already established by those invariants. On big-endian SH-2 this
 * is one MOV.W/MOV.L instead of assembling each scalar with byte loads. */
#if defined(__GNUC__) && defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static uint16_t be16(const uint8_t *p){
    uint16_t value;__builtin_memcpy(&value,__builtin_assume_aligned(p,2),sizeof value);return value;
}
static uint32_t be32(const uint8_t *p){
    uint32_t value;__builtin_memcpy(&value,__builtin_assume_aligned(p,4),sizeof value);return value;
}
#else
static uint16_t be16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]<<8|p[1]);}
static uint32_t be32(const uint8_t *p){return (uint32_t)be16(p)<<16|be16(p+2);}
#endif
static int32_t signed16(const uint8_t *p){
    uint32_t n=be16(p);return n<32768u?(int32_t)n:(int32_t)n-65536;
}
static const uint16_t slots[4]={1024,1024,4096,3584};
static const uint16_t strides[4]={128,128,32,128};
typedef struct {const uint8_t *data;uint32_t size,pos;} cursor_t;
static const uint8_t *take(cursor_t *c,uint32_t bytes){
    if(bytes>c->size-c->pos)return NULL;
    const uint8_t *p=c->data+c->pos;c->pos+=bytes;return p;
}
static uint32_t crc32(const uint8_t *p,uint32_t bytes){
    uint32_t crc=0xffffffffu;
    for(uint32_t i=0;i<bytes;i++){
        crc^=p[i];
        for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return ~crc;
}

bool wp0_attract_packet_validate(const void *data,uint32_t bytes,bool check_crc,
                                 wp0_attract_meta_t *metadata){
    if(!data||((uintptr_t)data&3u)||bytes<WP0_ATTRACT_HEADER_BYTES||(bytes&3u))return false;
    const uint8_t *p=data;
    if(p[0]!='W'||p[1]!='F'||p[2]!='A'||p[3]!='5'||be32(p+4)!=bytes||
       (be16(p+30)&~WP0_ATTRACT_PCM_ZERO)||be32(p+60))return false;
    wp0_attract_meta_t m={0};
    m.output_frame=be32(p+8);m.source_frame=be32(p+12);
    m.texture_runs=be16(p+16);m.map_runs=be16(p+18);
    m.palette_count=be16(p+20);m.sprite_count=be16(p+22);
    m.priority=be16(p+24);m.pcm_samples=be16(p+26);m.scene=be16(p+28);m.flags=be16(p+30);
    for(unsigned i=0;i<4;i++)m.scroll[i]=be16(p+32+i*2);
    m.pcm_offset=be32(p+40);m.logic=be32(p+44);m.source_pc=be32(p+48);
    m.line_runs=be32(p+56);
    if(m.pcm_samples||m.pcm_offset||m.flags||m.palette_count>1024u||
       (m.sprite_count!=WP0_ATTRACT_RETAIN&&m.sprite_count>1021u)||
       (m.line_runs!=WP0_ATTRACT_RETAIN&&m.line_runs>480u))return false;
    if(check_crc&&crc32(p+64,bytes-64u)!=be32(p+52))return false;
    cursor_t cursor={p,bytes,64};
    for(uint32_t i=0;i<m.texture_runs;i++){
        const uint8_t *r=take(&cursor,8);if(!r)return false;
        uint32_t kind=be16(r),first=be16(r+2),count=be16(r+4);
        if(kind>3u||!count||be16(r+6)||first+count>slots[kind])return false;
        if(!take(&cursor,count*strides[kind]))return false;
    }
    for(uint32_t i=0;i<m.map_runs;i++){
        const uint8_t *r=take(&cursor,8);if(!r)return false;
        uint32_t kind=be16(r),first=be16(r+2),count=be16(r+4);
        if(kind>2u||!count||be16(r+6)||first+count>4096u)return false;
        r=take(&cursor,count*4u);if(!r)return false;
        /* Two-word 8x8 pattern name: two flip bits, six palette bits and
         * fifteen character-address bits. Every kind owns 4096 characters. */
        for(uint32_t j=0;j<count;j++){
            uint32_t word=be32(r+j*4u),character=word&0x7fffu,base=kind*4096u;
            if((word&~0xc03f7fffu)||character<base||character>=base+4096u)return false;
        }
    }
    for(uint32_t i=0;i<m.palette_count;i++){
        const uint8_t *r=take(&cursor,4);if(!r||be16(r)>=1024u)return false;
        /* Preserve the original 16-bit palette word. The renderer converts
         * its RGB444 low bits; original upper bits are not packet padding. */
    }
    if(m.sprite_count!=WP0_ATTRACT_RETAIN){
        const uint8_t *r=take(&cursor,m.sprite_count*8u);if(!r)return false;
        for(uint32_t i=0;i<m.sprite_count;i++,r+=8){
            uint32_t tile=be16(r),attr=be16(r+6);
            if(tile==0xffffu){
                int32_t x=signed16(r+2),y=signed16(r+4);
                if(x||y<0||attr<(uint32_t)y||attr>=240u)return false;
            }else if(tile>=slots[3]||attr>63u)return false;
        }
    }
    if(m.line_runs!=WP0_ATTRACT_RETAIN){
        const uint8_t *r=take(&cursor,m.line_runs*12u);if(!r)return false;
        for(uint32_t i=0;i<m.line_runs;i++,r+=12){
            uint32_t kind=be16(r),first=be16(r+2),count=be16(r+4);
            int32_t x=signed16(r+8),y=signed16(r+10);
            if(kind>1u||!count||first+count>240u||be16(r+6)||
               x<0||x>7||y<0||(uint32_t)y+count>512u)return false;
        }
    }
    if(!(m.flags&WP0_ATTRACT_PCM_ZERO)&&!take(&cursor,m.pcm_samples*2u))return false;
    uint32_t padding=(0u-cursor.pos)&3u;
    const uint8_t *tail=take(&cursor,padding);if(!tail)return false;
    for(uint32_t i=0;i<padding;i++)if(tail[i])return false;
    if(cursor.pos!=bytes)return false;
    if(metadata)*metadata=m;
    return true;
}

static bool operations_valid(const wp0_attract_packet_ops_t *ops){
    return ops&&ops->texture&&ops->map&&ops->palette&&ops->sprites&&ops->lines&&ops->pcm;
}
static bool dispatch_validated(const void *data,const wp0_attract_meta_t *validated,
                               const wp0_attract_packet_ops_t *ops,wp0_attract_meta_t *metadata){
    const wp0_attract_meta_t m=*validated;
    const uint8_t *p=(const uint8_t *)data+64;
    /* No callback may mutate the packet or re-enter dispatch with its storage.
     * Every span below has already been checked, including all later spans. */
    for(uint32_t i=0;i<m.texture_runs;i++){
        uint16_t kind=be16(p),first=be16(p+2),count=be16(p+4);p+=8;
        if(!ops->texture(ops->context,kind,first,count,p))return false;
        p+=(uint32_t)count*strides[kind];
    }
    for(uint32_t i=0;i<m.map_runs;i++){
        uint16_t kind=be16(p),first=be16(p+2),count=be16(p+4);p+=8;
        if(!ops->map(ops->context,kind,first,count,p))return false;
        p+=(uint32_t)count*4u;
    }
    for(uint32_t i=0;i<m.palette_count;i++,p+=4)
        if(!ops->palette(ops->context,be16(p),be16(p+2)))return false;
    if(!ops->sprites(ops->context,m.sprite_count,m.sprite_count==WP0_ATTRACT_RETAIN?NULL:p))return false;
    if(m.sprite_count!=WP0_ATTRACT_RETAIN)p+=(uint32_t)m.sprite_count*8u;
    if(m.line_runs!=WP0_ATTRACT_RETAIN)for(uint32_t i=0;i<m.line_runs;i++,p+=12)
        if(!ops->lines(ops->context,be16(p),be16(p+2),be16(p+4),
                       (int16_t)signed16(p+8),(int16_t)signed16(p+10)))return false;
    if(!ops->pcm(ops->context,m.pcm_offset,m.pcm_samples,
                 (m.flags&WP0_ATTRACT_PCM_ZERO)?NULL:p))return false;
    if(metadata)*metadata=m;
    return true;
}

bool wp0_attract_packet_dispatch(const void *data,uint32_t bytes,bool check_crc,
                                 const wp0_attract_packet_ops_t *ops,wp0_attract_meta_t *metadata){
    if(!operations_valid(ops))return false;
    wp0_attract_meta_t m;
    if(!wp0_attract_packet_validate(data,bytes,check_crc,&m))return false;
    return dispatch_validated(data,&m,ops,metadata);
}
bool wp0_attract_packet_prepare(wp0_attract_prepared_t *view,const void *data,
                                uint32_t bytes,bool check_crc){
    if(!view)return false;
    *view=(wp0_attract_prepared_t){0};
    wp0_attract_meta_t m;
    if(!wp0_attract_packet_validate(data,bytes,check_crc,&m))return false;
    view->data=data;view->bytes=bytes;view->meta=m;view->valid=true;
    return true;
}
bool wp0_attract_packet_dispatch_prepared(const wp0_attract_prepared_t *view,
                                         const wp0_attract_packet_ops_t *ops,wp0_attract_meta_t *metadata){
    if(!operations_valid(ops)||!view||!view->valid||!view->data||
       ((uintptr_t)view->data&3u)||view->bytes<WP0_ATTRACT_HEADER_BYTES||(view->bytes&3u))return false;
    /* These are sanity checks, not proof against a forged or mutated view.
     * The caller must retain prepare's validated immutable storage/view. */
    if(be32(view->data)!=0x57464135u||be32(view->data+4)!=view->bytes)return false;
    return dispatch_validated(view->data,&view->meta,ops,metadata);
}
