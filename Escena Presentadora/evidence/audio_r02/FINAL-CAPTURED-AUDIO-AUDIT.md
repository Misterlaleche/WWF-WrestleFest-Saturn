# R02 final reference captured-audio audit

**PASS in the final guarded Mednafen capture. Physical Saturn playback remains unverified for R02.**

The audited capture is `wp02_r02/verification/reference-r02-final-full.mov`. Its 44,100 Hz, stereo, signed PCM16 audio was extracted without filtering, gain adjustment, or sample-rate conversion; only byte order changed from MOV PCM16BE to WAV PCM16LE. All source templates come from the immutable R01 package.

## Music continuity

| Measurement | Result |
| --- | ---: |
| Music onset | 32.210294785 s |
| Silence begins | 59.578117914 s |
| Played source duration | 27.367823129 s |
| Completed 32,768-sample ring cycles | 36 |
| Half-second comparison windows | 54 |
| Maximum absolute window lag | 0 samples |
| Cue-free music correlation, source 26.2–27.0 s | 0.999999992634 |
| Native/source gain | 0.837180204 (-1.543621 dB) |
| Residual RMS after accounting for that gain | 0.000016442 |
| Longest internal exact-zero run | 1 sample |
| Low-energy 10 ms dropout windows | 0 |
| Clipped scene PCM values | 0 |
| Left/right difference RMS | 0.0 |

The calibrated R01 recording has exactly the same measured music gain, cue-free correlation, residual RMS, and 27.367823129-second played-source duration. All 54 half-second comparisons retain zero sample lag. The final music stop is 5.533 ms after the visible terminal frame at 59.572585 s. These measurements show continuous source progression through the transport wraps at the stated analysis resolutions.

## Original cue delivery

All thirteen expected original cues are detected near their recovered scene deadlines. The timeline uses the independently verified first visible scene frame at 32.243379 s and the original 448 × 272 / 7,000,000-second update period. The first cue intentionally starts before the first visible frame. All later cue offsets are between −11.622 ms and +15.871 ms.

| Original scene frame | Command | Source bank/sample | Captured onset (s) | Correlation | Offset from scene deadline (ms) |
| ---: | --- | --- | ---: | ---: | ---: |
| 0 | 0x32 | 0/18 | 32.210125000 | 0.983131 | -33.254 |
| 179 | 0x7B | 1/37 | 35.369437500 | 0.922741 | +10.027 |
| 373 | 0x6D | 1/23 | 38.745750000 | 0.935372 | +9.187 |
| 438 | 0x32 | 0/18 | 39.882468750 | 0.980010 | +14.386 |
| 502 | 0x7C | 1/38 | 40.985531250 | 0.892648 | +3.336 |
| 524 | 0x71 | 1/27 | 41.353640625 | 0.903102 | -11.530 |
| 653 | 0x31 | 0/17 | 43.626562500 | 0.832179 | +15.760 |
| 718 | 0x7D | 1/39 | 44.746593750 | 0.892134 | +4.271 |
| 760 | 0x75 | 1/31 | 45.482203125 | 0.886930 | +8.744 |
| 1017 | 0x31 | 0/17 | 49.961578125 | 0.760639 | +14.263 |
| 1081 | 0x7C | 1/38 | 51.064906250 | 0.829130 | +3.479 |
| 1103 | 0x6E | 1/24 | 51.432781250 | 0.860087 | -11.622 |
| 1232 | 0x32 | 0/18 | 53.705906250 | 0.924382 | +15.871 |

Cue correlations range from 0.760639 to 0.983131. Compared with the preserved R01 lossless recording using the same analysis, matched cue gains differ by −0.0233 to +0.1190 dB. Source-bank, sample identity, and source-PCM hashes are retained for every cue in the JSON report.

## Method and calibration

The R01 package includes its audit results and documented method, but not its final cue-audit generator. This audit reconstructs that method and first validates it against the preserved R01 `reference-scene-lossless.mov`. BGM uses raw 44.1 kHz normalized waveform correlation, a cue-free interval for gain measurement, and 54 half-second windows with a ±2,048-sample lag search. The search excludes silent windows to prevent FFT roundoff from creating invalid normalized scores.

For cue analysis only, the aligned original BGM is subtracted, the residual is resampled to 8 kHz and filtered with a third-order zero-phase 100–3,400 Hz bandpass. Templates use the first second of each original cue, or the whole cue when shorter. This includes enough energy from the quiet beginning of sample 17 while avoiding assumptions that the original sequencer always plays an entire four-second common cue before replacing it.

The coarse 8 kHz cue match is retained in the report. The same filtered signals are interpolated locally by eight to refine the nearest-sample phase to 64 kHz timing resolution. This addresses the possible 62.5 µs error of a coarse 8 kHz match. The final coarse correlations are 0.731873–0.983113; the refined correlations are 0.760639–0.983131. The same refinement is applied to R01 calibration. None of this filtering, subtraction, or resampling changes runtime audio assets.

No required cue is missing or replaced in this recorded reference sequence. This is a waveform/onset audit; it does not assert that all fourteen asset files play to completion in this one reference roster. The separate host suites verify routing, original volume mappings, natural tails, and F1 suppression.

## Runtime evidence

The final capture reports 1569 scene ticks, 1165 refill blocks, 0 stream failures, 0 missed refreshes, and a maximum service call of 4862 µs. The final visual audit independently verifies all 169 original visual runs. The audio and video results are emulator evidence; the user's next Saturn test remains the physical confirmation.

## Reproduction and provenance

```sh
python3 wp02_r02/research_audio_r02/audit_reference_audio.py \
  wp02_r02/research_audio_r02/r02_final_reference.wav \
  --label reference_r02_final_audio_audit \
  --scene-origin 32.243379 --scene-end 59.572585 \
  --calibration wp02_r02/research_audio_r02/r01_same_method_calibration.json \
  --source-media wp02_r02/verification/reference-r02-final-full.mov \
  --runtime-metrics wp02_r02/verification/reference-r02-final-full.json
```

The command exits zero only when the music, continuity, cue, clipping, channel, and timing checks pass. `reference_r02_final_audio_audit.json` contains the original asset hashes, extracted-WAV hash, capture hash, extraction command, every comparison window, every cue measurement, calibration differences, and runtime metrics.

Capture SHA-256:

```text
475f6f39640a6f1a8ccaf77a124de08e3b87278cd588c7384ac325fdf5f1f2fe
```

## Portable host regressions

`portable/run_r02_tests.py` is ready to package as `source/verification/r02_audio/run_tests.py`, with the three sibling C tests and `fixtures/r01/wp02_bgm.c` plus `.h`. It reads the delivered `source/research_audio/integration` tree. A host C compiler is required; SH-2 compilation is optional and enabled only when `YAUL_INSTALL_ROOT` is supplied. All sixteen host cases pass, including the expected R01 failure and explicit inherited one-block timing limitation. Root also ran it successfully from the staged standalone delivery path.
