# Original-address / checkpoint host experiment

This directory is an isolated prototype. No file here is linked into native
firmware. The existing `SFX.DAT`, loader, public API and native source stay
unchanged. The `WFBA` tail is the private construction made by
`tools/reference/audit_oki_storage.py`, not an installed resource ABI.

Run from the production package, using a fresh private directory:

```sh
python3 -B tools/reference/probe_oki_address_replay.py \
  --rom-zip /private-input/wwfwfest.zip \
  --pack /private-output/resources/SFX.DAT \
  --model-run /private-output/full-board-mix \
  --out /private-output/oki-address-replay
```

Add `--sanitize` for AddressSanitizer and UndefinedBehaviorSanitizer. The runner
verifies the ROM and trace identities, regenerates private fixtures, compiles
the C test, records every command/result/hash, and requires three private
mutations to fail. It refuses existing destinations and repository output.
If LeakSanitizer cannot inspect `/proc` in a tracing sandbox, add
`--no-leak-check`; address and undefined-behavior checking remain enabled.

## What is exercised

The resolver scans at most 73 existing sample descriptors plus one extra range
on a cache miss, then caches only a span and its bank. It masks relative
addresses to 18 bits and reads every required address through either bank.
It does not allocate a full-ROM shadow or copy the start table into RAM.
Only the audited valid sample ranges and IDs are supported. Reads in the
remaining original address space are rejected.

The decoder is compared with the independent complete-ROM host model over all
114 valid samples, both nibble parities, repeated changes, saturation and the
inclusive last sample. The test then prepares a circular 4,096-sample buffer
and compares both consumed and repaired future samples at every sample end,
every ring wrap, sparse-checkpoint edges and repeated writes at the same cursor.
The interpolation guard always mirrors ring slot zero.

The 64-command mixed trace is replayed at its original 3,579,545 Hz bus-clock
boundaries and 8 kHz OKI ticks. All four per-voice outputs and their OKI mix are
compared with the original-ROM model. The existing start-latched
counterfactual must still reproduce the established byte/nibble/decoded/frame
differences. This does not synthesize FM again or produce a new mixed WAV.

## State and bounds

Each periodic checkpoint stores signed signal, step index and a tag in four
bytes. There are 33 slots per voice for the ring plus its preceding boundary:
528 bytes for four voices. One extra exact state and its absolute position
are retained at the latest committed bank write. A restore chooses the newer
of that anchor and the preceding periodic checkpoint. Therefore it never
replays across an earlier bank write with the wrong bank, and no unbounded
bank-event history is needed.

At a bank write the caller must broadcast to every active voice. Each voice
reconstructs the state at its committed cursor with at most 127 old-bank
samples, then regenerates the unconsumed prepared interval with the new bank.
The latter can still be 4,096 samples. Four active voices can therefore need
up to 16,892 restore/repair sample operations at one event. These counts
include zero padding after sample end; they are not measured CPU cycles.

## Deliberate limits

Positions are monotonic sample counts from voice start. A bank write applies
before the next nibble at `played`; the caller supplies that exact committed
boundary and serializes all operations. Native playback-cursor uncertainty,
concurrent SCSP reads, late events, prepared launch caches, both firmware bank
routes, event scheduling and atomic repair are not implemented. The prototype
has no claim of fitting the native linker margins or meeting a playback
deadline. Even after host equality passes, those remain separate work.

The read-only [2048-ring assessment](NATIVE-2048-READ-ONLY.md) documents the
recoverable memory, required DSP/loader changes, immediate-bank deadline and
unproved native cursor boundary. It does not authorize or install that profile.
