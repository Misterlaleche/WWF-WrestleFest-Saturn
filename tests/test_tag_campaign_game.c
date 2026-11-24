/* Include boundary for a physical-pad driver in the continuous host probe.
 * The original application's input mapping and game callbacks remain intact;
 * only its hardware key reader is replaced. The driver may inspect runtime
 * state and return Jo key bits, never write runtime/actor/progression state. */
#include "native/jo_host/jo/jo.h"
static bool campaign_pad_read(unsigned port,unsigned key);
#define jo_is_input_key_pressed campaign_pad_read
#include "../port/main.c"
#undef jo_is_input_key_pressed
