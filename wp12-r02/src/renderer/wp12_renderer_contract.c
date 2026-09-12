#include "wp12_renderer_contract.h"

bool wp12_renderer_item_from_op(const wp12_ctx_t *ctx,
                                 const wp12_op_t *op,
                                 wp12_render_item_t *out)
{
    if (ctx==0 || op==0 || out==0) return false;
    if (op->kind==WP12_OP_RENDER_WRESTLER && op->actor_index<WP12_WRESTLER_RECORDS) {
        const wp12_wrestler_t *a=&ctx->wrestlers[op->actor_index];
        out->render_class=WP12_RENDER_WRESTLER; out->actor_index=op->actor_index;
        out->source_set_or_type=a->type_plus02; out->machine_x=a->x_plus06; out->machine_y=a->y_plus0a; out->machine_z=a->z_plus0e;
        out->selector_word=a->draw_plus04; return true;
    }
    if (op->kind==WP12_OP_RENDER_PRESENTER) {
        out->render_class=WP12_RENDER_PRESENTER_SET_1F; out->actor_index=0xFFu;
        out->source_set_or_type=0x001Fu; out->machine_x=ctx->presenter.x_plus06; out->machine_y=wp12_presenter_y_machine_floor(ctx);
        out->machine_z=ctx->presenter.z_plus0e; out->selector_word=ctx->presenter.draw_plus04; return true;
    }
    if (op->kind==WP12_OP_RENDER_REFEREE) {
        out->render_class=WP12_RENDER_REFEREE_TYPE_0C; out->actor_index=0xFFu;
        out->source_set_or_type=0x000Cu; out->machine_x=ctx->referee.x_plus06; out->machine_y=ctx->referee.y_plus0a;
        out->machine_z=ctx->referee.z_plus0e; out->selector_word=ctx->referee.draw_plus04; return true;
    }
    return false;
}
