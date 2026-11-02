# Reference output identity and recovery

The reference tools now refuse to overwrite an existing output. A successful
audio comparison still requires checking the actual files against their
recorded SHA256 identities. A manifest records an earlier observation; it
does not make a later truncated file valid.

## Observed failures and preserved evidence

An identity audit of the 18 full-board music references checked the WAV,
control trace, PCM, capture report and command-delivery report for every
command. **88 of the 90 files matched. All 18 WAVs matched.** The trace and PCM
for command `0x05` had shorter contents than their recorded identities:

| Artifact | Observed bytes | Recorded complete SHA256 | Observed SHA256 |
| --- | ---: | --- | --- |
| `0x05` trace | 1,175,552 | `72d28af1b6321917b120dbf34280f2c937b3e390a9b0c1b44272288887472089` | `b7f757ec61333fb4a4c351e44cb44ab791a0bbfeb1d918d443e9ff4ce5fd2078` |
| `0x05` PCM16BE | 2,473,984 | `97228c81355b85a221883bde1c6bd271d0cc00b349cb904b7310fa0d923de32f` | `9e9e8a8060f8265137b45cb6b763ae921e90a4f1d3bdb18cf3345f9c629a4b49` |

Both lengths are multiples of 4096. Their filesystem modification times were
`2026-11-01T23:00:48.100433Z` and `.104433Z`, before the case report and
delivery report at `.664441Z`. Those observations do **not** establish who or
what truncated the files, or whether an interrupted producer was the cause.
No write to an old output path was identified in the inspected Python
wrappers or active-bank diagnostic. The cause remains unresolved.

The original files and the original manifest were preserved. A separate
directory reran only command `0x05` for 30 seconds with the exact original
executable, inputs and schedule, redirecting **all four** output paths. All
four regenerated outputs matched their original complete hashes. The
recovery manifest changes only the affected case's output paths and records
the relationship to the original manifest; no original WAV was replaced.

| Identity | SHA256 |
| --- | --- |
| Original full-board music manifest | `62aa5ad3ea9a39b201aa2dc36e0cf20dc3798ba67c4f7b78c14564df92f144a7` |
| Identity audit, before recovery | `527d9454f31f2844fd5f3918fb1114dbeadce3e876f5f1b6df4bfc7831ca748e` |
| Separate manifest using the recovered `0x05` case | `a4a9a9969aa26b9c9dc348c3ea0ac3bc3f861f329fd7dbf956b252eb18196412` |
| Original unchanged producer executable | `d96049ba8b3e3fd2784521bf97f76f8c8f796e1be549812edd6f881773202334` |

An earlier SFX export also rejected six truncated source cases (`0x23`,
`0x29`, `0x2C`, `0x34`, `0x43`, `0x6E`). Those originals were retained. The
six cases were reproduced in a separate frozen directory with the same
executable and matched their original output identities. The current
98-effect WAV manifest has SHA256
`7fa9289a91f9050d647a69cd7fe23d1e2357026c7b540ad7cdb966174a4fb797`.
Neither successful recovery establishes the cause of the earlier damage.

## Bounded producer fix

`prepare_and_build()` and `run_case()` already create their output
directories with `exist_ok=False`. Their regression tests confirm that an
existing directory is rejected before a child process starts and that its
sentinel contents remain unchanged.

The direct C command recorded in an old report could bypass that protection:
the old producer opened its four output arguments in `w`/`wb` mode. A causal
test reproduced this risk with private disposable sentinels: each of the
four output positions was overwritten in turn. This demonstrates an unsafe
replay path; it does not demonstrate that this path caused the observed
truncations.

`capture_board_replay.c` now checks all four output paths before opening any,
rejects duplicate paths, and uses exclusive `wx`/`wbx` opens. It also checks
all four close results. Its CPU, YM, OKI, clock and audio arithmetic are
unchanged. The four sentinel cases pass with the corrected producer, and a
fresh one-second original `0x05` run produces the same trace, PCM, report and
delivery hashes before and after this change. The causal evidence is in
[`reference-output-guard-validation.json`](../tools/reference/evidence/reference-output-guard-validation.json).

| Producer source | SHA256 |
| --- | --- |
| Before output-ownership fix | `693212ee41b643955e502f3f89d8e263399242c66ea219363a0d63d9465bf466` |
| After output-ownership fix | `b2b0bc868f8e846a2eb580e3f5d18034a52f6c83d46c170271e259952f0e9dc4` |

To run the direct producer tests, build a fresh reference directory first,
then provide that directory's executable and privately generated ROM inputs:

```sh
WF_REFERENCE_CAPTURE=/path/to/new-reference/capture_board_replay \
WF_REFERENCE_INPUTS=/path/to/new-reference/inputs \
python3 -m unittest discover -s tests -p test_reference_output_ownership.py -v
```

The historical executable and historical manifests are retained for
identity. Their recorded commands are provenance, not permission to reuse
their old output arguments. A recovery always uses new output paths and
retains the failed attempt.

## Current full-board register-use audit

The current audit uses the coupled full-board traces and verifies every
trace hash before and after reading it. Command identities come from the
manifest because the full-board trace's `ORIGIN` row does not encode the
music command. It rejects the truncated `0x05` trace; the completed audit
uses the separately recorded recovery manifest above.

Across 18 windows of 30 seconds, including each captured boot history,
there are 1,040,424 YM data writes. These windows contain no activation of
noise, CSM or nonzero test mode, and no modulating LFO on an ever-keyed
channel. Observed feedback values are `0`, `5`, `7`; observed algorithms
are `0` through `4`. These are bounded observations, not claims about every
future loop, transition or complete composition. The aggregate metadata is
[`full-board-ym-register-usage-summary.json`](../tools/reference/evidence/full-board-ym-register-usage-summary.json).

```sh
python3 tools/reference/audit_ym_usage.py \
  --full-board-report /path/to/verified/reference-report.json \
  --out /path/to/new-private-ym-register-usage.json
```

The audit's complete private report has SHA256
`6b192cf929943416173c28fd5b1c32dbab909fefc226f4764dac0e06b2a7b463`.
Its public summary contains counts and identities, not original sequence
events or sample payloads.
