#include "wf_r18_cdfs.h"
#include <cd-block.h>
#include <string.h>

#define SECTOR_BYTES 2048u
#define ROOT_CAPACITY 64u
#define WINDOW_SECTORS 8u
#define WINDOW_COUNT 8u
typedef struct { char name[13]; uint32_t lba, size; } root_file_t;
typedef struct {
    uint32_t data[WINDOW_SECTORS * SECTOR_BYTES / sizeof(uint32_t)];
    uint32_t first_lba, sectors, last_used;
} sector_window_t;
/* Static BSS is linked into high work RAM; never use caller stack or VDP RAM. */
static root_file_t files[ROOT_CAPACITY];
static uint32_t cache[SECTOR_BYTES / sizeof(uint32_t)];
static sector_window_t windows[WINDOW_COUNT] __attribute__((aligned(32)));
static uint32_t window_clock;
static uint32_t cached_lba, volume_sectors, file_count;
static int cache_valid, ready;
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[3] | (uint32_t)p[2]<<8 | (uint32_t)p[1]<<16 | (uint32_t)p[0]<<24;
}
static int fetch(uint32_t lba) {
    if (lba > UINT32_MAX - 150u) return -1;
    if (cache_valid && cached_lba == lba) return 0;
    cache_valid = 0;
    if (ready) {
        unsigned slot = 0u;
        for (unsigned i=0; i<WINDOW_COUNT; ++i) {
            if (windows[i].sectors && lba >= windows[i].first_lba &&
                lba-windows[i].first_lba < windows[i].sectors) {
                slot=i;
                goto window_hit;
            }
        }
        if (lba >= volume_sectors) return -1;
        for (unsigned i=0; i<WINDOW_COUNT; ++i)
            if (!windows[i].sectors || windows[i].last_used < windows[slot].last_used)
                slot=i;
        uint32_t first = lba;
        uint32_t count = volume_sectors-first;
        if (count > WINDOW_SECTORS) count=WINDOW_SECTORS;
        if (cd_block_sectors_read(LBA2FAD(first), windows[slot].data,
                                  count*SECTOR_BYTES) != 0) return -1;
        windows[slot].first_lba=first;
        windows[slot].sectors=count;
window_hit:
        windows[slot].last_used=++window_clock;
        memcpy(cache,(const uint8_t *)windows[slot].data+
                     (lba-windows[slot].first_lba)*SECTOR_BYTES,SECTOR_BYTES);
        cached_lba=lba; cache_valid=1;
        return 0;
    }
    if (cd_block_sector_read(LBA2FAD(lba), cache) != 0) return -1;
    cached_lba = lba; cache_valid = 1;
    return 0;
}
static int extent_ok(uint32_t lba, uint32_t size) {
    uint32_t sectors = size / SECTOR_BYTES + (size % SECTOR_BYTES != 0u);
    return lba < volume_sectors && sectors <= volume_sectors-lba;
}
int wf_r18_cdfs_init(void) {
    const uint8_t *p = (const uint8_t *)cache;
    uint32_t root_lba, root_size, pos;
    ready = 0; cache_valid = 0; file_count = 0; window_clock=0;
    for (unsigned i=0; i<WINDOW_COUNT; ++i) {
        windows[i].sectors=0;
        windows[i].last_used=0;
    }
    if (fetch(16u) != 0) return -1;
    if (p[0] != 1u || memcmp(p+1,"CD001",5) != 0 || p[6] != 1u ||
        p[128] != 0u || p[129] != 8u || p[130] != 8u || p[131] != 0u) return -1;
    volume_sectors = le32(p+80);
    if (volume_sectors != be32(p+84) || volume_sectors <= 16u ||
        volume_sectors > UINT32_MAX-150u || p[156] < 34u || !(p[181]&2u)) return -1;
    root_lba = le32(p+158); root_size = le32(p+166);
    if (root_lba != be32(p+162) || root_size != be32(p+170) ||
        !root_size || !extent_ok(root_lba,root_size)) return -1;
    for (pos=0; pos<root_size;) {
        uint32_t offset=pos%SECTOR_BYTES, len, lba, size, n, i;
        const uint8_t *r;
        if (fetch(root_lba+pos/SECTOR_BYTES) != 0) return -1;
        r=p+offset; len=r[0];
        if (!len) { uint32_t skip=SECTOR_BYTES-offset; pos += skip < root_size-pos ? skip : root_size-pos; continue; }
        if (len<34u || len>SECTOR_BYTES-offset || len>root_size-pos ||
            r[32]>len-33u || r[1] || r[26] || r[27]) return -1;
        pos+=len;
        if (r[25]&2u) continue;
        if (r[25]&0x80u) return -1; /* multi-extent files unsupported */
        lba=le32(r+2); size=le32(r+10);
        if (lba!=be32(r+6) || size!=be32(r+14) || !extent_ok(lba,size)) return -1;
        n=r[32];
        if (n>=2u && r[33+n-2u]==';' && r[33+n-1u]=='1') n-=2u;
        if (!n || n>12u || file_count==ROOT_CAPACITY) return -1;
        for (i=0; i<n; ++i) {
            uint8_t c=r[33+i];
            if (!((c>='A' && c<='Z') || (c>='0' && c<='9') || c=='_' || c=='.')) return -1;
            files[file_count].name[i]=(char)c;
        }
        files[file_count].name[n]='\0';
        for (i=0; i<file_count; ++i)
            if (strcmp(files[i].name,files[file_count].name)==0) return -1;
        files[file_count].lba=lba; files[file_count].size=size; ++file_count;
    }
    ready=1;
    return 0;
}
static const root_file_t *lookup(const char *name) {
    uint32_t i;
    if (!ready || !name) return NULL;
    for (i=0; i<file_count; ++i) if (strcmp(name,files[i].name)==0) return &files[i];
    return NULL;
}
int wf_r18_file_size(const char *name, uint32_t *out) {
    const root_file_t *file=lookup(name);
    if (!file || !out) return -1;
    *out=file->size; return 0;
}
int wf_r18_file_read(const char *name, uint32_t offset, void *dst, size_t bytes) {
    const root_file_t *file=lookup(name);
    uint8_t *out=(uint8_t *)dst;
    if (!file || (!dst && bytes) || offset>file->size || bytes>file->size-offset) return -1;
    while (bytes) {
        uint32_t within=offset%SECTOR_BYTES;
        size_t take=SECTOR_BYTES-within;
        if (!within && bytes>=16u*SECTOR_BYTES &&
            ((uintptr_t)out&31u)==0u) {
            if (cd_block_sectors_read(LBA2FAD(file->lba+offset/SECTOR_BYTES),
                                      out,16u*SECTOR_BYTES)!=0) return -1;
            out+=16u*SECTOR_BYTES;
            offset+=16u*SECTOR_BYTES;
            bytes-=16u*SECTOR_BYTES;
            continue;
        }
        if (take>bytes) take=bytes;
        if (fetch(file->lba+offset/SECTOR_BYTES)!=0) return -1;
        memcpy(out,(const uint8_t *)cache+within,take);
        out+=take; offset+=(uint32_t)take; bytes-=take;
    }
    return 0;
}
