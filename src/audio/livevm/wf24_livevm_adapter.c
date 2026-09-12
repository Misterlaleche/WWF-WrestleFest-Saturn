#include "wf24_livevm_adapter.h"
bool wf24_livevm_render_be(void *dst, uint32_t bytes, void *user){wf24_livevm_adapter_t*a=user;if(!a||!a->vm||!dst||(bytes&1u))return false;return wf_audio_live_vm_render_be(a->vm,(uint8_t*)dst,bytes/2u);}
bool wf24_livevm_send(wf24_livevm_adapter_t *a,uint8_t command){if(!a||!a->vm)return false;if(command>=1u&&command<=0x12u)return wf_audio_live_vm_start(a->vm,command);if(command==0x1Fu){a->vm->latch=command;z80_gen_nmi(&a->vm->cpu);return true;}return false;}
