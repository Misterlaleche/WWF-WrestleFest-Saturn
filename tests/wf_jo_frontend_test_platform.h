#ifndef WF_JO_FRONTEND_TEST_PLATFORM_H
#define WF_JO_FRONTEND_TEST_PLATFORM_H
#include <stdbool.h>
enum {JO_KEY_RIGHT=1,JO_KEY_LEFT=2,JO_KEY_UP=4,JO_KEY_DOWN=8,
      JO_KEY_A=16,JO_KEY_B=32,JO_KEY_C=64,JO_KEY_START=128};
unsigned jo_core_get_vblank_count(void);
int jo_get_player_input_port(int player);
bool jo_is_input_key_pressed(unsigned port,unsigned key);
#endif
