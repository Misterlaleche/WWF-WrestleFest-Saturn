#ifndef WF_STARTUP_NOTICE_H
#define WF_STARTUP_NOTICE_H
#include <stdbool.h>
#include <stdint.h>
const char *wf_startup_notice_message(uint32_t id,uint32_t bytes);
void wf_startup_notice_draw(volatile uint16_t *bitmap,const char *message);
bool wf_startup_notice_begin(uint32_t id,uint32_t bytes);
void wf_startup_notice_end(void);
extern volatile uint32_t wf_startup_notice_active,wf_startup_notice_fields;
#endif
