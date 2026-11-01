/* Independently compare the production native controller, with cooperative
 * service enabled, to complete I/O traces captured by the original full Z80
 * interpreter in the preserved A53 handoff. No synthesizer is linked here. */
#include "wf_music_seq.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char rom[65536];
static unsigned long long event_count, services;

typedef struct {
    FILE *stream;
    const char *name;
    uint64_t cycle;
    unsigned kind, reg, value;
    bool available;
} oracle_t;

void wf_music_unknown_opcode(void)
{
    fputs("Unexpected native-controller opcode fallback failure\n", stderr);
    exit(1);
}

static bool no_op_service(void *unused)
{
    (void)unused;
    services++;
    return true;
}

static void next_event(oracle_t *oracle)
{
    char line[256], kind[8];
    unsigned long long cycle;
    unsigned reg, value;
    oracle->available = false;
    while (fgets(line, sizeof line, oracle->stream)) {
        if (sscanf(line, "%llu,%7[^,],%u,%u", &cycle, kind, &reg, &value) != 4)
            continue;
        if (!strcmp(kind, "YM_D")) oracle->kind = WF_MUSIC_YM;
        else if (!strcmp(kind, "OKI")) oracle->kind = WF_MUSIC_OKI;
        else if (!strcmp(kind, "BANK")) oracle->kind = WF_MUSIC_BANK;
        else continue;
        oracle->cycle = cycle;
        oracle->reg = reg;
        oracle->value = value;
        oracle->available = true;
        return;
    }
}

static bool compare_event(unsigned kind, unsigned reg, unsigned value,
                          uint64_t cycle, void *user)
{
    oracle_t *oracle = user;
    if (kind == WF_MUSIC_IRQ) return true;
    if (!oracle->available || kind != oracle->kind || reg != oracle->reg ||
        value != oracle->value || cycle != oracle->cycle) {
        fprintf(stderr, "%s event %llu: actual=%llu/%u/%u/%u expected=%llu/%u/%u/%u available=%u\n",
                oracle->name, event_count, (unsigned long long)cycle, kind, reg, value,
                (unsigned long long)oracle->cycle, oracle->kind, oracle->reg,
                oracle->value, oracle->available);
        return false;
    }
    event_count++;
    next_event(oracle);
    return true;
}

static void trace_info(const char *path, uint64_t *origin, uint64_t *end, unsigned *command)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); exit(2); }
    char line[256], kind[8];
    unsigned long long cycle;
    unsigned reg, value;
    *origin = *end = *command = 0;
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "%llu,%7[^,],%u,%u", &cycle, kind, &reg, &value) == 4) {
            if (!strcmp(kind, "ORIGIN")) { *origin = cycle; *command = reg; }
            if (!strcmp(kind, "END")) *end = cycle;
        }
    fclose(f);
    assert(*origin && *end > *origin && *command >= 1 && *command <= 18);
}

int main(int argc, char **argv)
{
    assert(argc >= 3);
    FILE *f = fopen(argv[1], "rb");
    assert(f && fread(rom, 1, sizeof rom, f) == sizeof rom);
    fclose(f);
    for (int i = 2; i < argc; i++) {
        uint64_t origin, end;
        unsigned command;
        trace_info(argv[i], &origin, &end, &command);
        oracle_t oracle = {0};
        oracle.name = argv[i];
        oracle.stream = fopen(argv[i], "r");
        assert(oracle.stream);
        next_event(&oracle);
        wf_music_seq_t sequence;
        assert(wf_music_seq_init(&sequence, rom, sizeof rom, compare_event, &oracle, true));
        int status;
        do status = wf_music_seq_run_serviced(&sequence, 2ull * 3579545, 4096,
                                              no_op_service, NULL); while (status == 1);
        assert(status == 0 && sequence.cycles == origin);
        assert(wf_music_seq_send(&sequence, (uint8_t)command));
        do status = wf_music_seq_run_serviced(&sequence, end, 4096,
                                              no_op_service, NULL); while (status == 1);
        assert(status == 0 && sequence.cycles == end && !oracle.available && !sequence.error);
        fclose(oracle.stream);
        printf("0x%02X full A53 trace PASS through cycle %llu (cooperative service enabled)\n",
               command, (unsigned long long)end);
        fflush(stdout);
    }
    printf("PASS: %llu exactly matching YM/OKI/BANK events; %llu no-op service callbacks\n",
           event_count, services);
    return 0;
}
