# Combat integration gate — native TAG / Royal

**Selected candidate checkpoint, 2026-11-01: integration reproduced; audio and cadence acceptance remain failed.** The complete original TAG replay with scalar9/sequence `-Os` and firmware `cddd1b01…` preserves the exact game state and all 64 producers. It records **0 PCM underruns, 0 FM losses and 1 late transport event**. Enabled audio runs at **55.33036 logical steps/s**, versus **57.90184** for the matched control, adding **4.6475% combat time**. The isolated SH2 improvement does not translate into an improved full-combat rate. Neither waveform fidelity nor hardware readiness is claimed. Earlier failed firmware10 evidence is retained below and clearly identified.

## Selected full TAG comparison

The final pair uses the immutable source snapshot under `audio2-build/combat-prerequisites/integration/final-scalar9-cddd/source/`, the original TAG tape and firmware SHA256 `cddd1b01a4e277a8d8f82f16445dbc81dd8e50ab5a13c133fdeb25ef8353c090`. Both profiles use the same linked game; their ELFs differ at exactly one writable data byte to enable or disable Audio2. All 345 original game files and graphics assets are unchanged. Both captures use standalone Mednafen 1.32.1, normal presentation speed, no frame skipping and no expansion cartridge. They reach the original frame-7372 endpoint without missed cadence observations or gameplay faults.

| Combat-only result | Control | Selected Audio2 |
|---|---:|---:|
| Logical interval | 0 → 7372 | 0 → 7372 |
| VBlank interval | 738 → 8355 | 1124 → 9095 |
| Elapsed VBlanks | 7,617 | 7,971 |
| Measured duration | 127.318932 s | 133.236078 s |
| Logical steps/s | 57.901837 | 55.330359 |
| Gap of 1 VBlank | 7,127 | 6,779 |
| Gap of 2 VBlanks | 245 | 587 |
| Gap of 3 VBlanks | 0 | 6 |
| Median / P95 / P99 gap, VBlanks | 1 / 1 / 2 | 1 / 2 / 2 |
| Maximum gap | 2 = 33.4302 ms | 3 = 50.1453 ms |

Both native MOV recordings independently give 59.8261379176 video fields/s from the last 1000 complete video packet durations, totaling 737136 samples at 44100 Hz. The table excludes BIOS/CD loading and uses the actual first/last combat VBlank. Audio adds **354 VBlanks or 5.917146 seconds**, a **4.647499%** duration increase and **4.441099%** logical-throughput decrease. The approximately 57.44485 logical-steps/s target is not reached. The previous firmware10 pair measured 55.337301 logical steps/s; the selected full pair is effectively unchanged, one combat VBlank slower. That comparison changes several audio components and does not isolate the scalar optimization's contribution.

The complete 7648-byte runtime and all 2412 bytes of actor records match the control byte-for-byte. Their SHA256 values remain `5a6c73b1c1362e82b9faf0eb1f268f2231e6331a2044b95e73a04f70bfc6b87b` and `4fafe5f3c6044dfe01da3499f6476e67a540b7dba6fd94d7b188d4562e3a33a2`. The full ordered 64-word producer trace matches, including logical frames and original initial word `0x3104`, with hash 3024624743. All 64 words are accepted and completed; queue peak is 1, with no overflow, unsupported word or skipped word. API and worker remain RUNNING with error 0 through the endpoint. The rates describe this active but faulty candidate; the late event prevents certification of the cost of complete, on-time audio work. Analyzer v2 therefore reports `full_match_audio_cost_comparable=false` even though `audio_remained_operational=true`.

| Selected native audio observation | Value |
|---|---:|
| Actual 32-bit PCM underruns | 0 |
| FM discarded events / batches | 0 / 0 |
| FM rejected events / ring rejected events | 0 / 0 |
| Invalid FM batches | 0 |
| Transport late counter `B028` | **1 — failed gate** |
| Last poll exceeding 512 SCSP samples, `B034` | 0 |
| SFX engine fault / worker error / API error | 0 / 0 / 0 |

The actual firmware layout was cross-compiled from the same frozen headers and macro profile, including `WF_MUSIC_TIMING=0`; the underrun field is not inferred from a mailbox location. All three loaded firmware segments match their complete expected payloads. The complete 83156-byte cold module and 14624-byte hot code/constant region also remain intact. Code/constant plus state uses **17844 of 18432 bytes**, the unchanged slave stack reserves 8192 bytes, and 63424 bytes remain for the heap. The observed modified stack span is 544 bytes; this is evidence for this run, not a universal overflow proof. Module CD payload size is 98036 bytes; the original 440710-byte SFX pack still uses ephemeral pregraphics staging.

Maximum API mail latency is 33334 nominal microseconds, or two quantized VBlank increments. The actual video period is 16.715102 ms. These values do **not** measure FM write lateness, DAC latency or precise jitter. The selected uninstrumented module has no exhaustive FM-lateness histogram, and this final pair adds no new master-call timing probe. The earlier private master-service measurements below belong to their explicit legacy fixture and must not be relabeled as current-candidate CPU cost.

Current complete evidence is `audio2-build/combat-prerequisites/integration/final-scalar9-cddd/COMBAT-RESULT.v2.json`; the source-only copy is `evidence/tag7372-final-scalar9-cddd.v2.json`. The original v1 reports remain preserved but their worker-only comparability flag is superseded. The private directory also retains the exact build/capture/analyzer commands, immutable source manifest, maps, symbol/layout manifests, raw native MOV/WAV, F5 state, RAM and video timestamps. The input tape SHA256 is `2dcb67568e0dcace7f552cfbb96a64233c81b74dd5774b4b900a6da1cdc9f0dc`.

`build/audio2-fullmatch/tag-final-scalar9-cddd-30s.wav` is an exact PCM-frame copy of raw native seconds 50–80, stereo 16-bit at 44100 Hz. No gain, normalization, filtering, resampling or new mixing is applied. Its SHA256 is `5612f424dd442cf147792943d28dac04819d1c5484ca6311ad472847a26c2fcc`; the preserved complete source WAV is `e2c43fa56337935d45518d9665bdfd5a614c9ad87fee5b6a14a240ef5835dc25`. The adjacent JSON records the exact frame range and verifies PCM-byte identity. This is a listening artifact from a known-failed candidate; the whole-run late counter does not locate that event inside the excerpt.

### Analyzer v2 correction, 2026-11-02

Independent review demonstrated that the v1 `full_match_audio_cost_comparable` predicate only checked API/worker survival. It incorrectly remained true for the actual TAG final with one late event, the earlier TAG run with19 underruns/22 late events, and a synthetic damaged-module case. The actual final TAG assertion is preserved as RED in `audio2-build/capture-audit-v2/combat-analyzer/red.json`. No capture or emulator was rerun for this correction.

V2 separates worker survival, complete producer delivery, zero native losses/deadline failures and immutable code identity. Cost comparability requires all four, the full original endpoint, identical game/actor/producer work and identical emulator/BIOS. Missing counters or payload checks fail closed. `integration/test_combat_analysis.py` passes47 cases, including missing/nonzero counters, incomplete commands, retained queue entries, code mismatches and missing/changed/ambiguous actual ELF symbols.

Every symbol used to read game/queue/native counters is verified against the real linked SH2 or68000 ELF, including local/LTO definitions. Master ownership addresses are checked against its linker symbols; the debug module's destination/size/flags/payloads must match the packed module. Native memory confirms the nonwritable master executable sections, complete module payloads and complete firmware payloads. Writable shared state is excluded from immutable-section comparisons. These byte checks pass in all three final pairs.

| Final pair | API/worker operational | All producers delivered | Zero loss/deadline failures | Immutable code verified | Full correct-work cost comparable |
|---|---|---|---|---|---|
| TAG | true | true | **false:1 late** | true | **false** |
| Royal base | **false** | **false** | **false** | true | **false** |
| Royal4MiB | true | true | **false:6 late** | true | **false** |

The cadence values remain actual observations. TAG and Royal4MiB run with the worker active throughout; base Royal additionally includes a long interval after its worker stopped. None of those distinctions can turn a late/lost-audio candidate into a certified correct-work performance result. The final Royal captures use the same cddd/scalar9 snapshot; their complete current results and input-policy limitation are in `ROYAL-INTEGRATION-GATE.md`.

## Source and scope

The user-provided `project_sources/39-wp1979-fuentes-pruebas.zip` contains the complete native gameplay, reference runtime, input fixture, stage/graphics assets and build contracts. Its selected 1100-file extraction is private under `audio2-build/combat-prerequisites/wp1979`; `extraction-manifest.json` and `source-integrity.json` preserve original input hashes. The frozen master export contains audio hooks, not a complete independently buildable game, so it alone cannot establish combat performance.

The native profile starts a genuine four-wrestler TAG scene with original seed 1085 and active actor IDs 0, 1, 9 and 7. The unchanged `wp1979_input.c` and `wp1979_tape.inc` apply the archived input by logical game frame. The tape has 1761 entries, from frame 0 through 7344. Its SHA256 is `2dcb67568e0dcace7f552cfbb96a64233c81b74dd5774b4b900a6da1cdc9f0dc`.

All integration changes are restricted to audio binding, audio module loader/build, explicit audio linker ownership and common read-only audio diagnostics. The private copy retains 345 original game build/source files byte-for-byte. Graphics assets remain unchanged. Main, WP1979/80 and historical source trees were not edited. No expansion RAM, external timing changes, alternate Saturn emulator, gameplay simplification or synthetic four-sprite stress scene was used.

## Loader and memory contract

The original game audio implementation expects ABI2. The production static harness's ABI3 tag is not a supported dynamic interchange. Integration instead uses the previously validated strict four-segment ABI4 loader, excludes every old ABI2 audio object, and checks the exact master linker-owned hot address before any destination write. The loader revalidates the plan at copy time. Host checks exercise 351 malformed images and confirm no partial writes.

Frozen build10 ownership:

| Region | Range | Usage |
|---|---|---|
| Existing VDP1 cold audio | `0x05c5c000..0x05c7c000` | 83,172 bytes of code/rodata/48 KiB sound program |
| Existing VDP1 audio BSS | `0x05c7c000..0x05c7f000` | 6,336 bytes, bounded by original 12 KiB reserve |
| Existing shared page | `0x05c7f000..0x05c80000` | 176-byte ABI structure, uncached at `0x25c7f000` |
| New linker-owned HWRAM hot | `0x060ea040..0x060ee840` | 14,660 bytes code/const +3,220 bytes state =17,880/18,432 bytes |
| New linker-owned slave stack | `0x060ee840..0x060f0840` | 8,192 bytes |
| User heap start | `0x060f0840` | 63,424 bytes remain before HWRAM end |

The linker advances `___end` beyond both owned HWRAM regions. The slave stack definition and CD boot header agree with `___audio2_slave_stack_top`; the SDK's second stack-symbol definition was explicitly accounted for. This avoids putting the audio worker into a heap-owned or guessed stack range. No gameplay/graphics owner of the slave was found in this profile; its previous owner was the old audio module.

The original graphics VDP1 dynamic allocation ends at `0x05c5b580`, below the existing `0x05c5c000` audio tail. At the end of the actual match, all 83,172 cold module bytes and 14,660 HWRAM code/constant bytes still match their ELF payloads exactly. The slave-stack canary retains an untouched 7,640-byte prefix; the modified span from the top is 552 bytes. This is observed high-water evidence, not a general proof against every possible future overflow.

Audio boot temporarily leases the original tile-cache region `0x00261400..0x00300000` (650,240 bytes), before graphics initializes it. The module is 98,088 bytes and the SFX pack 440,710 bytes; they reuse this lease sequentially. The 53,524-byte SCSP firmware arrives in the original 128 KiB scene staging buffer. Assets are streamed from CD; a static 500 KiB embedded audio pack is not made resident in HWRAM. The game subsequently reuses the tile cache. The adapter therefore supports cold restart only; recovery after reuse is forbidden until a separate owned-resource design exists.

## Earlier exact matched experiment — firmware10

The integrated game is linked once. The control ELF changes one writable data byte, `wf_audio2_enabled=1` to `0`. Executable code, addresses, reserves, inputs and graphics are otherwise identical. Both builds retain the same producer trace and read-only VBlank cadence observer. The observer starts at logical frame 0 and is sampled by the existing actual audio VBlank callback. It does not infer timing from draw count or host polling.

Each run ends when the observer has captured logical frame 7372. Both observe all 7372 steps: no missed steps, counter regressions or fatal gameplay dependency. Startup/CD loading is outside the measured window. Video frequency is derived from the last 1000 complete native video packet durations: 737,136 audio ticks at 44,100 Hz, or **59.8261379 video fields/s**. The rounded video metadata and nominal 60 Hz are not used.

| Combat-only result | Control | Audio2 firmware10 |
|---|---:|---:|
| Logical frame interval | 0 → 7372 | 0 → 7372 |
| VBlank interval | 738 → 8355 | 1125 → 9095 |
| Elapsed VBlanks | 7,617 | 7,970 |
| Measured duration | 127.318932 s | 133.219363 s |
| Logical steps/s | 57.901837 | 55.337301 |
| Gap 1 VBlank | 7,127 | 6,780 |
| Gap 2 VBlanks | 245 | 586 |
| Gap 3 VBlanks | 0 | 6 |
| Median / P95 / P99 gap | 1 / 1 / 2 | 1 / 2 / 2 |
| Maximum gap | 2 =33.4302 ms | 3 =50.1453 ms |

The enabled candidate adds **353 VBlanks, 5.900431 seconds, or 4.6344% combat time**; logical throughput falls 4.4291%. Against the stated arcade target of approximately 57.44485 logical steps/s, the enabled candidate falls short. No game frame pacing was adjusted to hide that result. The earlier 300-frame firmware07 comparison added only four fields and was insufficient to characterize the complete match.

The complete 7,648-byte game runtime is identical between the two final native memory dumps, SHA256 `5a6c73b1c1362e82b9faf0eb1f268f2231e6331a2044b95e73a04f70bfc6b87b`. All nine actor records also match, SHA256 `4fafe5f3c6044dfe01da3499f6476e67a540b7dba6fd94d7b188d4562e3a33a2`. All **64 original `(logical frame, raw audio word)` producers** match in order, with hash `3024624743`. The original initial word is `0x3104`; no music/SFX producer was fabricated.

## Earlier audio failure exposed by combat — firmware10

Audio2 accepts and completes all 64 producers with queue peak 1, no overflow, unsupported command or skipped word. The API and worker remain RUNNING without errors. FM discarded events, discarded batches, rejected FM events, rejected ring events and invalid FM batches are all zero. Those facts do **not** imply an intact mixed audio result:

| Native observed counter | Value |
|---|---:|
| Actual `wf_sfx_engine.underruns`, 32-bit | **19** |
| Ring `B028` late events, 16-bit | **22** |
| Last poll gap exceeding 512 SCSP samples, `B034` | **546 samples** |
| Playing mask at that long gap, `B036` | **7: SFX voices 0, 1, 2** |
| SFX engine fault flag | 0 |

The m68k-compiled layout places the real underrun field at engine offset 32, engine address `0xfe50` for this firmware. The independent existing capture analyzer confirms 19 from the native state and verifies the firmware identity. `C05A` is not the underrun counter. All three loaded firmware code/wave segments also match their complete file payloads. The long-gap mask is evidence of a blocking interval while three game SFX voices are active, but a separate first-incident probe shows that it is not the only failure mode.

### First incident, independently observed

A second run of the **same unmodified firmware10 ISO** stops when the real 32-bit underrun counter first becomes nonzero. A standalone host helper compiled against the exact configured upstream Mednafen headers gives `sizeof(SS_SCSP)=0x1015c0`, matching the native executable's symbol size, and `offsetof(RAM)=0x15c0`. The observer reads `/proc/<child>/mem` only; the complete 512 KiB paused Sound RAM matches the native F5 state exactly. No emulator, game or audio code was patched.

The last observed zero occurs at logical frame **1118**, video clock 2290, SFX DSP frame **1,411,639**. The first observed one occurs at logical frame **1120**, video clock 2292, SFX DSP frame **1,412,493**. These external polls are 20.710 ms apart; the onset is bounded by those observations, not claimed as a sample-exact timestamp. Voice 1 stops at `played=2041`, `write_at=2048`; voices 0 and 3 continue. Both long-poll words remain zero. This first failure is therefore **per-voice refill starvation**, distinct from the later global pause of 546 samples.

The original producer trace contains two `0x002c` words at frame **1103**. The first transport-late counter increase, 0→2, is observed between logical frames 1103 and 1107. The last producer has not changed by the first underrun. This supplies a concrete short regression fixture with simultaneous SFX voices 0/1 and music voice 3; it does not require invented producers or gameplay changes. The exact first-incident bounds and surrounding queue/PCM observations are retained in `build10/first-underrun-observation.json` and `build10/audio2-first-underrun/report.json`.

Two initial private trace labels named `mail_request`/`mail_ack` point to the mailbox magic words; their values are excluded from all conclusions. The actual underrun, PCM, sequence, ring and game-frame addresses are correct and the complete Sound RAM comparison verifies the read mapping. Separating synchronous sample decoding from bus contention and correcting both failure modes remains audio-owner work.

The API's maximum mail latency reads 33,334 nominal microseconds. This adapter clock advances in whole VBlanks at a nominal 16,667 microseconds each; it is **quantized API telemetry, not a precise jitter measurement**. The cadence table above uses the independent native recording clock.

## Earlier evidence checkpoint and hashes — firmware10

Private evidence root: `audio2-build/combat-prerequisites/integration/build10/`.

- `integration-build.json`, `input-manifest.json`, `commands.json`, `master-symbols.json`, `module-symbols.json`, `module-layout.txt`: source/build/ownership contract.
- `control-fullmatch/` and `audio2-fullmatch/`: raw native MOV/WAV, F5 state, restored RAM, immutable capture command/report and video packet timestamps.
- `pair7372-legacy-profile.json`: exact comparison, all 64 original producers, cadence histograms and loaded-byte checks.
- `audio2-fullmatch/sfx-state-report.json`: independent actual 32-bit SFX counter analysis and raw WAV statistics.
- `audio2-first-underrun/`, `first-underrun-observation.json`: read-only native Sound RAM observation, first-fault bounds, complete live/F5 RAM match and unchanged candidate identity.
- `audio2-fullmatch/tag-combat-legacy-profile-30s.wav` and matching JSON: exact unchanged PCM copy of raw native WAV seconds 50–80. This is a **failed-candidate listening artifact**, not a final fidelity demonstration. The raw WAV remains preserved.

| Artifact | SHA256 |
|---|---|
| Standalone Mednafen 1.32.1 | `a12628d798edf9ad1ec7497cec666ee64b583a09fc48a0c650702c8aeb3a7689` |
| Firmware10 SCSP.ELF | `b5ecee6e3e2a0efe7c13f1c6f4cb89bbc6b18cb96db23a57203b20e4a2b06b66` |
| Audio ISO | `330f7b6f95b80741da82f29bee42a1f2e5192435f08dbd74ea55bdf57ef67220` |
| Control ISO | `bd64be88fa2abbfc427629e8ee1e2d49c345d915957103b82ccf51a518e2b246` |
| Audio raw native WAV | `a98a72549fb62e19b701705364bf07f6f05402747b1f0594562e550319224d02` |
| Exact 30-second WAV excerpt | `7ee3e7789df01df86aece99f965dff36f4ccabc6e0388f4f336545a55709a505` |

`production/integration/README.md` gives the fresh private build, exact capture and analyzer commands. Its source-only adapter, strict ABI4 loader, build tools and focused tests are published for review. Queue tests cover original raw-word preservation, same-frame order, BUSY retention, unsupported commands and capacity 64. Cadence tests cover excluding startup/repeated fields, exact gaps, missed-step disclosure and regression detection. These checks validate contracts; the native failure above remains a failed acceptance gate.

## Royal prerequisites — archive-specific scope

The archived normal frontend references `ROYSPR.BIN`, which is absent from the staged archive. The archived generator successfully reconstructs it privately from `validation/runtime18-royal-tiles.txt` and `audit/runtime18-cd/SPRITES.PAK`: 7,669 tiles, 769,012 bytes, SHA256 `a408fb7ecc1ddd2902b02128cf704a4d6121c231a8fc868d0f1a4a0c730700d7`. No historical or gameplay source edit is necessary for that asset.

The supplied **WP1979 archive** entry and recorded tape explicitly boot TAG; no deterministic Royal direct entry or matching Royal tape was found in that archive. This finding does not apply globally to all provided releases. A later source review located the separate **WP1980 HW1** Royal fixture, including original `ROYINIT.BIN`, deterministic `wp1980_input` and an 8474-step GAME OVER endpoint. The 1444-file archive has since been extracted pristine under `audio2-build/royal-prerequisites/pristine/wp1980`; its audio-only adapter/build integration is a separate ongoing task. Its historical baseline is already below the stated arcade cadence, so its own matched control must be measured before attributing any additional cost to Audio2.

No Royal comparison is claimed by the TAG evidence above. Entering an unverified menu route or merely displaying four characters would not establish an identical-input Royal comparison. Royal performance with this candidate, final audio fidelity and real Saturn hardware remain unmeasured at this checkpoint.

### Independent Royal VDP2 ownership audit

The original HW1 `Makefile.wp1980` selects `src/wp1980_combat_entry.c`, no expansion cart, and 3584 VDP1 texture slots. `wf_saturn_yaul_backend.c` places that pool at offsets `0x08020..0x78020`; it overlaps TAG's audio tail. The original compile-time `WP1980_DISABLE_MUSIC=1` guard is correct and must remain. A VDP1-based Audio2 module is ineligible for this Royal profile.

The independently audited alternative uses the otherwise unowned tail of **VDP2 bank A1 (bank 1)**. Bounds below are half-open physical address intervals. The corresponding uncached CPU alias adds `0x20000000`.

| Existing or proposed owner | Physical interval | Bytes |
|---|---|---:|
| Existing FG name reservation | `0x05e20000..0x05e22000` | 8192 |
| Existing BG name reservation | `0x05e22000..0x05e24000` | 8192 |
| Existing HUD names | `0x05e24000..0x05e26000` | 8192 |
| Proposed Audio2 cold code and ROM | `0x05e26000..0x05e3b000` | 86016 (84 KiB) |
| Proposed Audio2 BSS | `0x05e3b000..0x05e3e000` | 12288 |
| Proposed Audio2 shared page | `0x05e3e000..0x05e3f000` | 4096 |
| Untouched guard | `0x05e3f000..0x05e3fffe` | 4094 |
| Existing back color | `0x05e3fffe..0x05e40000` | 2 |

This is an ownership gate for an isolated native experiment, not a runtime pass. The previously measured 83172-byte cold module fits the proposed 86016-byte capacity with 2844 bytes remaining; every newly linked module must independently satisfy the bounds.

**Write ownership.** `include/wf_saturn_memory.h` declares all three name bases and the back-color word. `wf_r18_ring.c` selects foreground and background character patterns in banks 0 and 2, respectively. The validated ring pack has at most 1024 characters of 128 bytes each, so those uploads cannot spill into bank 1. Its normal name writer has 1024 `uint16_t` cells per layer, staying inside the larger reserved intervals above. `wf_r18_hud_map_update()` iterates 2048 source cells and writes both `cell` and `cell+2048`; its largest write ends exactly at bank-1 offset `0x6000`. The optional rate overlay writes three cells within the same HUD names. The failure bitmap is in bank 0. The source scan found no game VDP2 DMA or whole-bank clear: sprite DMA destinations remain VDP1, font uploads are confined to bank 3, and color updates address separate CRAM.

**Initialization and restart.** In the actual combat entry, `wf_saturn_yaul_video_init()` and the complete bank-3 font upload precede `combat_audio_boot()`. `combat_load_initial()` subsequently binds the ring, uploads only banks 0/2, and commits the unchanged VDP2 cycle pattern. Royal's local restart returns to `royal_restart`, after the one-time audio boot, and repeats these bounded asset operations. It does not reinitialize VDP2 wholesale. The installed Yaul archive does contain a 512 KiB VDP2 DMA clear in `___vdp2_vram_init`; disassembly of the exact SDK `init.o` proves that `___sys_init` calls `___vdp_init` before `_user_init`, hence before the module is loaded. The game's video-init wrapper does not call that SDK initializer again.

**CPU access.** The exact SDK declares `VDP2_VRAM_ADDR(1, offset)` as `0x25e20000+offset`, and its default VRAM-control data is partition mode 3, splitting both A and B banks. The original ring installs bank-1 cycle pattern `0x012ffffe`: three name reads, an idle T6 and CPU read/write at T7. This pattern must remain unchanged. The exact Mednafen 1.32.1 VDP2/SCU implementation accepts byte and word RAM writes, masks byte lanes, and exposes word reads through the B bus. The integration loader already writes through aligned 16-bit stores. Cached cold execution and the uncached shared alias must retain the adapter's cache purge and synchronization rules; changing the address does not remove that obligation.

**Remaining native gate.** Use a separate strict ABI4 profile with the new cold/BSS/shared bounds and reject the TAG profile before any writes. Verify native heartbeat/progress after the original ring setup, exact cold/hot file bytes after gameplay, untouched guard and back color, and identical runtime/graphics versus the matched control. B-bus access cost may differ from VDP1; throughput remains a measured question. No graphics pool, layer, cycle pattern or gameplay change is needed to test this candidate.

The private evidence checkpoint is `audio2-build/royal-prerequisites/vdp2-ownership-audit/`: `audit.json` records 13 exact source/SDK hashes and every interval; the relevant SDK startup, VRAM-init, back-color and partition-data disassemblies are preserved beside it. No original game file was edited during this audit.

The subsequent base-Saturn Royal link exposes a **separate HWRAM blocker**. In `integration-bf2/game/audit/audio2-build/audio2-royal.map`, the unchanged game plus audio adapter ends BSS/uncached storage at aligned address `0x060fca40`. Adding the same 18432-byte hot region and 8192-byte slave stack would end at `0x06103240`, outside HWRAM; retaining the explicit 4096-byte heap guard leaves a 16960-byte deficit. The VDP2 cold-region audit does not resolve that HWRAM failure. Base-Saturn Royal Audio2 is therefore still blocked with the current allocation; no claim that its cartridge is optional follows from this audit.

A further source-only alternative remains untested: the original Royal VDP1 texture pool ends at `0x05c78020`, leaving 32736 bytes below `0x05c80000`. The existing `vdp1_vram_partitions_set(1024,0x53560,0,0)` allocates no Gouraud or CLUT table, every emitted sprite keeps `cmd_grda=0`, and the normal/ROM DMA writers remain bounded below that tail. This region could physically hold an 18 KiB audio hot region and 8 KiB slave stack without altering graphics. The present API deliberately rejects a non-HWRAM hot workspace, and placement there would change bus cost. Such an experiment needs a new explicit ownership profile, strict loader/API agreement and native timing, stack and payload validation before use. It is not part of the accepted TAG memory profile or a demonstrated Royal solution.

The selected follow-up experiment moves **only the 18 KiB hot region** to `0x05c78020..0x05c7c820`. The dedicated 8 KiB slave stack stays in HWRAM at `0x060fca40..0x060fea40`, leaving 5568 bytes for the heap, above the existing 4096-byte guard. Another 14304 bytes of the VDP1 tail remain unused. This variant satisfies the independent static graphics-ownership audit and avoids a VRAM stack. A profile-specific exact workspace check, identical loader/module addresses and native validation are still required; the failed HWRAM-only build remains preserved.

The later **actual Royal captures** supersede the unmeasured runtime status of these prerequisite paragraphs; see `ROYAL-INTEGRATION-GATE.md` and its two `COMBAT-RESULT.json` files. Both memory profiles run the identical 8474-step game state and original 133 producers, but both audio workers fault. The base profile accepts only18/133 words; the original4MiB profile accepts53/133. Their ending rates therefore cannot measure sustained enabled-audio overhead. The original pool/graphics and the failed HWRAM-only link remain preserved.

## Native SH2 cost diagnosis — private probes, acceptance still failed

The diagnosis under `audio2-build/sh2-cost-profile/` retains the original TAG game and firmware10 as a stable performance fixture. It does not repair that firmware's known PCM or pitch failures. Only private audio observers and private module/compiler variants are used. No historical game source, frame pacing or emulator clock is changed.

### Clock and attribution

The native F5 state proves SMPC clock divisor65 and both FRT clock divisors128. With the exact Mednafen master clock1746818182, the SH2 clock is `1746818182/65` Hz and the timer rate is `1746818182/65/128 = 209954.108413` ticks/s. The calculations use that clock, not the SDK's nominal210 ticks/ms or the adapter's nominal60Hz clock.

The first original-TAG observer reaches3001 logical steps; the external polling endpoint is reported as observed. Over54.706122 seconds of active SCSP clock, `wf_music_seq_run_serviced` consumes48.558207 seconds, or88.7619%. Its4155 calls average11.6867ms with a maximum29.7160ms. **This inclusive call contains the original Z80 interpreter, YM/OKI callbacks, FM service and their memory/bus stalls.** It is not a measurement of Z80 arithmetic alone.

The master audio service consumes111.896ms across3001 calls, an average37.286µs and maximum123.837µs. The sampled four shared telemetry stores average5.061µs per outer loop; their estimated total is108.64ms, about0.2% of active time. Reducing those stores is not supported as a solution to the measured game-rate loss. The worker performs392.4 outer loops/s once music is active, rather than continuously polling at an assumed unmeasured rate. `profile-results.json` preserves every counter, timer floor, formula and capture identity.

A second original-TAG probe samples each callback category once in64 calls, excluding ROM boot. It reaches2001 logical steps, with35.883855 active seconds and30.961528 inclusive sequence seconds. All14 original producers seen at that endpoint are accepted; API/worker errors are zero. These are subsystem-observation facts, not PCM acceptance.

| Callback category | Calls | Samples | Mean sampled time | Estimated total | Fraction of inclusive sequence time |
|---|---:|---:|---:|---:|---:|
| YM event, including conversion/publication and nested flush |75478|1179|27.188µs|2.052094s|6.628%|
| IRQ event / FM flush |6598|103|16.925µs|0.111669s|0.361%|
| Cooperative FM service |16232|253|16.868µs|0.273801s|0.884%|
| OKI publication |859|13|28.944µs|0.024863s|0.080%|
| Bank event |0|0|unmeasured|unmeasured|unmeasured|

The largest sampled YM call is195.281µs. It is a sampled maximum, not an exhaustive bound. The observer adds per-call bookkeeping and changes instruction-cache placement, so the table is an attribution estimate rather than an exact subtractive decomposition. It does establish that measured YM conversion/flush accounts for a small minority of inclusive sequence cost. `callback-probe/profile-results.json` includes the native report and exact observer module hash.

### Matched native event checkpoint

Private variants run the same original TAG master, assets, input tape and firmware10. An independent untouched R01 plus original scalar Z80 reference supplies the first65536 musical/IRQ events, including each original64-bit cycle. Their digest is `4d3ef2a3`, ending at cycle108786762. The native checkpoint freezes after the complete sequencer call containing that event, so each run's actual virtual duration is recorded and the measured time is normalized by it.

| Private SH2 variant | Inclusive sequence seconds per virtual second | Difference from baseline | Native first65536 events/cycles |
|---|---:|---:|---|
| Original flags/source |0.851441065|baseline|exact|
| Explicit aligned SH2 `mov.w` ROM/RAM reads |0.851269239|−0.020%|exact|
| Only `z80-native.c` compiled `-Os` |0.875026344|+2.770%|exact|
| Only `wf_music_seq.c` compiled `-Os` |0.833957858|−2.053%|exact|
| Nine exact scalar admissions plus sequence `-Os` |0.805403475|−5.407%|exact|

All five checkpoint runs report API/worker error0. The word-access improvement is immaterial; the native-core `-Os` variant regresses and is rejected. Sequence `-O3` fails the unchanged18KiB link limit and is also rejected. Failed links, source snapshots and all native captures remain preserved. This table measures an instrumented audio subsystem, not final TAG cadence or continuity.

The nine scalar admissions are the original single-instruction helpers for opcodes `01,04,0E,38,56,82,90,BA,C6`. They retain the same cycle table, register/flag updates, memory operations and interrupt tail. The initial test fails because the baseline does not admit these opcodes; the candidate passes1245190 exact original-scalar cases, including physical boundaries, all16-bit load addresses, flags, RAM and I/O order. It also passes all18 tracks for600 seconds plus8 seconds of fade:10962 complete CPU/RAM/timer/event checkpoints,956109 calls and3151318 service callbacks, followed by the original live transition sequence. The native checkpoint above independently validates the target-compiled event path.

That candidate **still fails** the work128 firmware's nine-command transition fixture. The first observed FM deadline error moves from approximately9.260s to9.327s, with2 late events,1 discarded FM event/batch,0 PCM underruns and only4/9 commands accepted. It is a measured partial improvement, not a completed fix. The candidate was initially private; the subsequent selected-source installation below supersedes that installation status without changing the failed native result.

### Selected SH2 source installation

After review, only the nine scalar admissions and sequence `-Os` were installed. Static and dynamic builds keep `z80-native.c` at `-O2`; host sequence/reference verification compiles the selected sequence object at `-Os`. The hot reservation remains18KiB, the slave stack8KiB, the service bound512 instructions and the FM/producer capacities and4096-sample lead unchanged. No extra fade lift or24KiB memory profile was adopted.

The installed native source SHA256 is `c408e4a8ad5d7b54a55706f4c96fd9f4d39271dcd0fb5e2de14331af138f4fad`. A fresh production-source gate passed the intended RED assertion, all1245190 GREEN cases, all18×600s plus8s fades against unchanged R01, and the live command transition sequence. Input/source hashes remained stable. These full host state/event comparisons are distinct from the native count/digest/final-cycle checkpoint in the table: the native observer records a digest, not a literal exhaustive native event transcript.

The published source-only reproducer rebuilds both archived native observer modules byte-for-byte: baseline `2a2ae43dd9be511c746e9cb147e4eb7f464197b80916964d9d945da348e466b9` and selected `bc5a5e0314f4e8a3fd39238fad3a7288fb5393c2de327b79bb3ac16b5dce32b0`. The complete frozen 57-file observer/binding fixture is included in `tests/benchmarks/sh2_checkpoint/`. `tools/build_sh2_checkpoint_fixture.py` reconstructs it from the source package, supplied ROM ZIP, restored SDK and original WP1979 source/assets; no archived private build or object is required. The clean preflight also reproduced firmware10 and the original master flat binary exactly, kept all 345 game files unchanged, and then completed the two-module replay build through `--fixture`. No emulator was started and no new native timing is claimed. Complete commands, supplied-input requirements and acceptance limits are in `tests/sh2-scalar-admissions.md`; `evidence/sh2-checkpoint-source-preflight.json` records the new source reconstruction proof while `evidence/sh2-scalar9-selection.json` preserves the original measurements and earlier preflight.

### Why the `1F` fade stops idle skipping

The unchanged native transition replay is `01→06→12→1F→03→1F→02→01→1F`, at3-second intervals. Its input hash is `617d67b4f715cfeac79e503bbf9107e23ae5f63a900214be01a6baa8b305df1d`. A read-only rerun of the original failing ISO watches the actual module state, using a target-compiled `offsetof` helper rather than guessed structure offsets.

Generation remains4 after accepting the first `1F`; it increments to5 only when the fault handler runs. The sequence's skipped-instruction counter freezes at1079518, while executed instructions and cycles continue. The queued FM count remains between0 and22 in the observed interval, so queue capacity is not exhausted. The virtual lag rises from12.743ms at9.043s to69.559ms at9.110s,106.433ms at9.193s and139.329ms at9.227s. The first late event is observed at9.193s and worker error15 at9.260s. These are observation bounds from external polls, not sample-exact writes. There are no PCM underruns in this transition capture.

Two consecutive **unchanged original R01 instruction traces** after `1F` rule out treating this as a second empty loop:

| Original PC003A iteration | Z80 cycles | Executed instructions | YM writes | Timer IRQ events | TimerB period | Status at entry/exit |
|---|---:|---:|---:|---:|---:|---|
| First traced fade iteration |28080|3427|49|1|21504 cycles|02/02|
| Next fade iteration |27962|3414|49|1|21504 cycles|02/02|

The first iteration changes13 RAM bytes; the next changes10. Each performs real volume/channel work and outlasts a timer period. The existing120-cycle idle-loop guard correctly rejects these states. Enlarging that guard would skip original behavior. The complete CPU/RAM snapshots, every instruction boundary and every event are retained in `fade-r01-trace/{states.txt,instructions.tsv,events.tsv,trace.log}`; these host traces establish semantics, not Saturn performance.

The separate host work-count diagnostic observes227753 executed instructions and159214 skipped instructions in a representative second before fade, versus434928 executed and957 skipped in the first fade second. Common fade opcodes and the fixed four-byte `LDIR` still enter the cold fallback. This motivates private exact-block experiments whose full register, flags, stack, RAM, refresh-register, cycle, timer-boundary and512-instruction service contracts must all pass before any production adoption. Neither queue capacity nor the4096-sample output lead is increased.
