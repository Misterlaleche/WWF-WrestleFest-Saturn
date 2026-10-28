#ifndef WF24_LIVEVM_ADAPTER_H
#define WF24_LIVEVM_ADAPTER_H
#include <stdbool.h>
#include <stdint.h>
#include "wf_audio_live_vm.h"
typedef struct { wf_audio_live_vm_t *vm; } wf24_livevm_adapter_t;
bool wf24_livevm_render_be(void *dst, uint32_t bytes, void *user);
bool wf24_livevm_send(wf24_livevm_adapter_t *a, uint8_t command);
#endif
