#include "wp02_audio.h"
#include "wp02_audio_assets.h"
#include "wp02_pcm16.h"
#include "wp02_pcm16_gain.h"
#include "wp02_bgm.h"
#include "wf_audio_sfx.h"
#include "ponesound_min.h"
#include <yaul.h>
#include <string.h>

enum {
    WP02_SECTOR_BYTES = 2048,
    WP02_STAGING_BYTES = 0x4000,
    WP02_SCSP_START = 0x14000,
    WP02_SCSP_LIMIT = 0x80000
};

/* Selected identities use0x14000..0x6C428 at worst. The original0E background
 * has a disjoint transport ring at0x6C800. No playback-time disc I/O. */
static uint8_t staging[WP02_STAGING_BYTES] __aligned(32);
static uint32_t resident[WP02_PCM_ASSET_COUNT];
static uint16_t sample_ticks_left[3];
static bool suppress_next[3];
static volatile wp02_audio_state_t state = WP02_AUDIO_DISABLED;
static uint32_t timer_a_accum;
static unsigned last_command;
static unsigned sound_ram_end;
static unsigned bgm_max_service_us;
static bool driver_loaded;
static bool frt_started;
static volatile uint64_t frt_period_ticks;
static uint16_t frt_last_count;
static uint16_t frt_next_compare;
static uint32_t frt_pending_ticks;

/* Original $AC4A identities mapped through the recovered SFX table. */
static const uint8_t identity_sample[12] = {
    23,24,25,30,29,29,27,26,28,31,29,29
};

static int asset_index(unsigned bank, unsigned sample)
{
    /* Common samples 17/18 are identical in both original ROM banks. */
    if (sample == 17u || sample == 18u) bank = 0;
    for (unsigned i = 0; i < WP02_PCM_ASSET_COUNT; ++i)
        if (wp02_pcm_assets[i].bank == bank && wp02_pcm_assets[i].sample == sample)
            return (int)i;
    return -1;
}

static bool fixed_sample(const wp02_pcm_asset_t *a)
{
    return a->bank == 0u || a->sample == 37u ||
           a->sample == 38u || a->sample == 39u;
}

static bool preload(uint32_t fad, const uint8_t ids[4])
{
    unsigned address = WP02_SCSP_START;
    memset(resident, 0, sizeof(resident));
    for (unsigned i = 0; i < WP02_PCM_ASSET_COUNT; ++i) {
        const wp02_pcm_asset_t *a = &wp02_pcm_assets[i];
        bool wanted = fixed_sample(a);
        for (unsigned w = 0; w < 4u; ++w)
            wanted |= a->bank == 1u && a->sample == identity_sample[ids[w]];
        if (!wanted) continue;
        address = (address + 31u) & ~31u;
        if (address + a->bytes > WP02_SCSP_LIMIT) return false;
        for (unsigned copied = 0; copied < a->bytes;) {
            unsigned count = a->bytes - copied;
            if (count > WP02_STAGING_BYTES) count = WP02_STAGING_BYTES;
            unsigned read_bytes = (count + WP02_SECTOR_BYTES - 1u) &
                                  ~(WP02_SECTOR_BYTES - 1u);
            if (cd_block_sectors_read(fad + a->sector + copied / WP02_SECTOR_BYTES,
                                      staging, read_bytes) != 0) return false;
            if (!wp02_apply_oki_gain_be(staging, count, a->volume)) return false;
            wf_ponesound_copy_to_scsp(staging, (int)count, (int)(address + copied));
            copied += count;
        }
        resident[i] = address;
        address += a->bytes;
    }
    sound_ram_end = address;
    return true;
}

static void stop_cb(unsigned voice, void *user)
{
    (void)user;
    if (voice >= 3u) return;
    wf_ponesound_pcm_cease((short)voice);
    sample_ticks_left[voice] = 0;
    suppress_next[voice] = false;
}

static void play_cb(unsigned voice, unsigned bank, unsigned sample,
                    unsigned volume, bool suppress, void *user)
{
    (void)user;
    if (voice >= 3u) return;
    /* Original F1: an OKI start on a still-active voice is ignored. */
    if (sample_ticks_left[voice] && suppress_next[voice]) {
        suppress_next[voice] = suppress;
        return;
    }
    if (sample_ticks_left[voice]) wf_ponesound_pcm_cease((short)voice);
    int index = asset_index(bank, sample);
    if (index < 0 || resident[index] == 0u) {
        state = WP02_AUDIO_ERROR;
        return;
    }
    const wp02_pcm_asset_t *a = &wp02_pcm_assets[index];
    if (volume != a->volume) { state = WP02_AUDIO_ERROR; return; }
    wp02_pcm16_configure(voice, a->samples, resident[index]);
    /* Semi-protected mode permits an intentional replacement cue to restart
     * an existing physical slot. F1 suppression is already handled above. */
    /* Pinned driver writes volume<<13 into the three direct-send bits.
     * Original OKI gain is already present in resident PCM16. */
    wf_ponesound_pcm_play((short)voice, WP02_PCM_SEMI, 7);
    sample_ticks_left[voice] = a->ticks;
    suppress_next[voice] = suppress;
}

static void timer_a_tick(void)
{
    for (unsigned i = 0; i < 3u; ++i)
        if (sample_ticks_left[i]) --sample_ticks_left[i];
    wf_sfx_timer_a_tick(play_cb, stop_cb, NULL);
}

/* Call with interrupts masked, or from the FRT callback. The 16-bit hardware
 * counter is never rewritten after initialization. A compare IRQ normally
 * samples it every 3359 ticks (about 1 ms), far before its 19.51 ms rollover.
 * Getters also extend the epoch, including a pending, not-yet-serviced IRQ. */
static uint16_t frt_capture(void)
{
    uint16_t count = cpu_frt_count_get();
    uint16_t elapsed = (uint16_t)(count - frt_last_count);
    frt_last_count = count;
    frt_period_ticks += elapsed;
    frt_pending_ticks += elapsed;
    return count;
}

static void frt_1ms(void)
{
    volatile cpu_ioregs_t * const regs = (volatile cpu_ioregs_t *)CPU_IOREG_BASE;
    for (;;) {
        frt_capture();
        while (frt_pending_ticks >= CPU_FRT_NTSC_320_8_COUNT_1MS) {
            frt_pending_ticks -= CPU_FRT_NTSC_320_8_COUNT_1MS;
            frt_next_compare = (uint16_t)(frt_next_compare +
                                         CPU_FRT_NTSC_320_8_COUNT_1MS);
            if (state != WP02_AUDIO_READY) continue;
            /* Preserved A16 nominal-ms rational scheduler. Catch up every
             * elapsed quantum when a higher-priority IRQ delayed this one. */
            timer_a_accum += WF_SFX_TIMER_A_HZ_NUM;
            while (timer_a_accum >= WF_SFX_TIMER_A_HZ_DEN * 1000u) {
                timer_a_accum -= WF_SFX_TIMER_A_HZ_DEN * 1000u;
                timer_a_tick();
            }
        }
        /* OCRA's shared 8-bit port requires A selection and high-before-low
         * writes. Zero is a valid wrapped deadline; cpu_frt_oca_set(0, ...)
         * would disable the interrupt, so only initial registration uses it.
         * Keeping FRC free-running also avoids the documented-vs-emulated
         * CCLRA compare-clear phase difference. See review/REVIEW.md. */
        regs->tocr &= (uint8_t)~0x10u;
        regs->ocrah = (uint8_t)(frt_next_compare >> 8);
        regs->ocral = (uint8_t)frt_next_compare;
        /* If this callback crossed its next deadline while catching up,
         * advance it again now instead of waiting for a full counter wrap. */
        if ((int16_t)(frt_next_compare - cpu_frt_count_get()) > 0) break;
    }
}

void wp02_audio_stop(void)
{
    uint8_t old_mask = cpu_intc_mask_get();
    cpu_intc_mask_set(15);
    state = WP02_AUDIO_DISABLED;
    wp02_bgm_reset();
    if (driver_loaded) {
        for (unsigned i = 0; i < 3u; ++i) wf_ponesound_pcm_cease((short)i);
        wf_ponesound_vblank(NULL);
    }
    wf_sfx_runtime_reset();
    memset(sample_ticks_left, 0, sizeof(sample_ticks_left));
    memset(suppress_next, 0, sizeof(suppress_next));
    timer_a_accum = 0;
    cpu_intc_mask_set(old_mask);
}

bool wp02_audio_init(uint32_t fad, const uint8_t *driver, unsigned driver_bytes,
                     const uint8_t ids[4])
{
    wp02_audio_stop();
    if (!fad || !driver || !driver_bytes || driver_bytes > WP02_SCSP_START || !ids) {
        state = WP02_AUDIO_ERROR;
        return false;
    }
    for (unsigned i = 0; i < 4u; ++i)
        if (ids[i] >= 12u) { state = WP02_AUDIO_ERROR; return false; }
    wf_ponesound_load_driver(driver, (int)driver_bytes);
    driver_loaded = true;
    last_command = 0;
    bgm_max_service_us = 0;
    sound_ram_end = WP02_SCSP_START;
    if (!preload(fad, ids)) { state = WP02_AUDIO_ERROR; return false; }
    uint8_t old_mask = cpu_intc_mask_get();
    cpu_intc_mask_set(15);
    if (!frt_started) {
        cpu_frt_init(CPU_FRT_CLOCK_DIV_8);
        cpu_frt_count_set(0);
        frt_last_count = 0;
        frt_pending_ticks = 0;
        frt_next_compare = CPU_FRT_NTSC_320_8_COUNT_1MS;
        cpu_frt_oca_set(frt_next_compare, frt_1ms);
        cpu_frt_interrupt_priority_set(8);
        frt_started = true;
    }
    state = WP02_AUDIO_READY;
    cpu_intc_mask_set(old_mask);
    return true;
}

bool wp02_audio_bgm_load(uint32_t fad, uint32_t bytes)
{
    if (state != WP02_AUDIO_READY || sound_ram_end > WP02_BGM_RING_OFFSET ||
        !wp02_bgm_load(fad, bytes)) {
        state = WP02_AUDIO_ERROR;
        return false;
    }
    return true;
}

bool wp02_audio_service(void)
{
    if (state != WP02_AUDIO_READY) return false;
    uint64_t start = wp02_audio_time_us();
    bool ok = wp02_bgm_service(start);
    unsigned elapsed = (unsigned)(wp02_audio_time_us() - start);
    if (elapsed > bgm_max_service_us) bgm_max_service_us = elapsed;
    if (!ok) state = WP02_AUDIO_ERROR;
    return ok;
}

void wp02_audio_scene_done(void) { wp02_bgm_stop(); }
unsigned wp02_audio_bgm_refills(void) { return wp02_bgm_refills(); }
unsigned wp02_audio_bgm_failures(void) { return wp02_bgm_failures(); }
unsigned wp02_audio_bgm_max_service_us(void) { return bgm_max_service_us; }

bool wp02_audio_emit(uint8_t command)
{
    if (state != WP02_AUDIO_READY) return false;
    if (!(command == 0x31u || command == 0x32u ||
          (command >= 0x6Du && command <= 0x75u) ||
          (command >= 0x7Bu && command <= 0x7Du))) return false;
    const wf_sfx_command_t *c = wf_sfx_command(command);
    int index = asset_index(c->bank_policy < 2u ? c->bank_policy : 0u,
                            c->events[0].sample_id);
    if (index < 0 || resident[index] == 0u) return false;
    if (command == 0x32u && !wp02_bgm_start()) {
        state = WP02_AUDIO_ERROR;
        return false;
    }
    uint8_t old_mask = cpu_intc_mask_get();
    cpu_intc_mask_set(15);
    last_command = command;
    int voice = wf_sfx_start_voice(command, play_cb, stop_cb, NULL);
    cpu_intc_mask_set(old_mask);
    return voice >= 0 && state == WP02_AUDIO_READY;
}

void wp02_audio_vblank(void *work)
{
    if (driver_loaded) wf_ponesound_vblank(work);
}

wp02_audio_state_t wp02_audio_state(void) { return state; }
unsigned wp02_audio_last_command(void) { return last_command; }
unsigned wp02_audio_sound_ram_end(void) { return sound_ram_end; }

uint64_t wp02_audio_time_frt_ticks(void)
{
    uint8_t old_mask = cpu_intc_mask_get();
    cpu_intc_mask_set(15);
    if (frt_started) frt_capture();
    uint64_t ticks = frt_started ? frt_period_ticks : 0u;
    cpu_intc_mask_set(old_mask);
    return ticks;
}

uint64_t wp02_audio_time_us(void)
{
    /* Official Mednafen 1.32.1: NTSC master=1746818182, 320-mode CPU
     * divisor=65; FRT uses phi/8. Split the quotient to avoid long-run
     * multiplication overflow. Physical oscillator tolerance is separate. */
    const uint64_t ticks = wp02_audio_time_frt_ticks();
    const uint32_t master = 1746818182u;
    const uint32_t scale = 520000000u;
    return (ticks / master) * scale + (ticks % master) * scale / master;
}

uint32_t wp02_audio_frt_compare_ticks(void)
{
    return CPU_FRT_NTSC_320_8_COUNT_1MS;
}

unsigned wp02_audio_active_voices(void)
{
    unsigned count = 0;
    uint8_t old_mask = cpu_intc_mask_get();
    cpu_intc_mask_set(15);
    for (unsigned i = 0; i < 3u; ++i)
        if (sample_ticks_left[i] || wf_sfx_voice(i)->active) ++count;
    cpu_intc_mask_set(old_mask);
    return count;
}
