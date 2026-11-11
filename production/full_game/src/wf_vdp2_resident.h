#ifndef WF_VDP2_RESIDENT_H
#define WF_VDP2_RESIDENT_H
#include <stddef.h>
#include <stdint.h>
/* The recovered R3 VDP2 gaps only. VDP1 remains wholly owned by HW1. */
typedef int (*wf_vdp2_resident_reader_t)(const char*,uint32_t,void*,size_t);
void wf_vdp2_resident_reset(void);
int wf_vdp2_resident_bind(unsigned ring_tiles);
size_t wf_vdp2_resident_capacity(void);
int wf_vdp2_resident_load(wf_vdp2_resident_reader_t,const char*,uint32_t,size_t,void*,size_t);
int wf_vdp2_resident_read(uint32_t,void*,size_t);
#endif
