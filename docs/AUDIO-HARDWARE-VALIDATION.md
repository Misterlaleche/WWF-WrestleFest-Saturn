# AUDIO HARDWARE VALIDATION — 2026-09-11

## R04 superseding software-closure update — 2026-09-12

The historical test chronology below remains valid, but R04 supersedes the earlier software-open items for music source generation and command `0x1F`.

R04 adds:
- live Z80 + pinned Nuked-OPM rendering for music commands `0x01..0x12`;
- 18/18 first-12-second byte-exact matches against canonical A53;
- an optional live renderer input to the already hardware-proven CA-driven WP24 transport;
- original-driver handling for `0x1F`, with no PCM-domain linear fade.

Software status is now **AUDIO ROM-EXACT SOFTWARE CLOSED**. The remaining mandatory gate is real-Saturn validation of live-VM CPU budget and full-load concurrency. Runtime insertion of proven producers remains owned by the canonical gameplay integration work.

See `docs/AUDIO-ROM-EXACT-R04-CLOSURE.md`.

## Scope

This document records what has actually been proven for WWF WrestleFest audio on a real NTSC-U Sega Saturn with Terraonion MODE.

Authority order remains:

`wwfwfest.zip` ROM > audited FINAL ROM-EXACT audio packages > Saturn/Yaul technical authority > Saturn test code.

Nothing here authorizes guessed music loops, guessed fade timing, synthetic source material or replacement CDDA.

## Canonical arcade audio model

The audited FINAL audio research establishes:

- combined YM2151 + OKIM6295 production audio;
- music commands `0x01..0x12`;
- registered `0x13..0x1E` commands without demonstrated main-CPU producer;
- `0x1F` as a special fade lifecycle;
- SFX / voice commands `0x20..0x81`;
- bank 0 contains 73 decoded samples;
- bank 1 contains 41 logical samples, with IDs 20..41 being bank-1-exclusive material;
- Saturn production transport format: 44.1 kHz, mono, signed PCM16, big-endian.

## A53 validation pack — important correction

The canonical A53 `MUSIC.PAK` is an **exact renderer validation pack**, not a final production music pack.

It contains:

- 18 commands;
- 12 seconds per command;
- 529,200 samples per command;
- 1,058,400 bytes per command;
- total size 19,051,200 bytes;
- SHA256:
  `8e1c038b00f9800753d3d21eca47f2cace9a04d04c07280d7d4b9110b0fd27bf`.

Earlier diagnostic tests incorrectly treated those finite 12-second captures as complete songs. The resulting early stop was source exhaustion, not proof of a Saturn streaming limit.

Never relabel this pack as final production music.

## Hardware test chronology

### SCSP SMOKE-02
Result: PASS.

Proved:
- poneSound / SCSP path initializes on hardware;
- authentic recovered PCM is audible.

### SCSP SMOKE-03
Result: PASS.

Proved:
- master volume 0..15 works on real hardware.

### SFX browser / complete audio command test
Result: PASS.

Proved:
- SFX command playback is audible;
- bank-1 material is audible;
- presenter / ring-announcer voices are present and audible.

### AUDIO COMPLETE TEST 02
Result: PARTIAL.

Music became audible, but some commands stopped early.

The later audit established that the test was serving finite A53 12-second validation captures as if they were complete music sources.

### AUDIO COMPLETE TEST 03
Result: REJECTED architecture.

A fixed-time refill scheduler removed some stop behavior but introduced audible skipping/jitter.

Do not reuse the fixed-time refill architecture.

### AUDIO COMPLETE TEST 04
Result: transport behavior good, finite-source assumption still wrong.

The SCSP monitor was handled using CA-driven half selection. Music sounded clean again, but still ended because the diagnostic source was finite.

The key monitor rule is:
- slot 31 is selected for the SCSP monitor;
- do not rewrite monitor selection immediately before every CA read;
- CA values 0..3 indicate ring half 0;
- CA values 4..7 indicate ring half 1 for the 32768-sample slot-31 ring.

### Continuous checkpoint renderer

A checkpoint-capable renderer was built to preserve YM2151 + OKI state across render segments.

Validation:
- the first canonical 12 seconds remain byte-exact against A53;
- no synthetic loop is introduced;
- renderer segment boundaries do not reset synthesis state.

### AUDIO COMPLETE TEST 05
Result: **HARDWARE-PASS for long continuous transport**.

Problem commands retested:
`01,02,04,06,07,0A,0B,0E`.

Diagnostic continuous lengths:

| Command | Samples | Seconds |
|---|---:|---:|
| 0x01 | 2,646,000 | 60.000000 |
| 0x02 | 2,646,000 | 60.000000 |
| 0x04 | 2,646,000 | 60.000000 |
| 0x06 | 2,300,447 | 52.164331 |
| 0x07 | 2,284,063 | 51.792812 |
| 0x0A | 2,320,927 | 52.628730 |
| 0x0B | 2,304,543 | 52.257211 |
| 0x0E | 2,590,579 | 58.743288 |

All eight revalidated their first 12 seconds against canonical A53: PASS.

Diagnostic `LONGMUS.PAK`:
- bytes: 39,477,118
- SHA256:
  `b04634e61661797dc4a009e004d01adb546beedc04bc2d0d162713636956974f`

TEST-05 BIN SHA256:
`a7b8e4315e5f33abf54a97550fbeebfdf99c15277db6e4767f9dba39edb83951`

Real Saturn observation from the user:
- music plays correctly;
- no audible skipping;
- no premature cuts during the long diagnostic captures.

Therefore the Saturn PCM transport itself is no longer the identified blocker.

## Hardware-proven transport contract

The reusable WP24 contract is:

- physical SCSP slot: 31;
- sound RAM ring offset: `0x60000`;
- ring size: 64 KiB;
- two 32 KiB halves;
- 32,768 PCM16 samples total;
- LSA = 0;
- LEA = 32767;
- rate = 44,100 Hz;
- mono;
- signed PCM16;
- big-endian;
- half duration = 16,384 / 44,100 s = approximately 371,519 us;
- refill is driven by the actual SCSP current-address monitor, not a guessed wall-clock scheduler.

The stream core remains fail-closed:
- invalid CA -> error/stop;
- service gap >= one half duration -> error/stop;
- exhausted source marked CONTINUOUS -> error/stop;
- no invented short PCM loop metadata.

## What is CLOSED

- arcade audio reverse-engineering model: CLOSED by FINAL research;
- exact A53 12-second validation rendering: PASS;
- checkpoint continuation preserving synthesis state: PASS for canonical prefix validation;
- authentic PCM audible on Saturn: HARDWARE-PASS;
- master volume: HARDWARE-PASS;
- SFX path: HARDWARE-PASS;
- bank-1 / presenter voices: HARDWARE-PASS;
- slot-31 PCM16BE streaming transport: HARDWARE-PASS;
- long continuous diagnostic playback: HARDWARE-PASS.

## What remains OPEN

Do not mark the entire audio subsystem FINAL yet.

Still required:

1. prove or encode the production lifetime / stop condition of every music cue;
2. integrate exact arcade command dispatch with frontend and gameplay state;
3. implement and validate exact `0x1F` fade lifecycle;
4. choose the production continuous-source generation/packaging strategy without inventing loops;
5. validate simultaneous music + SFX + voice behavior under real game load;
6. perform final real-Saturn integration validation.

## Repository policy

This repository stores:
- transport source;
- contracts;
- manifests;
- hashes;
- provenance and hardware observations.

It does **not** store:
- original ROMs;
- ROM-derived protected audio packs;
- `AUDPCM.PAK`;
- `LONGMUS.PAK`;
- generated long PCM captures.

Those must be generated locally from the user's legally supplied source material and validated against the recorded hashes where applicable.
