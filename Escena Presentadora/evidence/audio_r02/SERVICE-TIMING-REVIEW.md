# Service-duration review and last-bucket containment

## Question

The adaptive service initially sampled SCSP CA once, then could decode and copy two blocks. The full-track host tests advance playback between service calls, so they did not establish safety if real playback crosses a half boundary during decoding.

## Reproduced edge

`test_bgm_service_duration.c` links the actual streamer and codec and models 4 ms of decode work immediately before each 2,048-byte SCSP copy. This is an explicit timing model, not a measured physical worst-case duration.

Monitoring is initialized, and service calls occur at 200 ms and 400 ms. A later call starts at 738 ms. Its 338 ms gap remains below the existing 371.519 ms late-service rejection. The first ring wrap is at 743.038549 ms.

| Version | First copy | Second copy | Writes into the active half |
| --- | ---: | ---: | ---: |
| R01 | 742 ms | none | 0 |
| Initial two-block candidate | 742 ms | 746 ms | 1 |
| Final candidate with last-bucket cap | 742 ms | none | 0 |

The initial candidate therefore added a measurable stale-CA window under this delayed-service scenario. Its failing zero-active-write regression is preserved in `last_bucket_red.txt`. The earlier `service_duration_results.json` is historical evidence from before containment; final results are in `candidate_test_results.json`.

## Minimal containment implemented

After extracting and validating CA, the final candidate applies:

```c
/* The last 4096-sample bucket has no positive time-to-boundary bound.
 * Keep R01's single-block policy there instead of adding a second copy. */
if ((ca & 3u) == 3u) budget = 1u;
```

This uses the existing monitor read and does not alter driver access or the failure API. CA 3/7 identifies the final 4,096-sample bucket of the current half. Its lowest guaranteed time remaining is zero. If a second block is allowed, the sampled position is in an earlier bucket, with at least 4,096 samples—92.879819 ms—until the next half boundary. The actual time remaining can be greater.

That lower bound is useful margin, but is not a universal physical execution-time proof. Full-cache emulator service measurements near 4.9 ms are observations, not physical SH-2 worst-case bounds under all interrupts and bus contention. A decode/copy stalled longer than that 92.88 ms margin could still cross the boundary.

## Existing one-block window remains explicit

The duration test also starts the delayed call at 740 ms. Both R01 and the final candidate perform their one block's copy at 744 ms, after the wrap, and report one active-half write. The runner expects this result deliberately: it documents the inherited one-block stale-CA window rather than asserting that the containment eliminates every timing race.

The correction specifically prevents adding a second block when the already-sampled bucket cannot provide a positive time-to-boundary margin.

## Why a blanket last-bucket failure was rejected

A proposed stronger rule—fail immediately whenever an unfinished refill reaches CA 3/7—would reject valid service schedules. Steady 45 Hz and 50 Hz calls are respectively 22.222 ms and 20 ms apart, both below the 23.220 ms threshold for two blocks. Filling sixteen blocks legitimately takes much of the 371.519 ms half and may use its last quarter. The original one-block policy can sustain these rates.

The selected containment preserves one-block work in the last bucket. The new full-track 45/50 Hz tests verify this tradeoff directly rather than treating a blanket refusal as harmless.

## Fresh verification

Run:

```sh
python3 -u wp02_r02/research_audio_r02/run_candidate_tests.py
```

Exit status is zero after:

- All five original audio/format/gain/codec/stream suites pass.
- All ten first-failure reasons and record immutability/reset pass.
- Every one of the 1,278,900 original PCM samples matches at 59.826 Hz, 29.913 Hz, the mixed missed-refresh schedule, 45 Hz, and 50 Hz.
- No simulated playback-time disc read or active-half write occurs in those five full-track cadence tests.
- 45/50 Hz and normal 59.826 Hz calls remain limited to one block.
- The 738 ms duration regression has zero active-half writes for R01 and the final candidate.
- The 740 ms duration demonstration records the inherited one active-half write for both versions.
- Original R01 retains its expected alternate-refresh failure.
- The final candidate compiles with the real SH-2 SDK.

The 10 Hz diagnostic test now records seven refills and 14,336 unfinished fill bytes, because the final pre-wrap service is limited to one block. Its source sample position is 39,936; the first failure record remains correct.

No assertion of physical hardware success follows from these models. Root owns the integrated capture and the user’s next console test.
