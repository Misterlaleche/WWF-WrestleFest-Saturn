#ifndef WF_JO_COMMAND_WORDS_H
#define WF_JO_COMMAND_WORDS_H
#include "wf_jo_video.h"

/* Four-byte aligned command buffers let SH-2 write work RAM in longwords.
 * may_alias permits inspecting the same objects as their halfword fields. */
typedef uint32_t wf_jo_command_word_t __attribute__((may_alias));
static inline __attribute__((always_inline)) uint32_t
wf_jo_command_pair(unsigned first,unsigned second)
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return ((uint32_t)(uint16_t)first<<16)|(uint16_t)second;
#else
    return ((uint32_t)(uint16_t)second<<16)|(uint16_t)first;
#endif
}
static inline __attribute__((always_inline)) void
wf_jo_command_write(wf_jo_command_t *out,unsigned ctrl,unsigned color,
                     unsigned address,unsigned size,int x,int y)
{
    wf_jo_command_word_t *words=(wf_jo_command_word_t *)(void *)out;
    words[0]=wf_jo_command_pair(ctrl,0);
    words[1]=wf_jo_command_pair(0x80u,color); /* Pen F is not an end code. */
    words[2]=wf_jo_command_pair(address,size);
    words[3]=wf_jo_command_pair((unsigned)x,(unsigned)y);
    words[4]=words[5]=words[6]=words[7]=0;
}
#endif
