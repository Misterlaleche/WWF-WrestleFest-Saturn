#ifndef WF_R18_TEST_CD_BLOCK_H
#define WF_R18_TEST_CD_BLOCK_H
#include <stdint.h>
typedef uint32_t fad_t;
#define LBA2FAD(lba) ((fad_t)((lba) + 150u))
int cd_block_sector_read(fad_t fad, void *output_buffer);
int cd_block_sectors_read(fad_t fad, void *output_buffer, uint32_t length);
#endif
