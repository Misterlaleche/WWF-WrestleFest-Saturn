/* Included after the native transport primitives so the new lease shares its
 * bounded, status-checked command path. Tests supply an independent buffer
 * model for those primitives. All payload FIFO transfers remain native I/O.
 */
#include "wp2015_cd_cache.h"
#include <string.h>
#define RESIDENT_PARTITION 23u
static uint32_t resident_sectors,resident_bytes;
static unsigned resident_loading;
/* P0/P1 cacheable/uncached mappings can denote the same physical RAM. */
static int resident_physical_overlap(uint32_t a,size_t an,uint32_t b,size_t bn){
    a&=0x1fffffffu;b&=0x1fffffffu;
    if(an>0x20000000u-a || bn>0x20000000u-b)return 1;
    return a<b+bn && b<a+an;
}
static void resident_invalidate(void){resident_sectors=0;resident_bytes=0;resident_loading=0;}
static int resident_end(uint32_t expected_words,response_t *response){
    int rc=command(0x0600u,0,0,0,0,response);
    if(rc)return rc;
    uint32_t words=((uint32_t)(response->cr1&255u)<<16)|response->cr2;
    if(words!=expected_words)return WP1979_CD_TRANSFER_COUNT;
    return wait_flag(EHST)?WP1979_CD_READY_TIMEOUT:0;
}
static int resident_restore(int rc,unsigned active,response_t *response){
    if(active)(void)command(0x0600u,0,0,0,0,response);
    int restored=setting(0x6000u,0,0,ESEL,response);
    if(!rc)rc=restored;
    if(rc)resident_invalidate();
    return rc;
}
int wp2015_cd_cache_reset(void){
    response_t response;int rc;
    resident_invalidate();
    if((rc=setting(0x4800u,0,RESIDENT_PARTITION<<8,ESEL,&response)))return rc;
    if((rc=setting(0x4400u,0,RESIDENT_PARTITION<<8,ESEL,&response)))return rc;
    if((rc=setting(0x4603u,(RESIDENT_PARTITION<<8)|255u,RESIDENT_PARTITION<<8,ESEL,&response)))return rc;
    if((rc=setting(0x6000u,0,0,ESEL,&response)))return rc;
    resident_loading=1;
    return 0;
}
int wp2015_cd_cache_append(const void *sector){
    if(!sector || ((uintptr_t)sector&3u) ||
       (uintptr_t)sector>UINTPTR_MAX-(WP2015_CD_CACHE_SECTOR_BYTES-1u))
        return WP2015_CD_CACHE_ARGUMENT;
    if(!resident_loading || resident_bytes || resident_sectors>=WP2015_CD_CACHE_MAX_SECTORS)
        return WP2015_CD_CACHE_STATE;
    response_t response;int rc;unsigned active=0;
    if((rc=setting(0x6003u,0x0300u,0,ESEL,&response)))goto done;
    active=1;
    if((rc=command(0x6400u,0,RESIDENT_PARTITION<<8,1u,DRDY|EHST,&response)))goto done;
    if(wait_flag(DRDY)){rc=WP1979_CD_READY_TIMEOUT;goto done;}
    HIRQ=(uint16_t)~DRDY;
    resident_fifo_put(sector,WP2015_CD_CACHE_SECTOR_BYTES/4u);
    rc=resident_end(WP2015_CD_CACHE_SECTOR_BYTES/2u,&response);active=(rc!=0);
done:
    rc=resident_restore(rc,active,&response);
    if(!rc)++resident_sectors;
    return rc;
}
int wp2015_cd_cache_seal(uint32_t logical_bytes){
    if(!resident_loading || !resident_sectors || resident_bytes)return WP2015_CD_CACHE_STATE;
    if(logical_bytes>(resident_sectors*WP2015_CD_CACHE_SECTOR_BYTES) ||
       logical_bytes<=(resident_sectors-1u)*WP2015_CD_CACHE_SECTOR_BYTES)
        return WP2015_CD_CACHE_ARGUMENT;
    response_t response;
    int rc=command(0x5100u,0,RESIDENT_PARTITION<<8,0,0,&response);
    if(!rc && response.cr4!=resident_sectors)rc=WP1979_CD_BUFFER_COUNT;
    if(rc){resident_invalidate();return rc;}
    resident_bytes=logical_bytes;resident_loading=0;
    return 0;
}
uint32_t wp2015_cd_cache_bytes(void){return resident_bytes;}
int wp2015_cd_cache_read(uint32_t offset,void *dst,size_t bytes,void *scratch,size_t scratch_bytes){
    if(!resident_bytes)return WP2015_CD_CACHE_NOT_READY;
    if(offset>resident_bytes || bytes>resident_bytes-offset)return WP2015_CD_CACHE_ARGUMENT;
    if(!bytes)return 0;
    if(!dst || !scratch || ((uintptr_t)scratch&3u) || scratch_bytes<WP2015_CD_CACHE_SECTOR_BYTES ||
       (uintptr_t)dst>UINTPTR_MAX-(bytes-1u) ||
       (uintptr_t)scratch>UINTPTR_MAX-(scratch_bytes-1u))return WP2015_CD_CACHE_ARGUMENT;
    if((uintptr_t)dst<=(uintptr_t)scratch+scratch_bytes-1u &&
       (uintptr_t)scratch<=(uintptr_t)dst+bytes-1u)return WP2015_CD_CACHE_ARGUMENT;
#ifdef WF_PLATFORM_SATURN
    if(resident_physical_overlap((uint32_t)(uintptr_t)dst,bytes,
                                (uint32_t)(uintptr_t)scratch,scratch_bytes))
        return WP2015_CD_CACHE_ARGUMENT;
#endif
    size_t scratch_sectors=scratch_bytes/WP2015_CD_CACHE_SECTOR_BYTES;
    uint32_t capacity=scratch_sectors>WP2015_CD_CACHE_MAX_SECTORS?
        WP2015_CD_CACHE_MAX_SECTORS:(uint32_t)scratch_sectors;
    response_t response;int rc;unsigned active=0;
    uint8_t *out=dst;
    if((rc=setting(0x6003u,0x0300u,0,ESEL,&response)))goto done;
    while(bytes){
        uint32_t sector=offset/WP2015_CD_CACHE_SECTOR_BYTES;
        uint32_t within=offset%WP2015_CD_CACHE_SECTOR_BYTES;
        uint32_t sectors=(uint32_t)((within+bytes+WP2015_CD_CACHE_SECTOR_BYTES-1u)/WP2015_CD_CACHE_SECTOR_BYTES);
        if(sectors>capacity)sectors=capacity;
        active=1;
        if((rc=command(0x6100u,(uint16_t)sector,RESIDENT_PARTITION<<8,(uint16_t)sectors,DRDY|EHST,&response)))goto done;
        if(wait_flag(DRDY)){rc=WP1979_CD_READY_TIMEOUT;goto done;}
        HIRQ=(uint16_t)~DRDY;
        resident_fifo_get(scratch,sectors*WP2015_CD_CACHE_SECTOR_BYTES/4u);
        rc=resident_end(sectors*WP2015_CD_CACHE_SECTOR_BYTES/2u,&response);active=(rc!=0);
        if(rc)goto done;
        size_t take=sectors*WP2015_CD_CACHE_SECTOR_BYTES-within;
        if(take>bytes)take=bytes;
        memcpy(out,(const uint8_t *)scratch+within,take);
        out+=take;offset+=(uint32_t)take;bytes-=take;
    }
done:
    return resident_restore(rc,active,&response);
}
