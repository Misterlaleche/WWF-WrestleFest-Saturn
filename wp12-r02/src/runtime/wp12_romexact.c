#include "wp12_romexact.h"

#define ROM_AE20  0x0000AE20u
#define ROM_264E2 0x000264E2u
#define ROM_7B70  0x00007B70u
#define ROM_C98   0x00000C98u
#define ROM_98BA  0x000098BAu
#define ROM_2AEA  0x00002AEAu
#define ROM_247C  0x0000247Cu
#define ROM_2836  0x00002836u
#define ROM_2B58  0x00002B58u
#define ROM_9A42  0x00009A42u
#define ROM_7506  0x00007506u
#define ROM_2A06  0x00002A06u
#define ROM_8AE6  0x00008AE6u
#define ROM_F8C   0x00000F8Cu

static const uint16_t tag_primary_xy[4][2] = {
    {0x0240u,0x0150u},{0x0220u,0x0160u},{0x02C0u,0x0150u},{0x02E0u,0x0160u}
};

static const uint16_t royal_xy[6][2] = {
    {0x0240u,0x0120u},{0x02D0u,0x0120u},{0x0230u,0x0150u},
    {0x02C0u,0x0150u},{0x0240u,0x0180u},{0x02B0u,0x0180u}
};
static const uint16_t royal_orientation[6] = {0x8000u,0u,0x8000u,0u,0x8000u,0u};

static const uint16_t identity_threshold[12] = {
    0x0040u,0x0080u,0x0080u,0x0080u,0x0080u,0x0080u,
    0x0080u,0x0080u,0x0040u,0x0100u,0x0040u,0x0040u
};
static const uint8_t identity_voice_low[12] = {
    0x6Du,0x6Eu,0x6Fu,0x74u,0x73u,0x73u,0x71u,0x70u,0x72u,0x75u,0x73u,0x73u
};
static const uint16_t final_audio[10] = {
    0x3104u,0x310Au,0x310Cu,0x310Fu,0x3106u,
    0x3104u,0x310Au,0x310Cu,0x310Fu,0x3106u
};
static const uint8_t aaea_audio_low[24] = {
    0x00u,0x00u,0x7Bu,0x00u,0xFFu,0x00u,0x7Cu,0x00u,
    0xFFu,0x00u,0x00u,0x7Du,0x00u,0xFFu,0x00u,0x7Cu,
    0x00u,0xFFu,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u
};
static const uint16_t aaea_fixed_threshold[24] = {
    0x0008u,0x0008u,0x00A0u,0x0008u,0x0000u,0x0008u,0x0014u,0x0008u,
    0x0000u,0x0008u,0x0008u,0x0028u,0x0008u,0x0000u,0x0008u,0x0014u,
    0x0008u,0x0000u,0x0008u,0x0008u,0x0008u,0x0008u,0x0008u,0x0008u
};

/* Raw $AD6C/$AD9C programs. Values in local_threshold are ROM thresholds;
 * dwell is threshold+1 callbacks. */
static const uint8_t local_count[12] = {11u,4u,4u,2u,2u,2u,2u,4u,2u,6u,3u,2u};
static const uint16_t local_selector[12][11] = {
 {0x005B,0x005A,0x0059,0x005B,0x005A,0x0059,0x005B,0x005A,0x0059,0x005C,0x005D},
 {0x0059,0x005A,0x0059,0x005A},
 {0x0059,0x005A,0x005B,0x005A},
 {0x0059,0x005A},
 {0x005B,0x005A},
 {0x005B,0x005A},
 {0x0059,0x005A},
 {0x0059,0x005A,0x005B,0x005A},
 {0x0059,0x005A},
 {0x0059,0x005A,0x0059,0x005A,0x0059,0x005A},
 {0x0059,0x005A,0x005B},
 {0x0059,0x005A}
};
static const uint16_t local_threshold[12][11] = {
 {6,6,6,6,6,6,6,6,6,6,32},
 {16,16,16,16},
 {8,8,8,16},
 {16,32},
 {89,91},
 {89,91},
 {16,32},
 {8,8,8,16},
 {91,8},
 {8,8,8,8,8,16},
 {8,16,16},
 {16,32}
};

void wp12_batch_clear(wp12_batch_t *batch)
{
    if (batch != NULL) { batch->count = 0u; }
}

static bool push(wp12_batch_t *batch, wp12_op_kind_t kind, uint32_t addr, uint32_t arg, uint8_t actor)
{
    wp12_op_t *op;
    if (batch == NULL || batch->count >= WP12_MAX_OPS) { return false; }
    op = &batch->ops[batch->count++];
    op->kind = kind; op->rom_address = addr; op->arg = arg; op->actor_index = actor;
    return true;
}

static wp12_status_t overflow(wp12_ctx_t *ctx)
{
    if (ctx != NULL) { ctx->phase = WP12_PHASE_BLOCKED; ctx->blocked_reason = WP12_BLOCKED_OP_OVERFLOW; }
    return WP12_BLOCKED_OP_OVERFLOW;
}

uint8_t wp12_identity57(const wp12_wrestler_t *actor)
{
    return actor == NULL ? 0u : (uint8_t)(actor->identity_word_plus56 & 0x00FFu);
}

uint16_t wp12_tag_identity_window_threshold(uint8_t id)
{
    return id < 12u ? identity_threshold[id] : 0u;
}

uint16_t wp12_tag_identity_voice_command(uint8_t id)
{
    return id < 12u ? (uint16_t)(0x3100u | identity_voice_low[id]) : 0u;
}

uint16_t wp12_tag_activation_command(uint8_t id)
{
    return id < 12u ? (id < 5u ? 0x3032u : 0x3031u) : 0u;
}

uint16_t wp12_final_audio_command(uint8_t idx)
{
    return idx < 10u ? final_audio[idx] : 0u;
}

static bool is_aaea(uint8_t st)
{
    return st==0x02u || st==0x04u || st==0x06u || st==0x08u || st==0x0Bu ||
           st==0x0Du || st==0x0Fu || st==0x11u;
}

static bool is_a814(uint8_t st)
{
    return st==0x05u || st==0x09u || st==0x0Eu || st==0x12u;
}

static bool is_a8c0(uint8_t st)
{
    return st==0x07u || st==0x0Au || st==0x0Cu || st==0x10u;
}

static int logical_slot_for_state(uint8_t st)
{
    if (st==0x04u || st==0x05u) return 0;
    if (st==0x08u || st==0x09u) return 1;
    if (st==0x0Du || st==0x0Eu) return 2;
    if (st==0x11u || st==0x12u) return 3;
    return -1;
}

int wp12_resolve_tag_actor_index(const wp12_ctx_t *ctx, uint8_t st)
{
    static const uint8_t primary[4] = {0u,1u,2u,3u};
    static const uint8_t fallback[4] = {4u,5u,4u,5u};
    int l = logical_slot_for_state(st);
    uint8_t p;
    if (ctx == NULL || l < 0) return -1;
    p = primary[(unsigned)l];
    if (ctx->wrestlers[p].active) return (int)p;
    /* ROM chooses fallback when primary is inactive; no fallback-active test. */
    return (int)fallback[(unsigned)l];
}

uint16_t wp12_presenter_y_machine_floor(const wp12_ctx_t *ctx)
{
    return ctx == NULL ? 0u : (uint16_t)(ctx->presenter.y_half_units >> 1);
}

static bool setup_referee_ok(const wp12_ctx_t *ctx)
{
    const wp12_referee_t *r = &ctx->referee;
    return r->active && r->type_plus02==0x000Cu && r->x_plus06==0x0280u &&
           r->y_plus0a==0x0198u && r->z_plus0e==0x0140u &&
           r->primary_plus20==0x8000u && r->plus34==0u && ctx->global_1c1697==1u;
}

static bool setup_tag_ok(const wp12_ctx_t *ctx)
{
    unsigned i;
    for (i=0u;i<4u;i++) {
        if (ctx->wrestlers[i].x_plus06 != tag_primary_xy[i][0] ||
            ctx->wrestlers[i].y_plus0a != tag_primary_xy[i][1] ||
            ctx->wrestlers[i].z_plus0e != 0x0140u) return false;
    }
    return true;
}

static bool setup_royal_ok(const wp12_ctx_t *ctx)
{
    unsigned i;
    for (i=0u;i<6u;i++) {
        const wp12_wrestler_t *a=&ctx->wrestlers[i];
        if (a->orientation_plus2e != royal_orientation[i] || a->draw_plus04 != royal_orientation[i] ||
            a->x_plus06 != royal_xy[i][0] || a->y_plus0a != royal_xy[i][1] ||
            a->z_plus0e != 0x0140u || a->plus32 != 1u || a->plus34 != 0u ||
            a->primary_plus20 != 0u || a->plusb6 != 0x8000u) return false;
    }
    return true;
}

wp12_status_t wp12_begin_paid_route(wp12_ctx_t *ctx, wp12_batch_t *batch)
{
    unsigned i;
    if (ctx == NULL || batch == NULL) return WP12_BLOCKED_BAD_SETUP;
    wp12_batch_clear(batch);
    ctx->blocked_reason = WP12_OK;
    ctx->global_1c0166 = 0u; /* $BD2 */
    if (!push(batch,WP12_OP_AUDIO_RAW,0x00008EB6u,0x3100u,0xFFu) ||
        !push(batch,WP12_OP_AUDIO_RAW,0x00008EB6u,0x3120u,0xFFu)) return overflow(ctx);
    if (ctx->global_1c0163==0x04u || ctx->global_1c0163==0x09u) {
        if (!push(batch,WP12_OP_CALL_AE20,ROM_AE20,0u,0xFFu) ||
            !push(batch,WP12_OP_CALL_264E2,ROM_264E2,0u,0xFFu)) return overflow(ctx);
    }
    if (!push(batch,WP12_OP_CALL_7B70,ROM_7B70,0u,0xFFu)) return overflow(ctx);

    /* Exact inline $C08..$C60 TAG/cabinet block. */
    if (ctx->mode == WP12_MODE_TAG && (ctx->global_1c0066 & 0x08u) != 0u) {
        ctx->global_1c0160 &= (uint16_t)~0x0040u;
        if (ctx->wrestlers[0].active && ctx->wrestlers[2].active) {
            ctx->global_1c0160 |= 0x0040u;
            for (i=0u;i<4u;i++) ctx->wrestlers[i].plus104 = 0x8000u;
        }
    }

    if (!push(batch,WP12_OP_CALL_C98,ROM_C98,0u,0xFFu)) return overflow(ctx);
    ctx->phase = WP12_PHASE_NEED_COMMON_SETUP;
    return WP12_NEED_COMMON_SETUP;
}

wp12_status_t wp12_confirm_common_setup(wp12_ctx_t *ctx,
                                         bool c98_equivalent_executed,
                                         bool referee_resource_bundle_prepared)
{
    if (ctx == NULL || ctx->phase != WP12_PHASE_NEED_COMMON_SETUP) return WP12_BLOCKED_BAD_SETUP;
    /* Saturn platform-adaptation proof flags: the caller confirms it actually
     * executed the canonical $C98 equivalent and the $10718 referee resource
     * prepare bundle (0C,07,0E,1D,01).  They are not arcade globals. */
    if (!c98_equivalent_executed || !referee_resource_bundle_prepared ||
        !setup_referee_ok(ctx) || (ctx->mode==WP12_MODE_TAG ? !setup_tag_ok(ctx) : !setup_royal_ok(ctx))) {
        ctx->phase = WP12_PHASE_BLOCKED;
        ctx->blocked_reason = WP12_BLOCKED_BAD_SETUP;
        return WP12_BLOCKED_BAD_SETUP;
    }
    ctx->phase = WP12_PHASE_VISIBLE;
    ctx->visible_entry_done = false;
    ctx->visible_updates = 0u;
    return WP12_OK;
}

static wp12_status_t validate_identity(wp12_ctx_t *ctx, uint8_t id)
{
    if (id < 12u) return WP12_OK;
    ctx->phase = WP12_PHASE_BLOCKED;
    ctx->blocked_reason = WP12_BLOCKED_BAD_IDENTITY;
    return WP12_BLOCKED_BAD_IDENTITY;
}

static wp12_status_t presenter_a9e2(wp12_ctx_t *ctx, wp12_batch_t *batch)
{
    wp12_presenter_t *p=&ctx->presenter;
    uint16_t threshold;
    if (!p->init_latch) {
        p->init_latch=true; p->flags_plus00=0x8001u; p->active=true; p->type_plus02=0x001Fu;
        p->x_plus06=0x0280u; p->y_half_units=(uint16_t)(0x0178u*2u); p->z_plus0e=0x0140u;
        p->counter_plus22=0u;
        if (!push(batch,WP12_OP_RESOURCE_PREP_2AEA,ROM_2AEA,0x001Fu,0xFFu)) return overflow(ctx);
    }
    p->draw_plus04=0x0002u;
    p->counter_plus22++;
    threshold = (p->state_plus1d==0x00u)?0x0080u:0x0020u;
    if (p->counter_plus22 > threshold) {
        p->init_latch=false; p->state_plus1d++;
        if (p->state_plus1d==0x01u) {
            if (!push(batch,WP12_OP_CALL_9A42,ROM_9A42,0u,0xFFu)) return overflow(ctx);
            if (ctx->global_1c0163 != 0u) p->state_plus1d=0x18u;
        }
    }
    return WP12_OK;
}

static wp12_status_t presenter_aa96(wp12_ctx_t *ctx)
{
    wp12_presenter_t *p=&ctx->presenter;
    uint16_t threshold;
    if (!p->init_latch) { p->init_latch=true; p->counter_plus22=0u; p->phase_plus24=0u; }
    p->draw_plus04=(uint16_t)(0x0002u + p->phase_plus24);
    threshold = p->phase_plus24==0u ? 0x0010u : 0x0020u;
    p->counter_plus22++;
    if (p->counter_plus22 > threshold) {
        p->counter_plus22=0u; p->phase_plus24++;
        if (p->phase_plus24 >= 2u) { p->init_latch=false; p->state_plus1d++; }
    }
    return WP12_OK;
}

static wp12_status_t presenter_aaea(wp12_ctx_t *ctx, wp12_batch_t *batch, uint8_t rng_nibble)
{
    wp12_presenter_t *p=&ctx->presenter;
    uint16_t threshold;
    uint8_t raw;
    if (!p->init_latch) {
        int idx;
        p->init_latch=true; p->counter_plus22=0u; p->phase_plus24=0u; p->aux_plus1e=0u;
        raw = aaea_audio_low[p->state_plus1d];
        if (raw != 0u && raw != 0xFFu) {
            if (!push(batch,WP12_OP_AUDIO_RAW,0x00002052u,(uint16_t)(0x3100u|raw),0xFFu)) return overflow(ctx);
        } else {
            idx=wp12_resolve_tag_actor_index(ctx,p->state_plus1d);
            if (idx < 0) return validate_identity(ctx,0xFFu);
            raw=wp12_identity57(&ctx->wrestlers[(unsigned)idx]);
            if (validate_identity(ctx,raw)!=WP12_OK) return ctx->blocked_reason;
            raw=identity_voice_low[raw];
            if (!push(batch,WP12_OP_AUDIO_RAW,0x00002052u,(uint16_t)(0x3100u|raw),(uint8_t)idx)) return overflow(ctx);
            ctx->global_1c0168=0u;
            if (raw==0x73u) ctx->global_1c0168=0x8000u;
        }
    }

    /* $AAEA writes the current toggle to low draw byte before countdown/RNG. */
    p->draw_plus04=(uint16_t)((p->draw_plus04 & 0xFF00u) | (uint16_t)p->toggle_plus25);
    if (p->counter_plus22==0u) {
        p->counter_plus22=(uint16_t)(6u + (uint16_t)(rng_nibble & 0x0Fu));
        p->toggle_plus25 ^= 1u;
    } else {
        p->counter_plus22--;
    }
    p->aux_plus1e++;

    threshold=aaea_fixed_threshold[p->state_plus1d];
    if (threshold==0u) {
        int idx=wp12_resolve_tag_actor_index(ctx,p->state_plus1d);
        uint8_t id;
        if (idx < 0) return validate_identity(ctx,0xFFu);
        id=wp12_identity57(&ctx->wrestlers[(unsigned)idx]);
        if (validate_identity(ctx,id)!=WP12_OK) return ctx->blocked_reason;
        threshold=identity_threshold[id];
    }
    if (p->aux_plus1e > threshold) { p->init_latch=false; p->state_plus1d++; }
    return WP12_OK;
}

static wp12_status_t presenter_a814(wp12_ctx_t *ctx, wp12_batch_t *batch)
{
    wp12_presenter_t *p=&ctx->presenter;
    if (!p->init_latch) {
        int idx=wp12_resolve_tag_actor_index(ctx,p->state_plus1d);
        uint8_t id;
        if (idx < 0) return validate_identity(ctx,0xFFu);
        id=wp12_identity57(&ctx->wrestlers[(unsigned)idx]);
        if (validate_identity(ctx,id)!=WP12_OK) return ctx->blocked_reason;
        p->init_latch=true;
        ctx->wrestlers[(unsigned)idx].local_word_plus1c =
            (uint16_t)((ctx->wrestlers[(unsigned)idx].local_word_plus1c & 0xFF00u) | 0x0001u);
        if (!push(batch,WP12_OP_AUDIO_RAW,0x00002052u,wp12_tag_activation_command(id),(uint8_t)idx)) return overflow(ctx);
        if (id==0x0Au && (unsigned)idx+1u < WP12_WRESTLER_RECORDS) {
            ctx->wrestlers[(unsigned)idx+1u].local_word_plus1c =
                (uint16_t)((ctx->wrestlers[(unsigned)idx+1u].local_word_plus1c & 0xFF00u) | 0x0001u);
        }
        p->counter_plus22=0u;
        p->draw_plus04=0x000Cu;
    }
    if (p->state_plus1d >= 0x0Bu) p->draw_plus04 |= 0x8000u;
    p->counter_plus22++;
    if (p->counter_plus22 >= 0x0040u) {
        if (ctx->global_1c0168 != 0u) p->state_plus1d=(uint8_t)(p->state_plus1d+4u);
        p->init_latch=false;
        p->state_plus1d++;
    }
    return WP12_OK;
}

static void presenter_a8c0(wp12_presenter_t *p)
{
    p->init_latch=false; p->state_plus1d++;
}

static void presenter_a8ec(wp12_presenter_t *p)
{
    if (!p->init_latch) { p->init_latch=true; p->counter_plus22=0u; p->phase_plus24=0u; }
    p->y_half_units++;
    p->draw_plus04=(uint16_t)(0x0008u+p->phase_plus24);
    p->counter_plus22++;
    if (p->counter_plus22 >= 0x000Cu) {
        p->counter_plus22=0u; p->phase_plus24++;
        if (p->phase_plus24 >= 4u) p->phase_plus24=0u;
    }
    if ((p->y_half_units>>1) >= 0x01A7u) { p->init_latch=false; p->state_plus1d++; }
}

static void presenter_a952(wp12_presenter_t *p)
{
    if (!p->init_latch) { p->init_latch=true; p->counter_plus22=0u; p->phase_plus24=0u; }
    p->draw_plus04=(uint16_t)(0x0004u+p->phase_plus24);
    p->counter_plus22++;
    if (p->counter_plus22 >= 0x000Cu) {
        p->counter_plus22=0u; p->phase_plus24++;
        if (p->phase_plus24 >= 4u) { p->init_latch=false; p->state_plus1d++; }
    }
}

static void presenter_a99a(wp12_presenter_t *p)
{
    if (!p->init_latch) { p->init_latch=true; p->counter_plus22=0u; p->phase_plus24=0u; }
    p->draw_plus04=(uint16_t)(0x000Du+p->phase_plus24);
    p->counter_plus22++;
    if (p->counter_plus22 >= 0x000Cu) {
        p->counter_plus22=0u; p->phase_plus24++;
        if (p->phase_plus24 >= 4u) { p->init_latch=false; p->state_plus1d++; }
    }
}

static bool presenter_needs_rng(const wp12_ctx_t *ctx)
{
    const wp12_presenter_t *p=&ctx->presenter;
    return ctx->mode==WP12_MODE_TAG && p->state_plus1d<0x18u && is_aaea(p->state_plus1d) &&
           (!p->init_latch || p->counter_plus22==0u);
}

static wp12_status_t presenter_step(wp12_ctx_t *ctx, wp12_batch_t *batch, uint8_t rng)
{
    wp12_presenter_t *p=&ctx->presenter;
    uint8_t st=p->state_plus1d;
    wp12_status_t s=WP12_OK;
    if (st==0x00u || st==0x03u || st==0x13u) s=presenter_a9e2(ctx,batch);
    else if (st==0x01u || st==0x14u) s=presenter_aa96(ctx);
    else if (is_aaea(st)) s=presenter_aaea(ctx,batch,rng);
    else if (is_a814(st)) s=presenter_a814(ctx,batch);
    else if (is_a8c0(st)) presenter_a8c0(p);
    else if (st==0x15u) presenter_a8ec(p);
    else if (st==0x16u) presenter_a952(p);
    else if (st==0x17u) presenter_a99a(p);
    else if (st>=0x18u) return WP12_OK;
    else { ctx->phase=WP12_PHASE_BLOCKED; ctx->blocked_reason=WP12_BLOCKED_BAD_SETUP; return WP12_BLOCKED_BAD_SETUP; }
    if (s!=WP12_OK) return s;

    /* Common $A6A2..$A6BA post-callback word semantics. */
    if (p->state_plus1d >= 0x0Bu) p->draw_plus04 |= 0x8000u;
    if (ctx->global_1c0163 != 0u) p->draw_plus04=0xFFFFu;
    return WP12_OK;
}

static wp12_status_t wrestler_step(wp12_ctx_t *ctx, unsigned idx, wp12_batch_t *batch)
{
    wp12_wrestler_t *a=&ctx->wrestlers[idx];
    uint8_t local=(uint8_t)(a->local_word_plus1c & 0x00FFu);
    uint8_t id;
    uint16_t threshold;
    if (local==0u) {
        if (ctx->mode==WP12_MODE_ROYAL) {
            a->counter_plus22++;
            if (a->counter_plus22 >= 0x0040u) {
                ctx->presenter.state_plus1d=0x18u;
                if (!push(batch,WP12_OP_AUDIO_RAW,0x00002052u,0x312Fu,(uint8_t)idx)) return overflow(ctx);
                return WP12_OK;
            }
        }
        a->draw_plus04=0u;
        if ((a->plus33 & 0x80u)!=0u) a->draw_plus04|=0x8000u;
        if (ctx->mode==WP12_MODE_TAG) { a->counter_plus22=0u; a->phase_plus24=0u; }
        return WP12_OK;
    }

    id=wp12_identity57(a);
    if (validate_identity(ctx,id)!=WP12_OK) return ctx->blocked_reason;
    if (a->phase_plus24 >= local_count[id]) {
        ctx->phase=WP12_PHASE_BLOCKED; ctx->blocked_reason=WP12_BLOCKED_BAD_SETUP;
        return WP12_BLOCKED_BAD_SETUP;
    }
    a->draw_plus04=local_selector[id][a->phase_plus24];
    a->counter_plus22++;
    threshold=local_threshold[id][a->phase_plus24];
    if (a->counter_plus22 > threshold) {
        a->counter_plus22=0u;
        a->phase_plus24++;
        if (a->phase_plus24 >= local_count[id]) a->local_word_plus1c=0u; /* $AD66 CLR.W +$1C */
    }
    return WP12_OK;
}

static wp12_status_t append_exit_and_handoff(wp12_ctx_t *ctx, wp12_batch_t *batch)
{
    uint16_t cmd=wp12_final_audio_command(ctx->global_1c0162);
    if (cmd==0u) {
        ctx->phase=WP12_PHASE_BLOCKED; ctx->blocked_reason=WP12_BLOCKED_BAD_FINAL_AUDIO_INDEX;
        return WP12_BLOCKED_BAD_FINAL_AUDIO_INDEX;
    }
    if (!push(batch,WP12_OP_RESOURCE_TEARDOWN_2B58,ROM_2B58,0x001Fu,0xFFu) ||
        !push(batch,WP12_OP_CALL_9A42,ROM_9A42,0u,0xFFu) ||
        !push(batch,WP12_OP_CALL_7506,ROM_7506,0u,0xFFu)) return overflow(ctx);
    ctx->presenter.flags_plus00=0u; ctx->presenter.active=false; /* $A78E CLR.W $1C14CE only */
    ctx->global_1c15fc=1u;
    if (!push(batch,WP12_OP_CALL_2A06,ROM_2A06,0u,0xFFu) ||
        !push(batch,WP12_OP_AUDIO_RAW,0x00008EB6u,0x3100u,0xFFu) ||
        !push(batch,WP12_OP_AUDIO_RAW,0x00008EB6u,0x3120u,0xFFu) ||
        !push(batch,WP12_OP_AUDIO_RAW,0x00002052u,0x312Fu,0xFFu)) return overflow(ctx);

    /* Back in live $BD2 after $A654. */
    ctx->global_1c15fc=1u;
    if (!push(batch,WP12_OP_CALL_2A06,ROM_2A06,0u,0xFFu) ||
        !push(batch,WP12_OP_AUDIO_RAW,0x00002052u,cmd,0xFFu) ||
        !push(batch,WP12_OP_CALL_8AE6,ROM_8AE6,0u,0xFFu) ||
        !push(batch,WP12_OP_HANDOFF_F8C,ROM_F8C,0u,0xFFu)) return overflow(ctx);
    ctx->phase=WP12_PHASE_HANDOFF;
    return WP12_OK;
}

wp12_status_t wp12_visible_iteration(wp12_ctx_t *ctx,
                                     const wp12_iteration_input_t *input,
                                     wp12_batch_t *batch)
{
    unsigned i;
    wp12_status_t s;
    bool exit_now;
    uint8_t rng=0u;
    if (ctx==NULL || input==NULL || batch==NULL) return WP12_BLOCKED_BAD_SETUP;
    wp12_batch_clear(batch);
    if (ctx->phase==WP12_PHASE_HANDOFF) return WP12_ALREADY_HANDOFF;
    if (ctx->phase!=WP12_PHASE_VISIBLE) return WP12_BLOCKED_BAD_SETUP;

    if (presenter_needs_rng(ctx)) {
        if (!input->rng_21b4_valid) return WP12_NEED_RNG_21B4;
        rng=(uint8_t)(input->rng_21b4_low_nibble & 0x0Fu);
    }

    if (!ctx->visible_entry_done) {
        /* $A654: when global $1C0163 == 0, referee X receives +$0080 once. */
        if (ctx->global_1c0163==0u) ctx->referee.x_plus06=(uint16_t)(ctx->referee.x_plus06+0x0080u);
        if (ctx->mode==WP12_MODE_TAG && !push(batch,WP12_OP_CALL_98BA,ROM_98BA,0u,0xFFu)) return overflow(ctx);
        if (!push(batch,WP12_OP_AUDIO_RAW,0x00002052u,0x3132u,0xFFu)) return overflow(ctx);
        ctx->visible_entry_done=true;
    }

    if (ctx->mode==WP12_MODE_TAG) {
        s=presenter_step(ctx,batch,rng); if (s!=WP12_OK) return s;
        if (!push(batch,WP12_OP_RENDER_PRESENTER,ROM_247C,ctx->presenter.draw_plus04,0xFFu)) return overflow(ctx);
    }

    for (i=0u;i<WP12_WRESTLER_RECORDS;i++) {
        if (!ctx->wrestlers[i].active) continue;
        s=wrestler_step(ctx,i,batch); if (s!=WP12_OK) return s;
        if (!push(batch,WP12_OP_RENDER_WRESTLER,ROM_247C,ctx->wrestlers[i].draw_plus04,(uint8_t)i)) return overflow(ctx);
    }
    if (!push(batch,WP12_OP_RENDER_REFEREE,ROM_247C,ctx->referee.draw_plus04,0xFFu) ||
        !push(batch,WP12_OP_VIDEO_SYNC_2836,ROM_2836,0u,0xFFu)) return overflow(ctx);

    ctx->visible_updates++;
    /* Input test occurs only after callback/render/sync. */
    exit_now=input->action_mask_0030_any_port || ctx->presenter.state_plus1d>=0x18u;
    if (exit_now) return append_exit_and_handoff(ctx,batch);
    return WP12_OK;
}
