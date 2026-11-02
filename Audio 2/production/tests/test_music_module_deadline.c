/* Real sequencer + real module queue; only the external sound bus and its
 * independent clock are modeled. A ROM fetch costs one DSP frame here, so
 * work continues to consume real time even when the worker never idles.
 * This is a scheduler regression, not a Saturn performance measurement. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define WF_MUSIC_MODULE_TEST 1
/* The production SH2 layout assertion belongs to the cross build. This
 * 64-bit host fixture supplies a real, naturally sized hot-state object. */
#define _Static_assert(condition, message)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#ifndef WF_MUSIC_MODULE_SOURCE
#define WF_MUSIC_MODULE_SOURCE "../src/wf_music_module.c"
#endif
#include WF_MUSIC_MODULE_SOURCE
#pragma GCC diagnostic pop
#undef _Static_assert

const uint8_t wf_music_sound_rom[0xc000] = {
    [0]=0x31,[2]=0xc8,[3]=0xc3,[4]=0x3a,
    [0x3a]=0xcd,[0x3b]=0x7a,[0x3d]=0xcd,[0x3e]=0xe0,[0x3f]=3,[0x40]=0xc3,[0x41]=0x3a,
    [0x66]=0x3a,[0x68]=0xe0,[0x69]=0x32,[0x6a]=0x20,[0x6b]=0xc0,[0x6c]=0xed,[0x6d]=0x45,
    [0x7a]=0xc9,[0x3e0]=0xc9
};
/* Compile this fixture with -fno-pie -no-pie. These absolute symbols model the
 * master's fixed linker-owned section; mmap below supplies its RAM. */
__asm__(".global wf_hotcode_start\n.set wf_hotcode_start,0x06004000\n"
        ".global wf_hotcode_end\n.set wf_hotcode_end,0x06004008\n"
        ".global wf_hotcode_load\n.set wf_hotcode_load,0x06004000\n");
wf_music_module_shared_t wf_music_test_shared;

static uint16_t sound_ram[0x80000 / 2], slot_words[512];
static wf_music_hot_t fixture_hot;
static uint8_t test_rom[0xc000];
static uint32_t model_frame, clock_reads, service_calls, largest_gap, previous_service;
static unsigned idle_calls;
static unsigned bus_write_cost;
static bool forbid_idle;
static unsigned lifecycle_stage;
static uint8_t (*original_read)(void *, uint16_t);

/* Weak only so this same behavioral regression runs against the original
 * source, before cooperative service exists. The fallback is the original
 * production scheduler; it is expected to fail the deadline assertion. */
extern int wf_music_seq_run_serviced(wf_music_seq_t *, uint64_t, unsigned,
                                    bool (*)(void *), void *) __attribute__((weak));
extern volatile uint32_t wf_music_fm_discarded_events __attribute__((weak));
extern volatile uint32_t wf_music_fm_discarded_batches __attribute__((weak));
extern volatile uint32_t wf_music_fm_rejected_events __attribute__((weak));
extern volatile uint32_t wf_music_ring_rejected_events __attribute__((weak));

uint16_t *wf_music_test_sound(unsigned address)
{
    assert(!(address & 1u));
    if (address >= 0x100000u) {
        assert(address < 0x100400u);
        model_frame += bus_write_cost;
        return &slot_words[(address - 0x100000u) / 2u];
    }
    assert(address < 0x80000u);
    if (address == 0xb010u) {
        ++clock_reads;
        sound_ram[address / 2u] = (uint16_t)model_frame;
    }
    return &sound_ram[address / 2u];
}

void wf_music_test_idle(void)
{
    ++idle_calls;
    assert(!forbid_idle && "queue saturation entered a blocking wait");
    ++model_frame;
    assert(idle_calls < 10000u);
    if(lifecycle_stage){
        SHARED.master_frame=(uint16_t)(model_frame/735u);
        sound_ram[0xb02au/2u]=sound_ram[0xb024u/2u];
        if(SHARED.state==WF_MUSIC_RUNNING){
            unsigned command=0xffffu;
            if(lifecycle_stage==1){command=6;lifecycle_stage=2;}
            else if(lifecycle_stage==2&&SHARED.consumer==1&&!sequence.command_pending&&sequence.ram[0x20]==6){command=18;lifecycle_stage=3;}
            else if(lifecycle_stage==3&&SHARED.consumer==2&&!sequence.command_pending&&sequence.ram[0x20]==18){command=0;lifecycle_stage=4;}
            else if(lifecycle_stage==4&&SHARED.consumer==3&&!playing){
                assert(sequence.cycles==boot_snapshot.cycles&&!sequence.command_pending);
                command=3;lifecycle_stage=5;
            }else if(lifecycle_stage==5&&SHARED.consumer==4&&!sequence.command_pending&&sequence.ram[0x20]==3){command=0x1f;lifecycle_stage=6;}
            else if(lifecycle_stage==6&&SHARED.consumer==5&&!sequence.command_pending&&sequence.ram[0x20]==0x1f){command=0;lifecycle_stage=7;}
            else if(lifecycle_stage==7&&SHARED.consumer==6&&!playing){SHARED.shutdown=1;lifecycle_stage=8;}
            if(command!=0xffffu){
                SHARED.commands[SHARED.producer&63u]=(uint16_t)command;
                ++SHARED.producer;
            }
        }
    }
}

static uint8_t timed_read(void *user, uint16_t address)
{
    ++model_frame;
    return original_read(user, address);
}

static bool service_queue(void *unused)
{
    (void)unused;
    uint32_t gap = model_frame - previous_service;
    if (gap > largest_gap) largest_gap = gap;
    previous_service = model_frame;
    ++service_calls;
    return fm_flush();
}

static void reset_fixture(void)
{
    static const uint8_t empty_loop[] = {0xcd,0x7a,0,0xcd,0xe0,3,0xc3,0x3a,0};
    memset(&wf_music_test_shared, 0, sizeof wf_music_test_shared);
    memset(sound_ram, 0, sizeof sound_ram);
    memset(slot_words, 0, sizeof slot_words);
    memset(&fixture_hot, 0, sizeof fixture_hot);
    memset(test_rom, 0, sizeof test_rom);
    memcpy(test_rom + 0x3a, empty_loop, sizeof empty_loop);
    hot = &fixture_hot;
    fm_producer = fm_consumer = 0;
    booting = false;
    generation = 1;
    SHARED.state = WF_MUSIC_RUNNING;
    sound_ram[0xb026u / 2u] = 2;
    model_frame = clock_reads = service_calls = largest_gap = previous_service = 0;
    idle_calls = 0;
    bus_write_cost = 0;
    forbid_idle = false;
    lifecycle_stage = 0;
    volatile uint32_t *counters[] = {&wf_music_fm_discarded_events, &wf_music_fm_discarded_batches,
                                    &wf_music_fm_rejected_events, &wf_music_ring_rejected_events};
    for (unsigned i=0; i<4; ++i) if (counters[i]) *counters[i] = 0;
    assert(wf_music_seq_init(&sequence, test_rom, sizeof test_rom, event, NULL, false));
    sequence.cpu.pc = 0x100;
    original_read = sequence.cpu.read_byte;
    sequence.cpu.read_byte = timed_read;
    booting = true;
    wf_scsp_ym_init(&synth, synth_write, NULL);
    booting = false;
    memset(slot_words, 0, sizeof slot_words);
}

static void already_queued_write_is_serviced_during_cpu_work(void)
{
    reset_fixture();
    output_frame = 100;
    assert(synth_write(0x100010u, 0x1234, NULL));
    assert(fm_flush()); /* At enqueue time the write is still 100 Fs early. */
    int rc;
    if (wf_music_seq_run_serviced)
        rc = wf_music_seq_run_serviced(&sequence, 16384, 4096, service_queue, NULL);
    else
        rc = wf_music_seq_run(&sequence, 16384, 4096);
    bool flushed = fm_flush();
    printf("scheduler: work=%u Fs enqueue_due=-100 service_calls=%u max_gap=%u "
           "clock_reads=%u batches=%u delivered=%04x rc=%d flushed=%d\n",
           model_frame, service_calls, largest_gap, clock_reads,
           sound_ram[0xb03au / 2u], slot_words[8], rc, (int)flushed);
    fflush(stdout);
    assert(slot_words[8] == 0x1234 && "on-time FM event starved during sequencer work");
    assert(rc == 0 && flushed && SHARED.state == WF_MUSIC_RUNNING);
    assert(sound_ram[0xb03au / 2u] == 0 && fm_consumer == fm_producer);
    assert(model_frame == 4096 && sequence.steps == 4096);
    assert(largest_gap <= 512 && clock_reads <= 12 && idle_calls == 0);
}

static void overdue_queue_faults_and_counts_every_cancelled_record(void)
{
    reset_fixture();
    output_frame = 65000;
    for (unsigned i=0; i<3; ++i) assert(synth_write(0x100010u + i*2u, 0x1234, NULL));
    model_frame = 67206; /* Timestamp wraps; head is exactly 2206 Fs late. */
    assert(!fm_flush() && "missed FM deadline reported success");
    assert(SHARED.state == WF_MUSIC_FAULT && SHARED.error == 15);
    assert(fm_consumer == fm_producer && sound_ram[0xb03au/2u] == 1);
    assert(wf_music_fm_discarded_events == 3 && wf_music_fm_discarded_batches == 1);
    unsigned old_generation = generation;
    fault(8); /* A callback failure must not overwrite its precise cause. */
    assert(SHARED.error == 15 && generation == old_generation);
    assert(wf_music_fm_discarded_events == 3 && wf_music_fm_discarded_batches == 1);
}

static void full_fm_queue_fails_without_waiting_for_a_future_head(void)
{
    reset_fixture();
    output_frame = 10000;
    for (unsigned i=0; i<512; ++i) assert(synth_write(0x100010u, (uint16_t)i, NULL));
    forbid_idle = true;
    assert(!synth_write(0x100010u, 0xffff, NULL));
    assert(SHARED.state == WF_MUSIC_FAULT && SHARED.error == 16);
    assert(wf_music_fm_discarded_events == 512 && wf_music_fm_discarded_batches == 1);
    assert(wf_music_fm_rejected_events == 1 && idle_calls == 0 && clock_reads <= 1);
}

static void full_pcm_transport_fails_without_blocking_fm_service(void)
{
    reset_fixture();
    sound_ram[0xb020u/2u] = 256;
    forbid_idle = true;
    assert(!publish(0x100u, 0x80u, 100));
    assert(SHARED.state == WF_MUSIC_FAULT && SHARED.error == 17);
    assert(wf_music_ring_rejected_events == 1 && idle_calls == 0);
    assert(sound_ram[0xb020u/2u] == 256 && sound_ram[0xb022u/2u] == 0);
}

static void rejected_current_write_is_counted_when_the_full_queue_is_overdue(void)
{
    reset_fixture();
    output_frame = 100;
    for (unsigned i=0; i<512; ++i) assert(synth_write(0x100010u, (uint16_t)i, NULL));
    model_frame = 2306;
    assert(!synth_write(0x100010u, 0xffff, NULL));
    assert(SHARED.state == WF_MUSIC_FAULT && SHARED.error == 15);
    assert(wf_music_fm_discarded_events == 512 && wf_music_fm_discarded_batches == 1);
    assert(wf_music_fm_rejected_events == 1);
}

static void late_key_metadata_cannot_leave_a_fake_running_worker(void)
{
    reset_fixture();
    model_frame = 2306;
    assert(!publish(0x400u, 7, 100));
    assert(SHARED.state == WF_MUSIC_FAULT && SHARED.error == 18);
    assert(sound_ram[0xb038u/2u] == 1 && wf_music_ring_rejected_events == 1);
    assert(SHARED.events == 0 && sound_ram[0xb020u/2u] == 0);
}

static void long_bus_flush_rechecks_the_independent_clock(void)
{
    reset_fixture();
    output_frame = 0;
    for (unsigned i=0; i<40; ++i) assert(synth_write(0x100010u, (uint16_t)i, NULL));
    bus_write_cost = 70;
    assert(!fm_flush() && "flush used one stale clock sample for every bus write");
    assert(SHARED.state == WF_MUSIC_FAULT && SHARED.error == 15);
    assert(wf_music_fm_discarded_events == 8 && wf_music_fm_discarded_batches == 1);
    assert(clock_reads == 2); /* One clock sample per <=32 writes, not per write. */
}

static void explicit_prepare_recovers_a_fault_into_a_fresh_generation(void)
{
    overdue_queue_faults_and_counts_every_cancelled_record();
    void *workspace = mmap((void *)(uintptr_t)0x06004000u, 18432,
                           PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);
    assert(workspace != MAP_FAILED);
    SHARED.workspace = 0x06004000u;
    wf_music_module_prepare();
    assert(SHARED.state == WF_MUSIC_BOOTING && SHARED.error == 0);
    assert(fm_consumer == 0 && fm_producer == 0 && generation == 1);
    assert(wf_music_fm_discarded_events == 0 && wf_music_fm_discarded_batches == 0);
    assert(wf_music_fm_rejected_events == 0 && wf_music_ring_rejected_events == 0);
    assert(munmap(workspace, 18432) == 0);
    hot = &fixture_hot;
}

static void direct_load_rejects_a_different_workspace_before_touching_ram(void)
{
    reset_fixture();
    void *linked = mmap((void *)(uintptr_t)0x06004000u, 18432,
                        PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);
    void *wrong = mmap((void *)(uintptr_t)0x0600c000u, 18432,
                       PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);
    assert(linked != MAP_FAILED && wrong != MAP_FAILED);
    memset(linked, 0x6b, 18432);
    memset(wrong, 0xa5, 18432);
    SHARED.workspace = 0x0600c000u;
    SHARED.events = 0x12345678u;
    wf_music_module_prepare();
    assert(SHARED.state == WF_MUSIC_FAULT && SHARED.error == 14);
    assert(SHARED.events == 0x12345678u && SHARED.workspace == 0x0600c000u);
    for (unsigned i=0; i<18432; ++i) {
        assert(((uint8_t *)linked)[i] == 0x6b);
        assert(((uint8_t *)wrong)[i] == 0xa5);
    }
    assert(munmap(linked, 18432) == 0 && munmap(wrong, 18432) == 0);
}

static void stop_restart_transition_and_fade_keep_fifo_and_generation_order(void)
{
    reset_fixture();
    void *workspace=mmap((void *)(uintptr_t)0x06004000u,18432,
                         PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    assert(workspace!=MAP_FAILED);
    SHARED.workspace=0x06004000u;
    wf_music_module_prepare();
    assert(SHARED.state==WF_MUSIC_BOOTING);
    lifecycle_stage=1;
    wf_music_module_run_body();
    assert(lifecycle_stage==8&&SHARED.state==0&&!SHARED.error);
    assert(SHARED.producer==6&&SHARED.consumer==6&&SHARED.last_command==0);
    assert(SHARED.generation==6&&generation==7);
    assert(!wf_music_fm_discarded_events&&!wf_music_fm_rejected_events&&!wf_music_ring_rejected_events);
    assert(munmap(workspace,18432)==0);
    hot=&fixture_hot;lifecycle_stage=0;
}

int main(int argc, char **argv)
{
    const char *which = argc > 1 ? argv[1] : "all";
#define RUN(name, function) if (!strcmp(which, "all") || !strcmp(which, name)) { function(); puts("PASS " name); }
    RUN("starvation", already_queued_write_is_serviced_during_cpu_work);
    RUN("deadline", overdue_queue_faults_and_counts_every_cancelled_record);
    RUN("fm-full", full_fm_queue_fails_without_waiting_for_a_future_head);
    RUN("ring-full", full_pcm_transport_fails_without_blocking_fm_service);
    RUN("rejected-current", rejected_current_write_is_counted_when_the_full_queue_is_overdue);
    RUN("late-key", late_key_metadata_cannot_leave_a_fake_running_worker);
    RUN("flush-clock", long_bus_flush_rechecks_the_independent_clock);
    RUN("recovery", explicit_prepare_recovers_a_fault_into_a_fresh_generation);
    RUN("workspace", direct_load_rejects_a_different_workspace_before_touching_ram);
    RUN("lifecycle", stop_restart_transition_and_fade_keep_fifo_and_generation_order);
#undef RUN
    return 0;
}
