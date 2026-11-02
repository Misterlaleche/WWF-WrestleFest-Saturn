/* Independent original R01 CPU/sequence comparison. The candidate executes
 * through the production cooperative scheduler, including its smaller native
 * batch grants. Input is the user's original sound ROM, never a recorded loop.
 * Compile the frozen R01 files with symbol renames; do not edit their bodies. */
#include "wf_music_seq.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern bool wf_ref_init(wf_music_seq_t *, const uint8_t *, size_t,
                        wf_music_emit_fn, void *, bool);
extern bool wf_ref_send(wf_music_seq_t *, uint8_t);
extern int wf_ref_run(wf_music_seq_t *, uint64_t, unsigned);

typedef struct { uint64_t hash, count; } digest_t;
static wf_music_seq_t reference, candidate;
static uint8_t rom[65536];
static uint64_t services;

static bool emit(unsigned kind, unsigned reg, unsigned value, uint64_t cycle, void *user)
{
    digest_t *digest = user;
    digest->hash ^= cycle ^ ((uint64_t)kind << 56) ^ ((uint64_t)reg << 48) ^ ((uint64_t)value << 40);
    digest->hash *= UINT64_C(1099511628211);
    ++digest->count;
    return true;
}

static bool service(void *user)
{
    (void)user;
    ++services;
    return true;
}

static void normalize(z80 *cpu)
{
    cpu->userdata = NULL;
    cpu->read_byte = NULL;
    cpu->write_byte = NULL;
    cpu->port_in = NULL;
    cpu->port_out = NULL;
}

static void equal(unsigned command, unsigned second, const digest_t *left, const digest_t *right)
{
    z80 a = reference.cpu, b = candidate.cpu;
    normalize(&a);
    normalize(&b);
    bool same = !memcmp(&a, &b, sizeof a) &&
        !memcmp(reference.ram, candidate.ram, sizeof reference.ram) &&
        reference.cycles == candidate.cycles && reference.last_cpu == candidate.last_cpu &&
        reference.next_tb == candidate.next_tb && reference.ym_addr == candidate.ym_addr &&
        reference.status == candidate.status && reference.tb == candidate.tb &&
        reference.ctl == candidate.ctl && reference.latch == candidate.latch &&
        reference.bank == candidate.bank && reference.command_pending == candidate.command_pending &&
        reference.error == candidate.error && left->hash == right->hash && left->count == right->count;
    if (!same)
        fprintf(stderr, "FAIL command=%02x checkpoint=%us cycles=%" PRIu64 "/%" PRIu64
                " events=%" PRIu64 "/%" PRIu64 " hashes=%016" PRIx64 "/%016" PRIx64 "\n",
                command, second, reference.cycles, candidate.cycles, left->count, right->count,
                left->hash, right->hash);
    assert(same);
}

static void live_command_transitions(void)
{
    static const uint8_t commands[]={1,6,0x12,0x1f,3,0x1f,2,1,0x1f};
    digest_t left={UINT64_C(1469598103934665603),0},right=left;
    assert(wf_ref_init(&reference,rom,sizeof rom,emit,&left,false));
    assert(wf_music_seq_init(&candidate,rom,sizeof rom,emit,&right,true));
    int rc;
    do{rc=wf_ref_run(&reference,2ull*3579545u,20000000);assert(rc>=0);}while(rc);
    do{rc=wf_music_seq_run_serviced(&candidate,2ull*3579545u,4096,service,NULL);assert(rc>=0);}while(rc);
    uint64_t target=reference.cycles;
    for(unsigned i=0;i<sizeof commands;i++){
        /* Command00 is handled by the module's generation reset, not accepted
         * by either ROM sequencer interface. Rejection must not mutate state. */
        assert(!wf_ref_send(&reference,0));
        assert(!wf_music_seq_send(&candidate,0));
        equal(0,i,&left,&right);
        assert(wf_ref_send(&reference,commands[i]));
        assert(wf_music_seq_send(&candidate,commands[i]));
        for(unsigned second=1;second<=3;second++){
            target+=3579545u;
            do{rc=wf_ref_run(&reference,target,20000000);assert(rc>=0);}while(rc);
            do{rc=wf_music_seq_run_serviced(&candidate,target,4096,service,NULL);assert(rc>=0);}while(rc);
            equal(commands[i],i*3+second,&left,&right);
        }
    }
    printf("PASS live ROM command transitions1,6,12,1f,3,1f,2,1,1f events=%" PRIu64
           " hash=%016" PRIx64 " and00 rejection\n",right.count,right.hash);
}

int main(int argc, char **argv)
{
    assert(argc >= 2 && argc <= 4);
    unsigned seconds = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 600;
    unsigned first = argc > 3 ? (unsigned)strtoul(argv[3], NULL, 0) : 1;
    unsigned last = argc > 3 ? first : 18;
    assert(seconds && seconds <= 1200 && first >= 1 && last <= 18);
    FILE *file = fopen(argv[1], "rb");
    assert(file && fread(rom, 1, sizeof rom, file) == sizeof rom);
    assert(fclose(file) == 0);
    uint64_t checkpoints = 0, calls = 0;
    for (unsigned command = first; command <= last; ++command) {
        digest_t left = {UINT64_C(1469598103934665603), 0}, right = left;
        assert(wf_ref_init(&reference, rom, sizeof rom, emit, &left, false));
        assert(wf_music_seq_init(&candidate, rom, sizeof rom, emit, &right, true));
        int rc;
        do { rc = wf_ref_run(&reference, 2ull*3579545u, 20000000); assert(rc >= 0); } while (rc);
        do { rc = wf_music_seq_run_serviced(&candidate, 2ull*3579545u, 4096, service, NULL); assert(rc >= 0); } while (rc);
        equal(command, 0, &left, &right);
        ++checkpoints;
        assert(wf_ref_send(&reference, (uint8_t)command));
        assert(wf_music_seq_send(&candidate, (uint8_t)command));
        uint64_t origin = candidate.cycles, target = origin;
        uint32_t fraction = 0;
        unsigned frame = 0, grid = 512;
        for (unsigned second = 1; second <= seconds + 8; ++second) {
            if (second == seconds + 1) {
                assert(wf_ref_send(&reference, 0x1f));
                assert(wf_music_seq_send(&candidate, 0x1f));
            }
            unsigned checkpoint = second * 44100u;
            while (frame < checkpoint) {
                unsigned next = grid < checkpoint ? grid : checkpoint;
                uint64_t numerator = (uint64_t)(next - frame) * 3579545u + fraction;
                target += numerator / 44100u;
                fraction = (uint32_t)(numerator % 44100u);
                frame = next;
                do {
                    rc = wf_music_seq_run_serviced(&candidate, target, 4096, service, NULL);
                    assert(rc >= 0);
                    ++calls;
                } while (rc);
                if (next == grid) grid += 512;
            }
            do { rc = wf_ref_run(&reference, origin + (uint64_t)second*3579545u, 20000000); assert(rc >= 0); } while (rc);
            equal(command, second, &left, &right);
            ++checkpoints;
        }
        printf("PASS cmd%02x %us+8s fade CPU/RAM/timers/events=%" PRIu64 " hash=%016" PRIx64 "\n",
               command, seconds, right.count, right.hash);
        fflush(stdout);
    }
    printf("PASS serviced R01 differential tracks=%u score_seconds=%u checkpoints=%" PRIu64
           " calls=%" PRIu64 " services=%" PRIu64 "\n", last-first+1, seconds, checkpoints, calls, services);
    live_command_transitions();
    return 0;
}
