#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cd-block.h"
#include "wf_r18_cdfs.h"

volatile uint32_t video_clock;
static FILE *disc;
static unsigned single_reads, block_reads;
static uint8_t bulk_got[32768] __attribute__((aligned(32)));
static uint8_t bulk_expected[32768];

static int disc_read(fad_t fad, void *out, size_t bytes)
{
    if (fad < 150u || fseek(disc, (long)(fad-150u)*2048L, SEEK_SET)) return -1;
    return fread(out, 1, bytes, disc) == bytes ? 0 : -1;
}
int cd_block_sector_read(fad_t fad, void *out)
{
    ++single_reads;
    return disc_read(fad, out, 2048u);
}
int cd_block_sectors_read(fad_t fad, void *out, uint32_t bytes)
{
    ++block_reads;
    return disc_read(fad, out, bytes);
}

int main(int argc, char **argv)
{
    uint8_t got[16384], expected[16384];
    uint32_t size = 0;
    if (argc != 3) return 2;
    disc = fopen(argv[1], "rb");
    FILE *pack = fopen(argv[2], "rb");
    if (!disc || !pack) return 2;
    if (wf_r18_cdfs_init() != 0 ||
        wf_r18_file_size("SPRITES.PAK", &size) != 0 || size != 8388608u ||
        fread(expected, 1, sizeof expected, pack) != sizeof expected ||
        wf_r18_file_read("SPRITES.PAK", 0, got, sizeof got) != 0 ||
        memcmp(got, expected, sizeof got) != 0) {
        fputs("CDFS payload differs from the sprite pack\n", stderr);
        return 1;
    }
    /* Eight contiguous sectors must take one CD command after directory load. */
    if (block_reads != 1u || single_reads > 2u) {
        fprintf(stderr, "Contiguous sprite read used %u single and %u block commands\n",
                single_reads, block_reads);
        return 1;
    }
    if (fseek(pack, 16384L-16L, SEEK_SET) ||
        fread(expected, 1, 32u, pack) != 32u ||
        wf_r18_file_read("SPRITES.PAK", 16384u-16u, got, 32u) != 0 ||
        memcmp(got, expected, 32u) != 0) {
        fputs("CDFS read across a block boundary differs\n", stderr);
        return 1;
    }
    if (wf_r18_file_read("SPRITES.PAK", size-31u, got, 32u) == 0) {
        fputs("CDFS accepted a read past the file end\n", stderr);
        return 1;
    }
    if (fseek(pack, 65536L, SEEK_SET) ||
        fread(bulk_expected, 1, sizeof bulk_expected, pack) != sizeof bulk_expected) return 2;
    unsigned before = block_reads;
    if (wf_r18_file_read("SPRITES.PAK", 65536u, bulk_got,
                         sizeof bulk_got) != 0 ||
        memcmp(bulk_got, bulk_expected, sizeof bulk_got) != 0 ||
        block_reads-before != 1u) {
        fprintf(stderr, "Aligned bulk read used %u CD commands\n",block_reads-before);
        return 1;
    }
    /* Independent working sets beyond the resident window count must
     * evict/reload exact source bytes, including unaligned sector crossings. */
    for(unsigned round=0;round<3u;round++)for(unsigned which=0;which<6u;which++){
        uint32_t offset=131072u+which*32768u+2041u;
        if(fseek(pack,(long)offset,SEEK_SET)||fread(expected,1,128u,pack)!=128u||
           wf_r18_file_read("SPRITES.PAK",offset,got,128u)!=0||memcmp(got,expected,128u)){
            fputs("CDFS window eviction differs from source bytes\n",stderr);return 1;
        }
    }
    uint32_t refills[WF_R18_CDFS_FILE_CLASSES]={0};
    uint32_t bulk[WF_R18_CDFS_FILE_CLASSES]={0};
    uint32_t total_refills=0;
    wf_r18_cdfs_file_stats(refills,bulk);
    wf_r18_cdfs_stats(&total_refills,NULL);
    if (!refills[1] || refills[1]!=total_refills || bulk[1]!=16u) {
        fputs("CDFS file attribution differs from command totals\n",stderr);
        return 1;
    }
    for(unsigned i=0;i<WF_R18_CDFS_FILE_CLASSES;i++)if(i!=1u&&(refills[i]||bulk[i])){
        fputs("CDFS attributed sprite reads to another file class\n",stderr);
        return 1;
    }
    fclose(pack);
    fclose(disc);
    puts("PASS RUNTIME-18 contiguous CDFS payload and command count");
    return 0;
}
