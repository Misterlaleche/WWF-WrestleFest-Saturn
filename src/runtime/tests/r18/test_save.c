#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "wf_r18_save.h"

int main(void)
{
    uint8_t factory[WF_R18_SAVE_RANKING_BYTES];
    uint8_t image[WF_R18_SAVE_IMAGE_BYTES];
    wf_r18_save_data_t a, b;
    for (unsigned i = 0; i < sizeof factory; i++) factory[i] = (uint8_t)(i * 17u + 9u);
    wf_r18_save_defaults(&a, factory);
    assert(memcmp(a.rankings, factory, sizeof factory) == 0);
    for (unsigned i = 0; i < sizeof a.options; i++) assert(a.options[i] == 0);
    a.rankings[0] = 0x47;
    a.rankings[131] = 0x81;
    a.options[0] = 3;
    a.options[15] = 0xa5;
    assert(wf_r18_save_encode(&a, image) == 1);
    assert(memcmp(image, "WFS1", 4) == 0);
    assert(image[4] == 0 && image[5] == 1);
    assert(image[6] == 0 && image[7] == WF_R18_SAVE_IMAGE_BYTES);
    memset(&b, 0, sizeof b);
    assert(wf_r18_save_decode(image, &b) == 1);
    assert(memcmp(&a, &b, sizeof a) == 0);
    image[8] ^= 0x40;
    assert(wf_r18_save_decode(image, &b) == 0);
    image[8] ^= 0x40;
    image[5] = 2;
    assert(wf_r18_save_decode(image, &b) == 0);
    image[5] = 1;
    image[7]--;
    assert(wf_r18_save_decode(image, &b) == 0);
    image[7]++;
    image[0] = 'X';
    assert(wf_r18_save_decode(image, &b) == 0);
    return 0;
}
