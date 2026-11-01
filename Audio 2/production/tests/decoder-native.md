# Exact native68000 decoder optimization

The production `src/wf_sfx_decode.S` now decodes the two nibbles of an encoded byte together. It removes a repeated byte read, nibble-phase toggle, remaining-count check and part of the loop control from the common path. The existing 49-step MSM6295 table, recurrence, saturation, signed PCM16 output and decoder ABI are unchanged. No audio buffer, refill threshold, scheduler, YM code or gameplay was changed by this optimization.

**This is a verified decoder improvement, not complete audio acceptance.** The combined scheduler/decoder must still pass the real64-command playback and native TAG comparison. All measurements below run actual baseline68000 instructions in unchanged standalone Mednafen1.32.1, with no expansion RAM. No host decoder result is represented as a native execution.

## Preservation and implementation contract

The original assembly remains privately at `audio2-build/decoder-optimization/decoder_original.S`. Its SHA256 is `5e97ad36a59fd374886bf3aad571e231a302a2b5ba413a53cc1d72d7cb04bf48`. The untouched historical file `Audio 2/base/src/audio/native/wf_sfx_decode.S` is byte-identical and can supply the baseline for reproduction. The baseline copy is never overwritten.

The installed candidate SHA256 is `64c8e4137aee642546dcd6c1f5bca1f0103b6b905faa683b3b7aaa093bd705ed`. Its text grows from176 to394 bytes, **+218 bytes**; the3136-byte lookup table is unchanged. It preserves the same callee-saved registers and does not allocate memory or increase stack depth.

Valid decoder state has index0..48, signal−2048..2047 and low-nibble phase0/1. An initial pending low nibble is handled separately. Complete bytes then use an unrolled high/low path; a final high nibble keeps the encoded offset on that byte. The valid input count is bounded once, and any requested tail beyond remaining input is filled with zero without input reads. Zero count returns immediately. The historical nonzero-count DBRA low16-bit semantics are retained; real engine callers request at most256 samples. State is compared on return; the decoder does not promise an atomic multi-field snapshot during its call.

## Native RED and GREEN

The negative fixture `decoder_stub.S` deliberately returns without decoding. It is test-only and is never installed in production. Native RED capture `red02-capture/decoder-results.json` rejects it on the first case: expected PCM word `0x8020`, observed untouched sentinel `0xdead`.

The actual candidate in `paired01-capture/decoder-results.json` passes:

- **1,034,226 samples from all114 original ROM entries**, comparing every PCM word directly with the preserved assembly.
- **114 independent expected CRC32 values**, generated from the complete PCM16BE oracle whose SHA256 and each sample SHA256 are checked against the pinned independent audit before generating the native fixture. This validates the baseline as well as the candidate.
- **24,942 decoder-state comparisons** after block calls, including offset, remaining count, signal, step index and nibble phase.
- **11,314 explicit edge cases**: all49 indices ×16 nibbles ×7 boundary/interior signals ×both phases, plus a matrix of zero/odd/even block counts and remaining lengths around boundaries. These cases additionally use the scalar recurrence compiled for the68000.
- Output guards before/after each block, exhausted-input zero fill and uneven continuation chunks of1,17,63,2,31,129,128 and255 samples.

Both assembly routines are linked under different names in one firmware and share the same lookup table. Native PCM/state comparisons and golden checks happen on the68000. The SH2 uploads the existing pack once, starts the test and reads diagnostics; it does not synthesize expected PCM or execute the decoder on behalf of the68000.

## Measured cost

Timing uses the existing SCSP DSP sample counter around each decoder call. Both routines receive the same initial state and encoded input and write the **same physical Sound RAM buffer**. Baseline/candidate order alternates. Input/state setup, output copying/comparison and result reporting are outside each timed interval. These are decoder-only timings, not the full refill scheduler cost or a gameplay frame-rate estimate.

The full GREEN run measures1024 blocks of128 samples: baseline136.725586 DSP samples per block, candidate93.094727, a **31.9113% reduction**. Its baseline range is136–137 and candidate range93–94.

A separate timing/PCM comparison covers the actual refill/preparation block sizes:

| Samples per block | Blocks | Baseline mean, DSP samples | Candidate mean | Time reduction |
|---:|---:|---:|---:|---:|
|32|1024|36.655273|26.374023|28.0485%|
|64|1024|70.032227|48.625000|30.5677%|
|128|1024|136.773438|93.125977|31.9122%|
|256|1024|270.250977|182.125000|32.6089%|

All4096 additional blocks match PCM and final state. `blocks02-capture/decoder-results.json` explicitly declares `benchmark_only=true`; its `correctness_pass=false` means it did not repeat the complete114-entry suite. The complete proof is the separate GREEN capture above. The performance threshold of5% in the probe only exceeds timer quantization; it is not an audio-acceptance threshold.

## Reproduce privately

Source the restored tool environment. Inputs remain local: full `SFX.DAT` and `PCM16BE.PAK`, the provided BIOS, the preserved original assembly, the pinned m68000 GCC runtime source, and a matching verified firmware build directory supplying `oki-lut.inc`, `wf_dsp_program.h` and `sine.pcm`. No generated ROM data or binary belongs in this source directory.

```bash
source /absolute/path/to/tools-env/env.sh
python3 production/tools/decoder_build_native.py \
  --candidate production/src/wf_sfx_decode.S \
  --baseline /absolute/path/to/preserved/wf_sfx_decode.S \
  --production /absolute/path/to/production \
  --resources /absolute/path/to/private-resources \
  --sdk /absolute/path/to/tools-env/sh2eb-elf \
  --template-firmware /absolute/path/to/firmware10 \
  --runtime-source /absolute/path/to/tools-env/src/gcc-lb1sf68.S \
  --out /absolute/path/to/new-decoder-build
python3 production/tools/decoder_capture.py \
  --build /absolute/path/to/new-decoder-build \
  --capture-tool /absolute/path/to/production/tools/capture_mednafen.py \
  --environment-root /absolute/path/to/tools-env \
  --bios /absolute/path/to/provided/mpr-17933.bin \
  --out /absolute/path/to/new-decoder-capture \
  --display 96 --seconds 300
```

Use an unused display and fresh output directories. A full run takes several minutes at normal presentation speed because comparison and independent checksum work also execute on the68000. The wall timeout includes boot; it is not the timing source. The build refuses the wrong baseline hash, mismatched independent oracle data, changed candidate sources or native ELF segments outside the existing loader contract.

To establish RED, use `production/tests/decoder_stub.S` as `--candidate` in a separate build. To repeat the additional timing-only experiment, add `--bench-only` when building and keep its report separate from the full correctness capture.

## Evidence identities

Private root: `audio2-build/decoder-optimization/`. It retains original and candidate source, all failed/intermediate builds, compiler commands, source manifests, symbol tables, disassembly, ISO, native F5 states and raw captures.

| Evidence | SHA256 |
|---|---|
|Unchanged Mednafen1.32.1|`a12628d798edf9ad1ec7497cec666ee64b583a09fc48a0c650702c8aeb3a7689`|
|Independent complete golden PCM pack|`45a22577841fc0ca9bdf84f6ec8acf72f2c87602c9a50eb42fce395a57131bee`|
|SFX pack supplied to native test|`dbe749cf4ce13f333ff7318a74a64e30fa2b9af05d953688a0813311cbabd216`|
|Full GREEN firmware|`b268ac126b45bbf774fae4891933f897f33c687db49a71225fed32e7dccd1266`|
|Full GREEN ISO|`46fdad99f9d765a575634ccaaaa9fd2bd32b34dce70e0e60066aebd3c5e77158`|
|Additional block-size firmware|`1b8d665324827184cdb8d6d84f25f5abb18846dbe7b1bd2cf2fa73476e6e4786`|

The focused change owns only `src/wf_sfx_decode.S` and new files whose names begin `decoder`. The production firmware, scheduler and YM sources belong to their respective owners and were not edited for this decoder improvement. Real Saturn hardware remains unmeasured.
