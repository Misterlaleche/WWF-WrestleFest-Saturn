# F1 continuation: original ROM evidence and bounded correction

The original full Z80 execution exposed **17 extra PCM starts** in the preserved
A15 physical playback model and the previous native SFX engine. The original
program skips these starts. Matching A15's logical sequence state did not detect
this difference, so that old gate cannot establish audible SFX equivalence.

The affected hexadecimal commands are `38,41,43,46,49,4C,51,5C,61,69,6A,6B,6E,
6F,70,75,7B`. Their first sample starts match. After the sample ends naturally,
the old model restarts a later F1 continuation. The original sequence retains
its suppression latch and stays silent instead of restarting.

## Original instruction path

This observation is pinned to sound ROM SHA-256
`f89e7782e7535576b2ebf8c5daea61c5fa2ecefff0b8b3280dea09efa57c25f7`.
The original SFX routine at `0479` loads the suppression-latch base `C049`,
adds the current logical voice, reads the byte and tests it. At `047F`,
`JP NZ,0523` skips the entire physical stop/start path. **The branch does not
read OKI busy status.** F1 sets this sequence latch; F2 clears it and performs
the corresponding stop. Natural PCM completion is not a write to this latch.

An observation-only hook on that actual branch records command `38` taking it
at relative Z80 cycle4,137,125 (1.155768401 seconds), with latch1 and the same
sample ID retained in D. The preceding PCM has already completed. The observed
run's PCM is byte-identical to the unobserved run. This is a direct branch
observation, not an inference from the sound's apparent duration.

## Why the packed flags are sufficient

The existing ROM resource generator retains two bits for these events. Across
all98 original programs and all165 parsed sample/delay events, the following
predicate was checked independently against the literal F1/F2 stream state:

| Packed flags | Original action |
|---|---|
|0 | Emit a new physical note |
|1 | Advance the sequence delay and pointer without emitting a physical note |
|3 | Emit a new physical note followed by F1 suppression |

All65 suppressed continuations match flags1, with zero discrepancies. This
predicate is specific to the verified original programs; it is not a new
general-purpose interpretation of arbitrary data.

## Minimal native change

Only `fire()` in `src/wf_scsp_sfx.c` changes. A flags1 event updates its delay
without invoking the physical start routine. For flags0/3, it clears the old
physical suppression bit and calls the existing start path. This lets an
explicit new note proceed even if a previous PCM voice has left that bit set.
The sequence delay, priority logic, command tables, buffers and structures are
unchanged. The general music/OKI start helper retains its existing behavior.

The original-start fixture is generated from full original-Z80 execution with
coupled pinned Nuked TimerA+B/busy/IRQ and a ROM-addressed OKI model. The native
host test failed exactly17 commands before the change and passed all98 after
it. The oracle observes100 physical starts across the98 isolated commands;
the legacy A15 model produces117. Source, executable, fixture and result hashes
are in `tools/reference/evidence/f1-native-red-green.json`.

The prior114 complete PCM/alias checks, prepared-buffer checks, logical command
and priority checks, and refill checks also pass after the change. Their scope
is preserved: the logical A15 comparison remains useful, but it does not test
this physical-start policy. The old A15 source and its rendered files remain
unaltered and explicitly retain the17 known extra starts.

## Reproduce from original inputs

```sh
python3 tools/reference/verify_full_board_sfx.py \
  --rom-zip /private-input/wwfwfest.zip \
  --out /private-output/full-board-sfx --jobs 2
```

The tool downloads the pinned host-only Nuked sources, or accepts `--opm-dir`,
recreates and hash-checks the114 complete PCM payloads, executes all98 original
commands, audits the165 stream predicates, generates the private C fixture and
compiles `tests/test_sfx_original_starts.c` against the current native engine.
The fixture, traces and WAVs remain outside the source package. The repository
contains only generator/test code and statistical/hash metadata.

This gate establishes the isolated commands' physical start counts and sample,
bank and volume identities. Mixed-command priority, active global bank changes,
admission latency, timing and physical native audio remain distinct gates.
The corrected firmware must still be built and captured before assigning a
native-runtime PASS to it.
