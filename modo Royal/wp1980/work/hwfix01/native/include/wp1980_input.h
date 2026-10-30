#ifndef WP1980_INPUT_H
#define WP1980_INPUT_H

#include "wf_runtime.h"

/* Launch policy drives the original coin/START/mode/roster owners solely
 * through their ordinary cabinet ports. It never changes arcade RAM. */
void wp1980_frontend_input_begin(uint32_t clock);
void wp1980_frontend_input_disable(void);
uint16_t wp1980_frontend_input(unsigned phase,const uint8_t *ram,
                              uint32_t clock,unsigned port,uint16_t physical);

/* Optional player-one demonstration. Any physical gameplay input permanently
 * returns control to the user. All opponents remain the original native AI. */
uint8_t wp1980_input(const wf_runtime_t *runtime,uint8_t physical);
extern volatile uint32_t wp1980_demo_active;
extern volatile uint32_t wp1980_launch_stage;

#endif
