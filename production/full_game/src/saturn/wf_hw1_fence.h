#ifndef WF_HW1_FENCE_H
#define WF_HW1_FENCE_H
#include <stdbool.h>
bool wf_hw1_fence_begin(void);
bool wf_hw1_fence_check(void);
void wf_hw1_fence_publish(void);
#endif
