# Original frontend on Jo Engine

> Historical development record. Results and measurements below apply to the revisions named in this document. See [current status](STATUS.md) for P1-03B. Some original capture evidence is external and is identified by its original path below.

`port/wf_jo_frontend.c` supplies the transport for the existing native arcade
owners. It runs original RESET, legal plates, title, coin/start, mode and roster
selection, match card and entrances through F8C. It also merges completed
combat back into the same raw cabinet image for results, continuation,
ranking, credits and the original return to BOOT6FC. Neither frontend nor
combat is reset at a scene boundary.

## Storage and initialization

The caller owns `wf_jo_frontend_t` and supplies existing runtime, video, world,
display, rational clock, scene staging, priority and render-count pointers.
The raw arcade RAM is a separate persistent 16 KiB allocation for the entire
session. `wf_r18_scene_resume` accepts that same image as source and destination
and overlays live typed combat state, video allocators, world caches and sprite
records in its defined alias order. There is no second retained 16 KiB copy.
Raw cabinet RAM never aliases the sprite cache or scene preparation storage.

With the current 4 KiB sprite pack, the integrated frontend build reserves
these final LWRAM ranges permanently:

| Range | Owner | Size |
| --- | --- | ---: |
| `0x00272000`–`0x002fbfff` | Decoded sprite cache lease, including its optional temporary scene loan | 552 KiB: 138 × 4 KiB blocks |
| `0x002fc000`–`0x002fffff` | Raw arcade cabinet RAM | 16 KiB |

The scene staging and compressed scene pack retain their existing independent
areas below `0x00272000`. Raw RAM stays allocated during every combat and
transition; the cache is never enlarged over it.
The current ROM, compressed sprite pack and SFX bank occupy 4,073,088 aligned
cartridge bytes. Both music prefixes occupy another 98,304 cartridge bytes,
leaving 22,912 bytes free. No LWRAM prefix lease is needed. The loader decides
this before binding the cache. If an older, larger pack cannot fit both
prefixes in the cartridge, the fallback reserves 32 KiB at `0x002f4000` and
reduces the cache to 520 KiB: 130 current 4 KiB blocks or 65 legacy 8 KiB blocks.
The frozen frontend05 sources, pack header and captured audio counter are
recorded in `evidence/frontend-05-timing-memory.json`.
Fixture `--cache-slots` and `WF_JO_TEST_CACHE_SLOTS` continue to specify fixed
8 KiB allocation units; `--cache-bytes` and `WF_JO_TEST_CACHE_BYTES` specify
the arena directly. Reports derive the actual slot count from the WFSZ header.

Load ROM, compressed sprites, all seven scene packs and the complete font first.
Initialize/prefetch audio afterwards and prime cue02 as the final loading
operation. `wf_jo_frontend_init()` checks resident scenes and initialized audio
before it can run. Scene binds use only the resident asset adapter; no scene
file or font is opened during the timed frontend. Audio streaming remains the
responsibility of the Jo audio owner.

After that audio priming, `wf_jo_frontend_init()` prepares the first
`R18MENU.BIN` from resident scene blocks before its first `wf_jo_clock_start()`.
It calls cooperative audio service after each slice, keeps cue 02 silent and
retains the completed menu until the original tick 4 bind. No ordinary file
reader or early scene publication is introduced. Audio service can still
complete work on its already-owned music reader. The application records
`start_field` from the clock's actual field after initialization, excluding
initial loading; transition clocks are not reset to hide later scene work.

## Incremental resident scene preparation

`port/wf_jo_scene_prepare.c` prepares a canonical scene during existing
frontend waits. It borrows exactly 172,032 bytes from the tail of the decoded
sprite cache: 128 KiB for scene output, 8 KiB for private zlib state and 32 KiB
for the resumable dictionary. It allocates no new heap and does not use the
cartridge, music prefixes, permanent scene staging or raw cabinet RAM.

For the current 138-block frontend lease, the active sprite cache temporarily
contains 96 blocks of 4 KiB. The lent range is `0x002d2000`–`0x002fbfff`;
the remaining active cache is `0x00272000`–`0x002d1fff`. The asset adapter
invalidates both maps of every lent slot before exposing the pointer and
preserves all remaining slots. It rejects a nested loan, a non-block-multiple
size, or a loan leaving less than 8 KiB active. A return restores the lent
slots as empty. These counts describe the current 4 KiB layout; a smaller
valid lease or an 8 KiB sprite pack derives its capacity from its own header.

Each preparation step emits at most 512 bytes. Its private `z_stream` can
remain suspended while ordinary sprite misses use the separate one-shot
inflater. The source is a checked read-only block view into the already
resident `SCENES.WFP`. A scene becomes available only after every block has
reached `Z_STREAM_END` with its checksum, exact decoded length and complete
input consumption validated. The 512-byte output budget bounds work units; it
does not establish a fixed SH-2 cycle or millisecond budget.

The prediction points are existing original waits:

| Current original state | Predicted data |
| --- | --- |
| BOOT at the `1E7C` wait | World 4 / card, `R18MAP4.BIN` |
| AC0 match call `C02` → `7B70`, while world 4 is bound, at its speech waits `7CDE`, `7DFC` or `7E5E` | Arena selected from the existing progression value and world table |

`next_tick()` captures the hint, runs the admitted original IRQs, then performs
one slice only if the generation is unchanged and IRQ work succeeded. It also
services audio after that slice. Preparation does not invoke an original
owner early, change its registers, add a wait, display the predicted scene
or read the CD. An input-driven generation change cancels the old prediction.

At the actual original `bind_world` call, only a complete preparation matching
the requested scene name is copied to scene staging. The loan is then
returned. An incomplete or different prediction is cancelled and the existing
validated synchronous resident loader runs; insufficient loan capacity also
retains this fallback. A corrupt compressed stream remains an explicit error.
`wf_jo_frontend_run()` cancels any remaining preparation before combat handoff,
restoring all 138 slots in the current layout. Callers must cancel preparation
before rebinding or unloading assets; the asset API itself revokes loans on
load/bind. The preparation module checks 8-byte alignment before placing zlib
state in the byte-oriented cache lease.

The ordinary one-shot asset decoder now has a separate 8 KiB HWRAM scratch
for sprite and synchronous scene output/history. It validates the whole
block before copying to the selected LWRAM destination, publishing the cache
mapping and calling the cooperative service hook. This scratch is not part
of the 172,032-byte loan and is not used by its suspended inflater.

Both changes are implemented and have host correctness evidence. The completed
final09 capture analysis (external evidence: `../evidence/final-capture-analysis-09.json`) measures
the combined implementation, including combat cadence, audio counters and the
instrumented post-owner intervals. It does not isolate the effect of each
optimization, establish first-visible transition duration or certify physical
hardware. Whole-game acceptance remains false. The historical baseline 11
measurements in the README remain separate from this implementation.

## Call sequence

1. Initialize Jo video, native presenter, runtime/bindings and HUD/font, then
   finish the resident asset and audio setup described above.
2. Call `wf_jo_frontend_init(front, config, seed)`. The explicit seed and lower
   cold-cabinet bytes are supplied once; original RESET654 clears its own RAM.
   With the current lease, initial menu preparation finishes before this call
   starts the arcade clock; a smaller lease retains the synchronous fallback.
3. Call `wf_jo_frontend_run(front)`. It blocks within Jo's normal application
   callback while original owners and synchronous waits run through F8C.
4. Call `wf_jo_frontend_flush(front)` and return the application callback.
   Start combat on a subsequent callback. Run imports world transport caches,
   priority, original participants and registers, initializes display scroll,
   and reinstates the caller's combat RAM-read callback.
5. During combat, call `wf_jo_frontend_combat_coins(front)` each application
   callback, even if no new combat tick is due. At the admitted IRQ3 seam,
   after the video owners and before ACK, call
   `wf_jo_frontend_combat_start(front, &accepted)`. An accepted attract START
   already performed the original clear and selected players: stop combat,
   then use `run()` without exporting the old actors a second time.
6. After the final combat ACK and drained queues, call
   `wf_jo_frontend_after_combat_at_level(front, owner, 1)`, flush, then use
   `run()` for the next original match or attract route. The explicit1 is
   the virtual HIGH level of the just-admitted IRQ3. A caller resuming after
   a known LOW event must supply0 instead. The compatibility
   `wf_jo_frontend_after_combat` wrapper supplies LOW.

Post entries are11B6 for TAG including its result,122C after an already shown
TAG result,1AFC for Royal including GAME OVER,1B1E after an already shown Royal
result,1120 for an attract ending, and1B4E for the interactive credits. Missing
owners or boundaries stay explicit failures. Generation changes cancel the
old match/post cursor so a credited START cannot replay an abandoned prefix.

The combat coin bridge runs the existing IRQ2 owner at17 calls per rational
arcade period on its own physical-field clock. Its persistent pulse counters
remain in the same raw cabinet image, while only coinage and credit bytes are
synchronized with typed combat state. It cannot refund a credit spent by a
combat participant owner. Paid-match START stays with the original gameplay
owners; the IRQ3 bridge calls978 only in attract. A full scene export occurs
only for a pressed START that can pay, followed by the existing8AE continuation.
The combat callback samples the four physical pads once when IRQ2 work is
pending; every original IRQ2 and its independent input read still executes.
`wf_r18_frontend_coin_run` drains the same owner without intermediate MORE
returns. It preserves the public step API, blocked/resume PCs, ordered MMIO
and CPU/RAM effects; it does not skip idle interrupts or replace coin pulses
with an elapsed-time credit shortcut.

The optional input callback receives ordinary physical cabinet levels. The
production default maps Jo pads, C coin inputs and DIP0300. Frontend UP/DOWN
bits follow the original menu order. Tests use the callback solely for
controller levels; it does not create participants or alter selection RAM.

## Timing and publication

The shared clock advances by37,609 arcade units per39,168 Saturn-field units.
Every consumed arcade tick admits exactly17 raster IRQ2 deliveries when the
emulated interrupt mask permits, followed by one admitted IRQ3. The original
`on_vblank` function invokes the platform commit; the adapter never invokes
that IRQ or commit a second time for the same tick. Portrait and speech
divisors are both1.

High/low waits expose the arcade field level. A high edge advances the rational
clock only when the old arcade level was low; a low wait consumes that level
without adding another field. `wait_vblank` always requests the next arcade
tick. Physical fields without a new arcade tick repeat the current complete
image. When work spans several physical fields, the clock retains pending
arcade ticks. Before admitting another IRQ3, the transport publishes and closes
any completed changed records, palette, map, scroll or layer state. An older
queued tag is closed first. Static unchanged states may consume debt directly;
changed animation states are never collapsed to conceal a late renderer.

Sprite callbacks copy into the existing presenter record workspace and defer
hardware submission. Palette/map callbacks stage state. Immediately before
`__jo_core_synch`, the adapter publishes the completed staged state once, and
does so before another arcade tick can replace a changed state.
If the application already queued that Jo tag, the wait closes it and keeps
the newer records for the next tag. Public `flush()` returns WF_BLOCKED for
that specific retryable case; native synchronous wait callbacks never treat
WF_BLOCKED as a yield.

Scene clear hides layers and sprites without stopping TVMD. A changed map
stages an empty list, replaces character data while hidden and invalidates
the pack's initial foreground/background palette acknowledgment. Only a
subsequent complete sprite/map/live-palette frame releases the gate. Legal
plates expose only an opaque HUD plane. World scenes restore their native
transparent/opaque plane priorities.

After combat, the clock anchors to the current physical field while retaining
its fractional remainder. It does not replay combat's elapsed interrupts into
the frontend. Result/credits level requests synchronize A registers around
the exact wait. Their original waits are not supplemented with loading or
transition delays.

Royal loss1AFC reaches2A0A without a LOW wait after the combat ACK. Under the
adapter's virtual field contract its initial level is therefore HIGH; the
conditional2A06 owner adds no edge there. A LOW entry waits for the next HIGH.
The native owner consequently takes97 or98 logical ticks, respectively. This
is an outcome of the waits, not a fixed duration in the adapter. The time
spent exporting RAM on the SH-2 cannot establish the original arcade input
level. Cycle/level capture on the arcade remains the separate physical-phase
reference; these tests establish the stated virtual timing contract.

## Verification and remaining hardware gate

`tests/run_jo_frontend_tests.sh` executes this adapter with the real native
owners, resident assets and Jo crop/command backend. It verifies:

- The complete intro reaches title at tick1172.
- The deterministic paid TAG route reaches F8C at tick4510; Royal at tick1771.
- Each tick produces17 raster IRQs and one VBlank IRQ in these fixture states.
- Rational physical-field counts are exact at one field per sync. A deliberately
  slow three-field transport preserves the clock/debt equation and every dirty
  state; its increased physical duration remains visible. A stress fixture
  changes all1,771 IRQ3 snapshots and proves none is replaced before publication.
- A prequeued tag and a second staged submit never duplicate Jo commands.
- Raw RAM aliasing with scene storage is rejected, and scene reads cause no CD
  access after loading.
- Both real F8C routes resume through interactive credits to BOOT6FC in2504
  ticks; Royal loss is tested with both HIGH97 and LOW98 entries. The tests
  cover A registers, generation, exact IRQ counts and clock reanchoring.
- Original coin pulse/release rules produce one312A per counted credit, held
  coin does not repeat, and typed credit spending cannot be overwritten by
  stale raw RAM. Paid START stays in gameplay; real attract F8C can reach the
  original978→8AE→52BE acceptance path.
- The whole coin runner and step API match the intact ROM in 6,534 input/RAM
  cases each, including actual frontend04/05 snapshots. A separate sanitized
  test covers 4,929 run/step comparisons and 1,710 rejected-event resumptions.
- A prepared sound command receives cooperative audio service immediately
  after its14000C write, before a following scene clear or video publication.

After importing the delivered disc assets, the source package can reproduce
the incremental preparation checks with its included canonical references:

```sh
python3 tests/test_scene_prepare.py \
  --source-dir source_assets --asset-dir cd --sanitize
```

The recorded 4 KiB (external evidence: `../evidence/scene-prepare-final-4k-tests.log`) and
8 KiB (external evidence: `../evidence/scene-prepare-final-8k-tests.log`) runs each completed all six
byte-identical scenes in 1,387 bounded steps of at most 512 output bytes,
with sprite reads and evictions
between slices. They also cover cancellation, checksum failure, alignment,
lease recovery and zero CD reads after initialization. The
frontend fixture (external evidence: `../evidence/frontend-final-loading-tests.log`) checks the
integrated original logical waits and endpoints. The lower-level loan/view
bounds checks are in assets-cache-loan-test.json (external evidence: `../evidence/assets-cache-loan-test.json`).
The README gives the command to regenerate an isolated 8 KiB fixture directory;
it does not replace the current disc assets. Evidence files are present in a
source ZIP only when selected in its package manifest; the test sources are
included regardless.

`wf_jo_front_diagnostic[8]` contains phase, original PC, arcade ticks, physical
fields, raster IRQ count, VBlank IRQ count, publications and pending debt.
These32bytes support captures while the application callback is inside a
synchronous scene. The host tests establish ownership and original logical
durations. Saturn/emulator captures must still establish visible timing and
audio behavior under actual device and rendering load.
