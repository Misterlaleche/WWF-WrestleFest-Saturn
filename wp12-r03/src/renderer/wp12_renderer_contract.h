#ifndef WP12_RENDERER_CONTRACT_H
#define WP12_RENDERER_CONTRACT_H
#include <stdint.h>
#include <stdbool.h>
#include "../runtime/wp12_romexact.h"

typedef enum {
    WP12_RENDER_WRESTLER = 0,
    WP12_RENDER_PRESENTER_SET_1F,
    WP12_RENDER_REFEREE_TYPE_0C
} wp12_render_class_t;

typedef struct {
    wp12_render_class_t render_class;
    uint8_t actor_index;
    uint16_t source_set_or_type;
    uint16_t machine_x;
    uint16_t machine_y;
    uint16_t machine_z;
    uint16_t selector_word;
} wp12_render_item_t;

/* Produces only ROM-authoritative machine-space inputs.  Camera/raster mapping,
 * VDP1/VDP2 placement and priorities remain owned by the canonical renderer. */
bool wp12_renderer_item_from_op(const wp12_ctx_t *ctx,
                                 const wp12_op_t *op,
                                 wp12_render_item_t *out);
#endif
