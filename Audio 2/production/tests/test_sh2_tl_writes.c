/* Exact TL setup + YM-write lifts, checked against the untouched scalar Z80.
 * Usage: test_sh2_tl_writes SOUND.ROM [0x0c09|0x0c16|0x0c21|0x0c2c]
 *                          [--oracle-only|--rejects-only]
 *
 * The private ROM is loaded at run time; no original data is embedded here.
 * Each lift stops at 0F5F, before EX AF,AF' / RET. This is a host semantic
 * test, not a native throughput, timer-scheduling or audio-fidelity test.
 *
 * The observer snapshots complete CPU/RAM/adapter state on entry to each
 * C800/C801 write callback. Status is stable and read-only in this fixture.
 * Read callback counts (including the original C801 status read) are
 * deliberately not observations: the native adapter may read v.status
 * directly. No timer or external device is synthesized by this fixture.
 */
#include "wf_music_seq.h"
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned wf_z80_control_fast_step(z80 *, unsigned, uint32_t);
extern bool wf_z80_native_compatible(const uint8_t *, size_t);
extern void wf_ref_z80_init(z80 *);
extern void wf_ref_z80_step(z80 *);

enum { TL_INSTRUCTIONS = 18, TL_FINAL_PC = 0x0f5f };
typedef struct {
    uint16_t pc, level, back;
    unsigned ticks;
} tl_spec;
static const tl_spec specs[] = {
    {0x0c09, 0x008a, 0x0c16, 139},
    {0x0c16, 0x008c, 0x0c21, 130},
    {0x0c21, 0x008b, 0x0c2c, 130},
    {0x0c2c, 0x008d, 0x0000, 123}
};
typedef struct {
    uint16_t address;
    uint8_t value;
    wf_music_seq_t state;
} write_event;
typedef struct {
    unsigned count;
    write_event events[2];
} write_log;
enum run_mode { DIFFERENTIAL, ORACLE_ONLY, REJECTS_ONLY };
static uint8_t rom[65536];
static wf_music_seq_t reference, candidate, initial;
static write_log reference_log, candidate_log;
static const tl_spec *operation;
static const char *phase;
static enum run_mode mode;
static uint64_t accepted, rejected;

static void fail(const char *message)
{
    fprintf(stderr, "FAIL TL %04X %s: %s; accepted=%" PRIu64
            " rejected=%" PRIu64 " SP=%04X D=%02X channel=%02X level=%02X\n",
            operation ? operation->pc : 0, phase ? phase : "input", message,
            accepted, rejected, initial.cpu.sp, initial.cpu.d,
            initial.ram[0x65], operation ? initial.ram[operation->level] : 0);
    exit(3);
}
static void require(bool condition, const char *message)
{
    if (!condition) fail(message);
}
static uint8_t flags(const z80 *z)
{
    return (uint8_t)((z->sf << 7) | (z->zf << 6) | (z->yf << 5) |
                     (z->hf << 4) | (z->xf << 3) | (z->pf << 2) |
                     (z->nf << 1) | z->cf);
}
static void set_flags(z80 *z, unsigned f)
{
    z->sf = (f >> 7) & 1; z->zf = (f >> 6) & 1;
    z->yf = (f >> 5) & 1; z->hf = (f >> 4) & 1;
    z->xf = (f >> 3) & 1; z->pf = (f >> 2) & 1;
    z->nf = (f >> 1) & 1; z->cf = f & 1;
}
static uint8_t refresh(uint8_t r, unsigned n)
{
    return (uint8_t)((r & 128u) | ((r + n) & 127u));
}
static uint8_t read_byte(void *user, uint16_t address)
{
    wf_music_seq_t *v = user;
    if (address < 0xc000) return v->rom[address];
    if (address < 0xc800) return v->ram[address - 0xc000];
    if (address == 0xc800 || address == 0xc801) return v->status;
    fail("unexpected MMIO read");
    return 0;
}
static void write_byte(void *user, uint16_t address, uint8_t value)
{
    wf_music_seq_t *v = user;
    if (address >= 0xc000 && address < 0xc800) {
        v->ram[address - 0xc000] = value;
        return;
    }
    require(address == 0xc800 || address == 0xc801, "unexpected non-RAM write");
    write_log *log = v == &reference ? &reference_log : &candidate_log;
    require(v == &reference || v == &candidate, "unknown write observer");
    require(log->count < 2, "more than two YM callbacks");
    write_event *event = &log->events[log->count++];
    event->address = address; event->value = value;
    memcpy(&event->state, v, sizeof *v);
    /* All function pointers are identical and remain compared. Only the
     * necessarily different owner pointer is normalized. */
    event->state.cpu.userdata = NULL;
    if (address == 0xc800) v->ym_addr = value;
    ++v->io_epoch; /* Observable writes only; no count of status reads. */
}
static uint8_t port_in(z80 *z, uint8_t port)
{
    (void)z; (void)port; fail("unexpected port input"); return 0;
}
static void port_out(z80 *z, uint8_t port, uint8_t value)
{
    (void)z; (void)port; (void)value; fail("unexpected port output");
}
static void compare_state(const wf_music_seq_t *left, const wf_music_seq_t *right)
{
    wf_music_seq_t l, r;
    memcpy(&l, left, sizeof l); memcpy(&r, right, sizeof r);
    l.cpu.userdata = r.cpu.userdata = NULL;
    if (!memcmp(&l, &r, sizeof l)) return;
    fprintf(stderr, "CPU PC=%04X/%04X cyc=%lu/%lu R=%02X/%02X "
            "SP=%04X/%04X WZ=%04X/%04X F=%02X/%02X\n",
            l.cpu.pc, r.cpu.pc, l.cpu.cyc, r.cpu.cyc, l.cpu.r, r.cpu.r,
            l.cpu.sp, r.cpu.sp, l.cpu.mem_ptr, r.cpu.mem_ptr,
            flags(&l.cpu), flags(&r.cpu));
    const unsigned char *lp = (const unsigned char *)&l;
    const unsigned char *rp = (const unsigned char *)&r;
    for (size_t i = 0; i < sizeof l; ++i) {
        if (lp[i] != rp[i]) {
            fprintf(stderr, "First state byte mismatch at %zu: %02X/%02X\n",
                    i, lp[i], rp[i]);
            break;
        }
    }
    fail("complete CPU/RAM/adapter state differs from original");
}
static uint32_t next_seed(uint32_t *seed)
{
    *seed ^= *seed << 13; *seed ^= *seed >> 17; *seed ^= *seed << 5;
    return *seed;
}
static void setup(unsigned source, unsigned level, unsigned f, unsigned alt_f,
                  uint32_t seed)
{
    memset(&reference, 0, sizeof reference);
    reference.rom = rom;
    for (unsigned i = 0; i < sizeof reference.ram; ++i)
        reference.ram[i] = (uint8_t)(i * 37u + seed + ((i >> 3) ^ (seed >> 16)));
    z80 *z = &reference.cpu;
    wf_ref_z80_init(z);
    z->read_byte = read_byte; z->write_byte = write_byte;
    z->port_in = port_in; z->port_out = port_out; z->userdata = &reference;
    z->pc = operation->pc; z->sp = (uint16_t)(0xc402u + seed % 1023u);
    z->cyc = (seed & 1u) ? 0xfffffff0ul : 0x123456ul;
    z->ix = (uint16_t)next_seed(&seed); z->iy = (uint16_t)next_seed(&seed);
    z->mem_ptr = (uint16_t)next_seed(&seed);
    z->a = (uint8_t)next_seed(&seed); z->b = (uint8_t)next_seed(&seed);
    z->c = (uint8_t)next_seed(&seed); z->d = (uint8_t)next_seed(&seed);
    z->e = (uint8_t)next_seed(&seed); z->h = (uint8_t)next_seed(&seed);
    z->l = (uint8_t)next_seed(&seed); z->a_ = (uint8_t)next_seed(&seed);
    z->b_ = (uint8_t)next_seed(&seed); z->c_ = (uint8_t)next_seed(&seed);
    z->d_ = (uint8_t)next_seed(&seed); z->e_ = (uint8_t)next_seed(&seed);
    z->h_ = (uint8_t)next_seed(&seed); z->l_ = (uint8_t)next_seed(&seed);
    z->i = (uint8_t)next_seed(&seed); z->r = (uint8_t)next_seed(&seed);
    z->interrupt_mode = (uint8_t)(next_seed(&seed) % 3u);
    z->int_data = (uint8_t)next_seed(&seed);
    z->iff1 = next_seed(&seed) & 1u; z->iff2 = next_seed(&seed) & 1u;
    set_flags(z, f); z->f_ = (uint8_t)alt_f;
    reference.status = (uint8_t)(next_seed(&seed) & 127u);
    reference.ym_addr = (uint8_t)next_seed(&seed);
    reference.tb = (uint8_t)next_seed(&seed); reference.ctl = (uint8_t)next_seed(&seed);
    reference.latch = (uint8_t)next_seed(&seed); reference.bank = (uint8_t)next_seed(&seed);
    reference.next_tb = next_seed(&seed); reference.last_cpu = next_seed(&seed);
    reference.cycles = (uint64_t)next_seed(&seed) << 19;
    reference.steps = next_seed(&seed); reference.skipped = next_seed(&seed);
    reference.optimized = true; reference.command_pending = (seed & 1u) != 0;
    reference.io_epoch = next_seed(&seed);
    reference.ram[0x65] = (uint8_t)next_seed(&seed);
    if (operation->pc == 0x0c09) reference.ram[0x65] = (uint8_t)source;
    else z->d = (uint8_t)source;
    /* Distinct unused levels detect using the wrong operator's RAM cell. */
    for (unsigned i = 0; i < 4; ++i)
        reference.ram[0x8a + i] = (uint8_t)(level ^ (0x33u * (i + 1u)));
    reference.ram[operation->level] = (uint8_t)level;
}
static void clone_case(void)
{
    memcpy(&initial, &reference, sizeof initial);
    memcpy(&candidate, &reference, sizeof candidate);
    candidate.cpu.userdata = &candidate;
    memset(&reference_log, 0, sizeof reference_log);
    memset(&candidate_log, 0, sizeof candidate_log);
}
static void run_original(void)
{
    const unsigned prefix_ticks = operation->ticks - 81u;
    const uint8_t reg = operation->pc == 0x0c09
        ? (uint8_t)(initial.ram[0x65] + 0x60u) : (uint8_t)(initial.cpu.d + 8u);
    const uint8_t level = initial.ram[operation->level];
    const uint16_t sp = (uint16_t)(initial.cpu.sp - (operation->back ? 2u : 0u));
    uint8_t expected_ram[sizeof reference.ram];
    memcpy(expected_ram, initial.ram, sizeof expected_ram);
    if (operation->back) {
        expected_ram[sp - 0xc000] = (uint8_t)operation->back;
        expected_ram[sp - 0xbfff] = (uint8_t)(operation->back >> 8);
    }
    uint8_t prefix_flags = 0;
    for (unsigned step = 1; step <= TL_INSTRUCTIONS; ++step) {
        wf_ref_z80_step(&reference.cpu);
        require(reference_log.count == (step < 12u ? 0u : step < 18u ? 1u : 2u),
                "original callback instruction boundary differs");
        if (step < TL_INSTRUCTIONS)
            require(reference.cpu.pc != TL_FINAL_PC, "original reached final PC too early");
        if (step == 6) {
            require(reference.cpu.pc == 0x0f4b && reference.cpu.mem_ptr == 0x0f4b,
                    "original six-instruction prefix target differs");
            require(reference.cpu.cyc == initial.cpu.cyc + prefix_ticks &&
                    reference.cpu.r == refresh(initial.cpu.r, 6), "original prefix T/R differs");
            require(reference.cpu.sp == sp && reference.cpu.d == reg &&
                    reference.cpu.a == level && reference.cpu.e == level,
                    "original prefix register/stack contract differs");
            require(!memcmp(reference.ram, expected_ram, sizeof expected_ram),
                    "original CALL return address or JP RAM preservation differs");
            prefix_flags = flags(&reference.cpu);
        }
    }
    require(reference.cpu.pc == TL_FINAL_PC && reference.cpu.sp == sp &&
            reference.cpu.cyc == initial.cpu.cyc + operation->ticks &&
            reference.cpu.r == refresh(initial.cpu.r, TL_INSTRUCTIONS),
            "original final PC/SP/T/R differs");
    require(reference.cpu.mem_ptr == (uint16_t)((level << 8) | 2u) &&
            reference.cpu.a == level && reference.cpu.a_ == level &&
            reference.cpu.f_ == prefix_flags && flags(&reference.cpu) == 0x54,
            "original final writer/alternate-AF state differs");
    require(!memcmp(reference.ram, expected_ram, sizeof expected_ram), "original final RAM differs");
    require(reference.status == initial.status, "status must remain read-only");
    for (unsigned i = 0; i < 2; ++i) {
        const write_event *event = &reference_log.events[i];
        const z80 *z = &event->state.cpu;
        require(event->address == 0xc800u + i && event->value == (i ? level : reg),
                "original YM address/data order or value differs");
        require(z->pc == (i ? TL_FINAL_PC : 0x0f57) && z->sp == sp &&
                z->cyc == initial.cpu.cyc + (i ? operation->ticks : prefix_ticks + 48u) &&
                z->r == refresh(initial.cpu.r, i ? 18u : 12u), "original callback PC/SP/T/R differs");
        require(z->a == event->value && z->d == reg && z->e == level &&
                z->a_ == level && z->f_ == prefix_flags && flags(z) == 0x54 &&
                z->mem_ptr == (i ? (uint16_t)((reg << 8) | 1u) : 0xc802),
                "original callback CPU state differs");
        require(!memcmp(event->state.ram, expected_ram, sizeof expected_ram),
                "original callback RAM/return address differs");
    }
}
static void admit(unsigned budget, uint32_t extra_time)
{
    clone_case();
    run_original();
    if (mode != ORACLE_ONLY) {
        uint32_t limit = (uint32_t)initial.cpu.cyc + operation->ticks + extra_time;
        unsigned took = wf_z80_control_fast_step(&candidate.cpu, budget, limit);
        if (took != TL_INSTRUCTIONS) {
            if (!took) {
                compare_state(&initial, &candidate);
                require(candidate_log.count == 0, "unadmitted lift emitted a write");
            }
            fprintf(stderr, "RED TL operation %04X expected %u original instructions, admitted %u\n",
                    operation->pc, TL_INSTRUCTIONS, took);
            exit(2);
        }
        require(candidate_log.count == reference_log.count, "YM callback count differs");
        for (unsigned i = 0; i < reference_log.count; ++i) {
            const write_event *l = &reference_log.events[i], *r = &candidate_log.events[i];
            require(l->address == r->address && l->value == r->value,
                    "YM callback address/data differs");
            compare_state(&l->state, &r->state);
        }
        compare_state(&reference, &candidate);
    }
    ++accepted;
}
static void acceptance_matrix(void)
{
    phase = "source/level matrix";
    for (unsigned source = 0; source < 256; ++source)
        for (unsigned level = 0; level < 256; ++level)
            for (unsigned pattern = 0; pattern < 2; ++pattern) {
                unsigned f = (source ^ level) ^ (pattern ? 255u : 0u);
                setup(source, level, f, f ^ 0xa5u,
                      0x9e3779b9u ^ (operation->pc << 16) ^ (source << 8) ^
                      level ^ (pattern ? 0x85ebca6bu : 0u));
                reference.cpu.cyc = pattern ? 0xfffffff0ul : 0x123456ul;
                admit(18, 0);
            }
    phase = "principal/alternate flags matrix";
    for (unsigned f = 0; f < 256; ++f)
        for (unsigned alt = 0; alt < 256; ++alt) {
            setup(f ^ 0xb3u, alt ^ 0x69u, f, alt, 0x52c40000u | (f << 8) | alt);
            admit(18, 0);
        }
    phase = "all valid SP values";
    for (unsigned sp = 0xc402; sp <= 0xc800; ++sp) {
        setup(sp, sp >> 3, sp, ~sp, 0xb23a0000u | sp);
        reference.cpu.sp = (uint16_t)sp;
        admit(18, 0);
    }
    phase = "cycle/budget boundary matrix";
    static const unsigned long starts[] = {
        0ul, 1ul, 0x123456ul, 0x7ffffffful, 0x80000000ul,
        0xfffffff0ul, 0xfffffffful, ULONG_MAX - 150ul, ULONG_MAX - 1ul
    };
    static const unsigned budgets[] = {18u, 19u, UINT_MAX};
    const uint32_t extra[] = {0u, 1u, 0x7fffffffu - operation->ticks};
    for (unsigned t = 0; t < sizeof starts / sizeof starts[0]; ++t)
        for (unsigned b = 0; b < sizeof budgets / sizeof budgets[0]; ++b)
            for (unsigned e = 0; e < sizeof extra / sizeof extra[0]; ++e) {
                setup(0xf8u + t, 0x7fu + b, e * 0x55u, 0xffu - e * 0x55u,
                      0xc5630000u | (t << 8) | (b << 4) | e);
                reference.cpu.cyc = starts[t];
                admit(budgets[b], extra[e]);
            }
    phase = "interrupt state preservation";
    for (unsigned im = 0; im < 3; ++im)
        for (unsigned state = 0; state < 6; ++state) {
            setup(im * 0x7fu, state * 0x33u, state, ~state, 0x61480000u | (im << 8) | state);
            reference.cpu.interrupt_mode = (uint8_t)im;
            reference.cpu.int_data = (uint8_t)(im * 0x71u + state * 0x31u);
            reference.cpu.int_pending = state >= 4;
            reference.cpu.iff1 = state < 4 && (state & 1u);
            reference.cpu.iff2 = state < 4 ? (state >> 1) & 1u : state & 1u;
            admit(18, 0);
        }
}
static void reject(unsigned budget, uint32_t limit)
{
    wf_music_seq_t before;
    memcpy(&before, &candidate, sizeof before);
    unsigned took = wf_z80_control_fast_step(&candidate.cpu, budget, limit);
    require(!took, "invalid admission was accepted");
    require(!memcmp(&before, &candidate, sizeof before), "rejection mutated CPU/RAM/adapter bytes");
    require(!candidate_log.count, "rejection emitted a YM callback");
    ++rejected;
}
static void rejection_setup(unsigned seed, bool wrap)
{
    setup(0xf8, 0x7f, 0xa5, 0x5a, 0x62590000u ^ seed);
    reference.cpu.cyc = wrap ? 0xfffffff0ul : 0x123456ul;
    clone_case();
}
static uint32_t full_limit(void)
{
    return (uint32_t)candidate.cpu.cyc + operation->ticks;
}
static void rejection_matrix(void)
{
    phase = "budget/time/busy/stack rejection";
    static const uint16_t bad_sp[] = {
        0, 1, 0x3ffe, 0xbffe, 0xbfff, 0xc000, 0xc001, 0xc3fe,
        0xc3ff, 0xc400, 0xc401, 0xc801, 0xc802, 0xcffe, 0xffff
    };
    for (unsigned wrap = 0; wrap < 2; ++wrap) {
        for (unsigned budget = 0; budget < TL_INSTRUCTIONS; ++budget) {
            rejection_setup(budget, wrap != 0); reject(budget, full_limit());
        }
        for (unsigned available = 0; available < operation->ticks; ++available) {
            rejection_setup(available, wrap != 0);
            reject(18, (uint32_t)candidate.cpu.cyc + available);
        }
        for (unsigned status = 128; status < 256; ++status) {
            rejection_setup(status, wrap != 0); candidate.status = (uint8_t)status;
            reject(18, full_limit());
        }
        for (unsigned i = 0; i < sizeof bad_sp / sizeof bad_sp[0]; ++i) {
            rejection_setup(i, wrap != 0); candidate.cpu.sp = bad_sp[i];
            reject(18, full_limit());
        }
        static const uint32_t late[] = {0xffffffffu, 0x80000001u, 0x80000000u};
        for (unsigned i = 0; i < sizeof late / sizeof late[0]; ++i) {
            rejection_setup(i, wrap != 0);
            reject(18, (uint32_t)candidate.cpu.cyc + late[i]);
        }
    }
    phase = "interrupt/halt/entry rejection";
    for (unsigned im = 0; im < 3; ++im)
        for (unsigned iff2 = 0; iff2 < 2; ++iff2) {
            rejection_setup(im * 2u + iff2, true);
            candidate.cpu.interrupt_mode = (uint8_t)im;
            candidate.cpu.int_pending = candidate.cpu.iff1 = true;
            candidate.cpu.iff2 = iff2 != 0;
            reject(18, full_limit());
        }
    for (unsigned state = 0; state < 4; ++state) {
        rejection_setup(state, false);
        candidate.cpu.nmi_pending = true;
        candidate.cpu.iff1 = state & 1u; candidate.cpu.iff2 = (state >> 1) & 1u;
        reject(18, full_limit());
    }
    static const uint8_t delays[] = {1, 2, 255};
    for (unsigned i = 0; i < sizeof delays / sizeof delays[0]; ++i) {
        rejection_setup(i, true); candidate.cpu.iff_delay = delays[i];
        reject(18, full_limit());
    }
    for (unsigned iff1 = 0; iff1 < 2; ++iff1) {
        rejection_setup(iff1, false); candidate.cpu.halted = true;
        candidate.cpu.iff1 = iff1 != 0; reject(18, full_limit());
    }
    for (unsigned side = 0; side < 2; ++side) {
        rejection_setup(side, false);
        candidate.cpu.pc = (uint16_t)(operation->pc + (side ? 1 : -1));
        reject(18, full_limit());
    }
}
int main(int argc, char **argv)
{
    unsigned selected = 0;
    if (argc < 2 || argc > 4) return 64;
    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "--oracle-only") && mode == DIFFERENTIAL) mode = ORACLE_ONLY;
        else if (!strcmp(argv[i], "--rejects-only") && mode == DIFFERENTIAL) mode = REJECTS_ONLY;
        else {
            char *end = NULL;
            unsigned long value = strtoul(argv[i], &end, 0);
            if (selected || !*argv[i] || *end ||
                (value != 0x0c09 && value != 0x0c16 && value != 0x0c21 && value != 0x0c2c))
                return 64;
            selected = (unsigned)value;
        }
    }
    FILE *input = fopen(argv[1], "rb");
    if (!input) { perror(argv[1]); return 66; }
    bool complete = fread(rom, 1, sizeof rom, input) == sizeof rom &&
                    fgetc(input) == EOF && !ferror(input);
    if (fclose(input)) complete = false;
    require(complete && wf_z80_native_compatible(rom, sizeof rom),
            "require complete unchanged original SOUND.ROM");
    for (unsigned i = 0; i < sizeof specs / sizeof specs[0]; ++i) {
        operation = &specs[i];
        if (selected && selected != operation->pc) continue;
        uint64_t before_accepted = accepted, before_rejected = rejected;
        if (mode != ORACLE_ONLY) rejection_matrix();
        if (mode != REJECTS_ONLY) acceptance_matrix();
        uint64_t expected_accepted = mode == REJECTS_ONLY ? 0u : 197730u;
        uint64_t expected_rejected = mode == ORACLE_ONLY ? 0u : 345u + 2u * operation->ticks;
        require(accepted - before_accepted == expected_accepted &&
                rejected - before_rejected == expected_rejected, "matrix did not finish every case");
        printf("PASS TL %04X %s accepted=%" PRIu64 " rejected=%" PRIu64
               " instructions=18 T=%u R=18 final-PC=0F5F\n", operation->pc,
               mode == ORACLE_ONLY ? "original-ROM oracle" : mode == REJECTS_ONLY
               ? "non-mutating rejections" : "exact differential",
               accepted - before_accepted, rejected - before_rejected, operation->ticks);
    }
    printf("PASS TL total accepted=%" PRIu64 " rejected=%" PRIu64
           " callback-snapshots-per-engine=%" PRIu64 " mode=%s\n",
           accepted, rejected, accepted * 2u, mode == ORACLE_ONLY ? "oracle-only"
           : mode == REJECTS_ONLY ? "rejects-only" : "differential");
    return 0;
}
