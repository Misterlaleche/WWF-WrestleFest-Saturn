#ifndef WP2015_DISPLAY_PIPELINE_H
#define WP2015_DISPLAY_PIPELINE_H
#include <stddef.h>
#include <stdint.h>

typedef struct {uint16_t cell,value;} wp2015_name_write_t;

/* All map/CRAM writes preserve the logical arcade image. During combat,
 * scanout and the already submitted VDP1 image own separate name banks.
 * Only the third bank is writable. Outside combat these are direct stores. */
#if defined(WF_PLATFORM_SATURN) || defined(__sh__)
void wp2015_display_name_write(volatile uint16_t *dest,uint16_t value);
/* Same ordered writes as name_write(base+cell,value), including error prefixes. */
void wp2015_display_names_write(volatile uint16_t *base,const wp2015_name_write_t *writes,size_t count);
void wp2015_display_cram_write(uint16_t entry,uint32_t rgb888);
void wp2015_display_scroll(unsigned layer,unsigned x,unsigned y);
void wp2015_display_priority(uint8_t priority,unsigned show);
void wp2015_display_reset(void);
int wp2015_display_begin(unsigned first_slot);
int wp2015_display_seal(unsigned slot,uint32_t generation);
void wp2015_display_vblank_in(unsigned draw_slot,uint32_t draw_generation);
void wp2015_display_vblank_out(void);
int wp2015_display_prepare(unsigned slot);
void wp2015_display_stop(void);
int wp2015_display_active(void);
uint32_t wp2015_display_visible_generation(void);
/* sealed, displayed, restored names, changed names, colors committed,
 * maximum colors/IRQ, maximum FRT ticks/IRQ, IRQ late, ownership faults,
 * IRQ end outside VBlank, maximum IRQ ending VCOUNT, currently scanned bank. */
extern volatile uint32_t wp2015_display_observer[12];
#else
static inline void wp2015_display_name_write(volatile uint16_t *dest,uint16_t value){*dest=value;}
static inline void wp2015_display_names_write(volatile uint16_t *base,const wp2015_name_write_t *writes,size_t count){for(size_t i=0;i<count;i++)wp2015_display_name_write(base+writes[i].cell,writes[i].value);}
#endif
#endif
