# Combat integration gate — native TAG / Royal

**Checkpoint, 2026-11-01: integration reproduced; audio acceptance blocked.** A complete genuine TAG input replay has run through the new audio binding on standalone Mednafen 1.32.1. Gameplay is byte-identical to the matched control at the original frame-7372 endpoint. The frozen firmware10 candidate nevertheless records 19 actual PCM underruns, 22 late transport events and a 4.6344% increase in combat time. This candidate also predates the subsequently identified ROM-authoritative pitch correction. Neither fidelity nor hardware readiness is claimed.

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

## Exact matched experiment

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

## Audio failure exposed by combat

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

## Evidence checkpoint and hashes

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
