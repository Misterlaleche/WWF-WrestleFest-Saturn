#include "wp1979_input.h"
#include <stddef.h>
typedef struct { uint16_t frame;uint8_t level; } input_event_t;
static const input_event_t input_tape[]={
#include "wp1979_tape.inc"
};
#ifdef WF_R18_REPLAY_NOINPUT
volatile uint32_t wp1979_demo_active=0u;
#else
volatile uint32_t wp1979_demo_active=1u;
#endif
uint8_t wp1979_input(uint32_t frame,uint8_t physical)
{
    static size_t index;
    if(physical!=0xffu)wp1979_demo_active=0u;
    if(!wp1979_demo_active)return physical;
    while(index+1u<sizeof input_tape/sizeof input_tape[0]&&input_tape[index+1u].frame<=frame)++index;
    return input_tape[index].level;
}
