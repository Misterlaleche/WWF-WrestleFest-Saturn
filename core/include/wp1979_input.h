#ifndef WP1979_INPUT_H
#define WP1979_INPUT_H
#include <stdint.h>
uint8_t wp1979_input(uint32_t frame,uint8_t physical);
extern volatile uint32_t wp1979_demo_active;
#endif
