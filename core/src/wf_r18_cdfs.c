#include "wf_r18_cdfs.h"
#include <cd-block.h>
#include "wp1979_cd_read.h"
#include <string.h>

#define SECTOR_BYTES 2048u
#define ROOT_CAPACITY 64u
#define WINDOW_SECTORS_TOTAL 8u
#define WINDOW_COUNT_MAX 4u
typedef struct { char name[13]; uint32_t lba, size; } root_file_t;
typedef struct {
    uint32_t first_lba, sectors, last_used;
} sector_window_t;
/* Static BSS is linked into high work RAM; never use caller stack or VDP RAM. */
static root_file_t files[ROOT_CAPACITY];
static uint32_t cache[SECTOR_BYTES / sizeof(uint32_t)];
static uint32_t window_data[WINDOW_SECTORS_TOTAL * SECTOR_BYTES / sizeof(uint32_t)] __attribute__((aligned(32)));
static sector_window_t windows[WINDOW_COUNT_MAX];
static unsigned window_span=8u, window_count=1u;
static uint32_t window_clock;
static volatile uint32_t window_misses, window_sectors;
static uint32_t file_refills[WF_R18_CDFS_FILE_CLASSES];
static uint32_t file_bulk_sectors[WF_R18_CDFS_FILE_CLASSES];
static unsigned active_file_class=5u;
static uint32_t cached_lba, volume_sectors, file_count;
static int cache_valid, ready;
/* WP1979 read-only bootstrap diagnostics. */
volatile uint32_t wp1979_cdfs_stage,wp1979_cdfs_lba,wp1979_cdfs_probe[4];
volatile int32_t wp1979_cdfs_rc;
void wf_r18_cdfs_stats(uint32_t *out_windows, uint32_t *out_sectors) {
    if (out_windows) *out_windows=window_misses;
    if (out_sectors) *out_sectors=window_sectors;
}
void wf_r18_cdfs_file_stats(uint32_t refills[WF_R18_CDFS_FILE_CLASSES],
                             uint32_t bulk_sectors[WF_R18_CDFS_FILE_CLASSES]) {
    if (refills) memcpy(refills,file_refills,sizeof file_refills);
    if (bulk_sectors) memcpy(bulk_sectors,file_bulk_sectors,sizeof file_bulk_sectors);
}
static unsigned file_class(const char *name) {
    if (!strcmp(name,"ROM68K.BIN")) return 0u;
    if (!strcmp(name,"SPRITES.PAK")) return 1u;
    if (!strcmp(name,"HOTSPR.BIN") || !strcmp(name,"FRONTSPR.BIN") ||
        !strcmp(name,"ROYSPR.BIN") || !strcmp(name,"POSTSPR.BIN")) return 2u;
    if (name[0]=='R' && name[1]=='1' && name[2]=='8') return 3u;
    if (name[0]=='M' && name[1]=='U' && name[2]=='S') return 4u;
    return 5u;
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[3] | (uint32_t)p[2]<<8 | (uint32_t)p[1]<<16 | (uint32_t)p[0]<<24;
}
static int fetch(uint32_t lba) {
    wp1979_cdfs_lba=lba;
    if (lba > UINT32_MAX - 150u) return -1;
    if (cache_valid && cached_lba == lba) return 0;
    cache_valid = 0;
    if (ready) {
        unsigned slot = 0u;
        for (unsigned i=0; i<window_count; ++i) {
            if (windows[i].sectors && lba >= windows[i].first_lba &&
                lba-windows[i].first_lba < windows[i].sectors) {
                slot=i;
                goto window_hit;
            }
        }
        if (lba >= volume_sectors) return -1;
        for (unsigned i=0; i<window_count; ++i)
            if (!windows[i].sectors || windows[i].last_used < windows[slot].last_used)
                slot=i;
        uint32_t first = lba;
        uint32_t count = volume_sectors-first;
        if (count > window_span) count=window_span;
        uint32_t *data=window_data+slot*window_span*SECTOR_BYTES/sizeof(uint32_t);
        if (wp1979_cd_sectors_read(LBA2FAD(first), data,
                                  count*SECTOR_BYTES) != 0) return -1;
        ++window_misses; window_sectors += count;
        ++file_refills[active_file_class];
        windows[slot].first_lba=first;
        windows[slot].sectors=count;
window_hit:
        windows[slot].last_used=++window_clock;
        memcpy(cache,(const uint8_t *)window_data+slot*window_span*SECTOR_BYTES+
                     (lba-windows[slot].first_lba)*SECTOR_BYTES,SECTOR_BYTES);
        cached_lba=lba; cache_valid=1;
        return 0;
    }
    wp1979_cdfs_rc=wp1979_cd_sector_read(LBA2FAD(lba), cache);
    for(unsigned i=0;i<4u;++i)wp1979_cdfs_probe[i]=cache[i];
    if (wp1979_cdfs_rc != 0) return -1;
    cached_lba = lba; cache_valid = 1;
    return 0;
}
static int extent_ok(uint32_t lba, uint32_t size) {
    uint32_t sectors = size / SECTOR_BYTES + (size % SECTOR_BYTES != 0u);
    return lba < volume_sectors && sectors <= volume_sectors-lba;
}
void wf_r18_cdfs_combat_window_set(unsigned combat) {
    unsigned span=combat?2u:8u;
    if (window_span==span) return;
    window_span=span;window_count=combat?4u:1u;cache_valid=0;
    for (unsigned i=0; i<WINDOW_COUNT_MAX; ++i) {
        windows[i].sectors=0;windows[i].last_used=0;
    }
}
int wf_r18_cdfs_init(void) {
    const uint8_t *p = (const uint8_t *)cache;
    uint32_t root_lba, root_size, pos;
    ready = 0; cache_valid = 0; file_count = 0; window_clock=0;
    window_misses=window_sectors=0;
    memset(file_refills,0,sizeof file_refills);
    memset(file_bulk_sectors,0,sizeof file_bulk_sectors);
    active_file_class=5u;
    window_span=8u;window_count=1u;
    for (unsigned i=0; i<WINDOW_COUNT_MAX; ++i) {
        windows[i].sectors=0;
        windows[i].last_used=0;
    }
    wp1979_cdfs_stage=1;
    if (fetch(16u) != 0) return -1;
    wp1979_cdfs_stage=2;
    if (p[0] != 1u || memcmp(p+1,"CD001",5) != 0 || p[6] != 1u ||
        p[128] != 0u || p[129] != 8u || p[130] != 8u || p[131] != 0u) return -1;
    wp1979_cdfs_stage=3;
    volume_sectors = le32(p+80);
    if (volume_sectors != be32(p+84) || volume_sectors <= 16u ||
        volume_sectors > UINT32_MAX-150u || p[156] < 34u || !(p[181]&2u)) return -1;
    wp1979_cdfs_stage=4;
    root_lba = le32(p+158); root_size = le32(p+166);
    if (root_lba != be32(p+162) || root_size != be32(p+170) ||
        !root_size || !extent_ok(root_lba,root_size)) return -1;
    wp1979_cdfs_stage=5;
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
    wp1979_cdfs_stage=6;
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
    unsigned previous_class=active_file_class;
    active_file_class=file_class(file->name);
    while (bytes) {
        uint32_t within=offset%SECTOR_BYTES;
        size_t take=SECTOR_BYTES-within;
        if (!within && bytes>=16u*SECTOR_BYTES &&
            ((uintptr_t)out&31u)==0u) {
            if (wp1979_cd_sectors_read(LBA2FAD(file->lba+offset/SECTOR_BYTES),
                                      out,16u*SECTOR_BYTES)!=0) goto failed;
            window_sectors+=16u;
            file_bulk_sectors[active_file_class]+=16u;
            out+=16u*SECTOR_BYTES;
            offset+=16u*SECTOR_BYTES;
            bytes-=16u*SECTOR_BYTES;
            continue;
        }
        if (take>bytes) take=bytes;
        if (fetch(file->lba+offset/SECTOR_BYTES)!=0) goto failed;
        memcpy(out,(const uint8_t *)cache+within,take);
        out+=take; offset+=(uint32_t)take; bytes-=take;
    }
    active_file_class=previous_class;
    return 0;
failed:
    active_file_class=previous_class;
    return -1;
}
