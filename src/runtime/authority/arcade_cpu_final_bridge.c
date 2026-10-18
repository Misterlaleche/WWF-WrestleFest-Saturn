#include "arcade_cpu_final_bridge.h"

static const uint32_t cpu_main[13]={
 0x0001c2ccU,
 0x0001c8f4U,
 0x0001d7b6U,
 0x0001da66U,
 0x0001da7eU,
 0x0001dfe2U,
 0x0001edc4U,
 0x0001ee96U,
 0x0001ee98U,
 0x0001eea0U,
 0x0001ee9eU,
 0x0001efaaU,
 0x0001f06aU
};

static const uint32_t cpu_state1[13]={
 0x0001c93eU,
 0x0001d398U,
 0x0001d524U,
 0x0001d524U,
 0x0001d1f0U,
 0x0001d078U,
 0x0001d106U,
 0x0001d16aU,
 0x0001d206U,
 0x0001d31eU,
 0x0001d5a2U,
 0x0001d74aU,
 0x0001c93cU
};

static const uint32_t cpu_exec[143]={
 0x0001e2f6U,
 0x0001e6feU,
 0x0001e254U,
 0x0001e70eU,
 0x0001e532U,
 0x0001e53aU,
 0x0001e74eU,
 0x0001e73eU,
 0x0001e5f6U,
 0x0001e5aeU,
 0x0001e664U,
 0x0001e664U,
 0x0001e6e2U,
 0x0001e6e2U,
 0x0001e6e2U,
 0x0001e6dcU,
 0x0001e62cU,
 0x0001e6e2U,
 0x0001e6e2U,
 0x0001e62cU,
 0x0001e62cU,
 0x8001e83aU,
 0x8001ea2eU,
 0x0001e59aU,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e6e2U,
 0x0001e792U,
 0x0001e792U,
 0x0001e254U,
 0x0001e254U,
 0x0001e4f6U,
 0x0001e2f6U,
 0x0001e5aeU,
 0x0001e6e2U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e73eU,
 0x0001e254U,
 0x0001e254U,
 0x0001e52aU,
 0x0001e792U,
 0x0001e254U,
 0x0001e4daU,
 0x0001e74eU,
 0x0001e254U,
 0x0001e792U,
 0x0001e74eU,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e6e2U,
 0x0001e792U,
 0x8001e670U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x8001e42aU,
 0x8001ebceU,
 0x8001eb62U,
 0x0001e254U,
 0x0001e532U,
 0x0001e512U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e562U,
 0x0001e62cU,
 0x0001e664U,
 0x0001e6a4U,
 0x0001e67cU,
 0x8001e452U,
 0x0001e274U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e256U,
 0x8001e452U,
 0x8001e34eU,
 0x8001e368U,
 0x0001e254U,
 0x0001e254U,
 0x0001e274U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x8001e39cU,
 0x8001e452U,
 0x8001e334U,
 0x8001e3b2U,
 0x8001e3c8U,
 0x8001e3deU,
 0x8001e382U,
 0x8001e452U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e2a8U,
 0x0001e254U,
 0x0001e254U,
 0x0001e25eU,
 0x0001e254U,
 0x0001e3c8U,
 0x0001e254U,
 0x8001e27aU,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U,
 0x0001e254U
};

static const uint8_t cpu_state6_weights[10][6]={
 {0x14U,0x50U,0x0fU,0x55U,0x0aU,0x5aU},
 {0x14U,0x50U,0x0fU,0x55U,0x0fU,0x55U},
 {0x14U,0x50U,0x14U,0x50U,0x0fU,0x55U},
 {0x14U,0x50U,0x14U,0x50U,0x14U,0x50U},
 {0x1eU,0x46U,0x14U,0x50U,0x19U,0x4bU},
 {0x19U,0x4bU,0x14U,0x50U,0x14U,0x50U},
 {0x19U,0x4bU,0x19U,0x4bU,0x14U,0x50U},
 {0x19U,0x4bU,0x19U,0x4bU,0x19U,0x4bU},
 {0x19U,0x4bU,0x19U,0x4bU,0x19U,0x4bU},
 {0x1eU,0x46U,0x1eU,0x46U,0x1eU,0x46U}
};

static const uint8_t cpu_state9_ground[12]={0x0fU,0x0cU,0x0cU,0x0fU,0x0cU,0x0cU,0x0fU,0x0cU,0x0fU,0x12U,0x0fU,0x11U};
static const uint8_t cpu_state9_normal[12]={0x1bU,0x1bU,0x0dU,0x0dU,0x1bU,0x1bU,0x1bU,0x1bU,0x0dU,0x0dU,0x1bU,0x1bU};


static uint32_t ror32(uint32_t v, unsigned n)
{
    n &= 31U;
    return n ? ((v >> n) | (v << (32U - n))) : v;
}

uint32_t arcade_cpu_main_handler(uint8_t s)
{
    return s < ARCADE_CPU_MAIN_STATE_COUNT ? cpu_main[s] : 0U;
}

uint32_t arcade_cpu_state1_handler(uint8_t s)
{
    return s < ARCADE_CPU_STATE1_SUBSTATE_COUNT ? cpu_state1[s] : 0U;
}

arcade_cpu_exec_entry_t arcade_cpu_executor_entry(uint8_t s)
{
    arcade_cpu_exec_entry_t r = {0, 0, false};
    if (s >= ARCADE_CPU_SELECTOR_COUNT)
        return r;
    r.raw_entry = cpu_exec[s];
    r.force_call = (r.raw_entry & 0x80000000U) != 0;
    r.callback = r.raw_entry & 0x7fffffffU;
    return r;
}

bool arcade_cpu_executor_should_call(uint8_t s, uint16_t state20)
{
    arcade_cpu_exec_entry_t e = arcade_cpu_executor_entry(s);
    if (!e.callback)
        return false;
    if (e.force_call)
        return true;
    /* $1E002..$1E012: suppress only while numeric state is 5 AND +$20.bit7 is set. */
    return !(((state20 & 0x00ffU) == 5U) && ((state20 & 0x8000U) != 0));
}

uint32_t arcade_cpu_rng_step(arcade_cpu_rng_t *r, uint32_t d1)
{
    uint32_t d0;
    if (!r)
        return 0;
    d0 = r->seed;
    d0 += d1;
    d0 += r->d2;
    d0 += r->d3;
    d0 += r->d4;
    d0 += r->d5;
    d0 += r->d6;
    d0 += r->d7;
    d0 += r->global_80;
    d0 = ror32(d0, (unsigned)(d0 & 7U));
    r->seed = d0;
    return d0;
}

int arcade_cpu_weighted_select(arcade_cpu_rng_t *r, const uint8_t *weights,
                               size_t count, uint16_t *sample_out)
{
    uint16_t sample = 50U;
    bool accepted = false;
    uint8_t total = 0;
    int d1;
    size_t i;

    if (!r || !weights || !count)
        return -1;
    for (d1 = 3; d1 >= 0; --d1) {
        uint32_t v = arcade_cpu_rng_step(r, (uint32_t)d1);
        sample = (uint16_t)((v & 0xffU) >> 1);
        if (sample < 100U) {
            accepted = true;
            break;
        }
    }
    if (!accepted)
        sample = 50U;
    if (sample == 0U)
        sample = 1U;
    if (sample_out)
        *sample_out = sample;

    /* $24FA ADD.B accumulates only the low byte of D2. */
    for (i = 0; i < count; ++i) {
        total = (uint8_t)(total + weights[i]);
        if ((uint16_t)total >= sample)
            return (int)i;
    }
    return -1;
}

arcade_cpu_geometry_t arcade_cpu_target_geometry(uint16_t ax, uint16_t ay,
                                                  uint16_t tx, uint16_t ty,
                                                  uint8_t ts, uint8_t t65,
                                                  uint8_t t2e, uint8_t t61)
{
    arcade_cpu_geometry_t r = {0, 0, 0, 0};
    uint16_t ex = tx;
    uint16_t dx;
    uint16_t sy;
    uint16_t dy;

    if (ts == 4U) {
        uint16_t off = t65 == 8U ? 0x40U : (t65 == 9U ? 0x28U : 0U);
        if (off)
            ex = (uint16_t)(ex + ((t2e & 0x80U) ? (uint16_t)(0U - off) : off));
    }
    dx = (uint16_t)(ex - ax);
    if (ex >= ax)
        r.b5_low2 |= 1U;
    else
        dx = (uint16_t)(0U - dx);
    r.b0 = dx;

    sy = ay;
    if (ts == 5U && t61 == 0x22U)
        sy = (uint16_t)(sy + 8U);
    dy = (uint16_t)(sy - ty);
    if (sy < ty) {
        dy = (uint16_t)(0U - dy);
        r.b5_low2 |= 2U;
    }
    r.b2 = dy;
    r.b4 = dx < 0x50U ? 0U : (dx < 0xb0U ? 1U : 2U);
    return r;
}

uint8_t arcade_cpu_stamina_bucket(uint16_t s)
{
    if (!s) return 8U;
    if (s >= 0x80U) return 0U;
    if (s >= 0x65U) return 1U;
    if (s >= 0x50U) return 2U;
    if (s >= 0x40U) return 3U;
    if (s >= 0x25U) return 4U;
    return 5U;
}

bool arcade_cpu_state9_lod_partner_trigger(uint8_t bd, uint8_t ps, uint8_t p61,
                                           uint16_t *st, uint16_t *r60)
{
    if (bd < 8U || ps != 5U || p61 != 0x89U)
        return false;
    if (st)
        *st = 0x0005U;
    if (r60)
        *r60 = 0x0004U;
    return true;
}

const uint8_t *arcade_cpu_state6_weight_pair(uint8_t p, uint8_t tier)
{
    if (p >= 10U || tier >= 3U)
        return 0;
    return &cpu_state6_weights[p][tier * 2U];
}

uint8_t arcade_cpu_state9_selector(uint8_t id, bool special)
{
    if (id >= 12U)
        return 0xffU;
    return special ? cpu_state9_ground[id] : cpu_state9_normal[id];
}

const uint32_t *arcade_cpu_main_table(void) { return cpu_main; }
const uint32_t *arcade_cpu_state1_table(void) { return cpu_state1; }
const uint32_t *arcade_cpu_executor_table(void) { return cpu_exec; }
