#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wf_r18_present.h"

static uint8_t rom[524288], tile_cache[384u*1024u];
static FILE *pack, *hot;
static unsigned sprite_reads;
static unsigned corrupt;

static int read_file(const char *name, uint32_t offset, void *dst, size_t bytes)
{
    FILE *file;
    if (!strcmp(name, "SPRITES.PAK")) { file=pack; ++sprite_reads; }
    else if (!strcmp(name, "HOTSPR.BIN")) file=hot;
    else return -1;
    if (fseek(file, (long)offset, SEEK_SET) != 0 ||
        fread(dst, 1, bytes, file) != bytes) return -1;
    if (file==hot && !offset) {
        uint8_t *p=dst;
        if (corrupt==1u) p[0]=0;
        if (corrupt==2u) { p[10]=p[8]; p[11]=p[9]; }
        if (corrupt==3u) p[7]=127u;
        if (corrupt==4u) { p[4]=255u; p[5]=255u; }
    }
    return file==hot && offset && corrupt==5u ? -1 : 0;
}

int main(int argc, char **argv)
{
    uint8_t header[4096], expected[128];
    const uint8_t *got;
    unsigned count;
    if (argc != 3 || !(pack=fopen(argv[1], "rb")) ||
        !(hot=fopen(argv[2], "rb"))) return 2;
    if (fread(header, 1, sizeof header, hot) != sizeof header) return 2;
    count=(unsigned)header[4]<<8 | header[5];
    wf_r18_present_init(read_file, rom, sizeof rom, tile_cache, sizeof tile_cache);
    if (wf_r18_hot_sprites_load() != WF_OK || count == 0) {
        fputs("Hot sprite pack failed to load\n",stderr); return 1;
    }
    for (unsigned i=0; i<count; ++i) {
        unsigned tile=(unsigned)header[8+i*2]<<8 | header[9+i*2];
        if (fseek(pack,(long)tile*128,SEEK_SET) != 0 ||
            fread(expected,1,sizeof expected,pack) != sizeof expected ||
            wf_r18_tile_read((uint16_t)tile,&got) != WF_OK ||
            memcmp(expected,got,sizeof expected) != 0 || sprite_reads != 0) {
            fprintf(stderr,"Hot tile %u differs or caused a pack read\n",tile);
            return 1;
        }
    }
    if (fseek(pack,65535L*128L,SEEK_SET) != 0 ||
        fread(expected,1,sizeof expected,pack) != sizeof expected ||
        wf_r18_tile_read(65535u,&got) != WF_OK ||
        memcmp(expected,got,sizeof expected) != 0 || sprite_reads != 1u ||
        wf_r18_tile_read(65535u,&got) != WF_OK || sprite_reads != 1u) {
        fputs("Cold tile fallback differs or rereads the pack\n",stderr);
        return 1;
    }
    for (corrupt=1u; corrupt<=5u; ++corrupt) {
        wf_r18_present_init(read_file,rom,sizeof rom,tile_cache,sizeof tile_cache);
        if (wf_r18_hot_sprites_load()!=WF_BLOCKED ||
            wf_r18_hot_sprites_bytes()!=0u) {
            fprintf(stderr,"Malformed hot pack %u accepted\n",corrupt);
            return 1;
        }
    }
    puts("PASS RUNTIME-18 hot sprites: exact bytes, zero hot CD reads, cold fallback, malformed rejection");
    return 0;
}
