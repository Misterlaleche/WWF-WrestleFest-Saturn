#ifndef WF_STATE5_H
#define WF_STATE5_H
#include <stdint.h>
typedef struct { uint32_t raw_entry; uint32_t callback_rom; uint8_t forced_bit31; } wf_state5_map_t;
extern const wf_state5_map_t wf_state5_map[143];
static inline const wf_state5_map_t *wf_state5_lookup(uint8_t selector){return selector<143u?&wf_state5_map[selector]:0;}
#endif
