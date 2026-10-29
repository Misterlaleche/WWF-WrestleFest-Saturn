#include <yaul.h>
#include "wp0_timer.h"
uint16_t wp0_timer_count_get(void) { return cpu_frt_count_get(); }
