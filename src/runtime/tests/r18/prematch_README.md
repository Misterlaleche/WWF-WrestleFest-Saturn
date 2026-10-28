# Prematch controller and actor programs

`wf_r18_prematch_tag_step_a68a` executes one original TAG dispatch from `$A68A`
to `$A6A2`, through all 24 entries of `$A7B4`. `wf_r18_prematch_actor_step_acb6`
executes the original actor visual program for an A0 selecting one of nine
actor records. Both operate on the caller's complete 16 KB big-endian arcade
RAM, live D0-D7 shadow and A0-A6. They supply no new time source or register
initialization. Terminal low state `$18` is accepted without dispatch.

The synchronous effect callback must implement `$2AEA` palette allocation,
`$9A42` FG clearing, `$2208` movement, and `$2052` audio with the same RAM and
register context. Missing effects return `WF_BLOCKED` with the dependency PC.
Failure preserves the executed prefix and requires stopping or restoring a
checkpoint before retry. No placeholder effect counts as successful support.

`wf_r18_prematch_scene_init/step` now supplies the complete `$A654` cursor:
TAG join, presenter and actor render traversal, frame handshake, four physical
input words masked by `$0030`, and common `$A778` teardown. It stops at the
original RTS `$A7B2`; caller `$C6C` and later gameplay continuation remain owned
by the surrounding composition. The retained Royal branch inside `$ACB6`
supplies no Royal anchor placement. No entry, Makefile, guard in another owner,
or platform video implementation was changed.

Scene status `MORE` continues execution; `WAIT` keeps PC `$A728` until the
supplied SYNC callback sets RAM `$1C006F.bit1`; `DONE` retains PC `$A7B2`.
SYNC does not increment any clock itself. READ16 callbacks return the original
active-low physical words at `$140020/22/24/26`; the cursor performs the exact
complement/mask operations. CALL dependencies also include `$98BA`, `$247C`,
`$27B8`, `$2836`, `$2B58`, `$7506`, `$2A06`, and `$8EB6`. They must retain their
original register and memory effects. Failed effects/IO are fatal prefixes.

The differential harness compares intact-ROM execution of each controller or
actor update, including 16 KB RAM, 16 KB FG, 36 KB palette aperture and padding,
D0-D7/A0-A6, seed and ordered dependencies. The native bridge delegates `$2AEA`,
`$9A42`, and `$2208` to intact Musashi code. Only device audio `$2052` is bypassed
on both sides. Thus the result verifies this module, not native implementations
of the delegated effects, a composed prematch scene, or physical Saturn timing.

The full matrix passed **30,907 comparisons**. Eleven separate controls verify
invalid contexts, unknown table windows, terminal state and missing dependency.
RED logs demonstrate the prior missing `$A7B4` and `$ACB6` guards. Evidence is in
`validation/local-20261024/prematch-{red.log,actor-red.log,green.json,green.log,
controls-green.json,controls-green.log}`.

The full scene differential additionally passed **334 complete scenarios** and
**14 lifecycle/error controls**. It checks RAM/FG/palettes/padding/board ports and
D0-D7/A0-A6 at every CALL, SYNC, and READ16 boundary, plus final complete regions,
WAIT count and terminal PC. This includes four input ports, each action mask,
non-action inputs, delayed IRQ ACKs, all 24 TAG states, nine active actors,
natural TAG completion and natural Royal completion with one/six active actors.
Its external CALL helpers run intact ROM, with audio `$2052` bypassed and actual
VBlank status supplied to delegated `$2A06` at its two polls. TAG fixtures require
at least one active participant: `$98BA` otherwise inherits A3 without a producer.
Royal zero-active traversal is included. Evidence is `prematch-scene-{red.log,
green.json,green.log,controls-green.json,controls-green.log}` in the same directory.
The earlier controller/actor source prefix remains byte-for-byte identical,
SHA256 `63531cc5790ac3b5a1793924f7fba009731d888392db920ee7bc5f99334f2c59`.

The three synthetic fresh-presenter routes give 989 fixed updates plus four
identity windows of 65/129/257. This requires zero fractional presenter Y.
`$A9E2` preserves fractional coordinates; nonzero initial fractions can reduce
the `$A8EC` walk from 94 to 93 updates. `$1C0168` also allows activation to skip
four subsequent states. Neither duration is asserted as universal.

ROM authority also resolves two summary ambiguities: `$A814` writes pose `$000C`
to the presenter A0, while enabling an actor's state byte through A2. Its sound
selection uses `$3032` for identity below 5 and `$3031` for identity 5 or above.
The twelve visual windows preserve the original aliases and lengths, including
identities 4/5 sharing `$ADA8` with a shorter length and 8/10 sharing `$ADFC`.

Run from runtime18 after `. ../toolchains/host-env.ps1`:

```powershell
& $taskPython src/runtime/tests/r18/prematch_verify_rom.py --report validation/local-20261024/prematch-green.json
& $taskPython src/runtime/tests/r18/prematch_verify_rom.py --controls-only --report validation/local-20261024/prematch-controls-green.json
& $taskPython src/runtime/tests/r18/prematch_scene_verify_rom.py --report validation/local-20261024/prematch-scene-green.json
& $taskPython src/runtime/tests/r18/prematch_scene_verify_rom.py --controls-only --report validation/local-20261024/prematch-scene-controls-green.json
# Reproduce missing-owner RED without changing production:
& $taskPython src/runtime/tests/r18/prematch_verify_rom.py --smoke --pending
& $taskPython src/runtime/tests/r18/prematch_verify_rom.py --smoke --actor-pending
```

No global suite or composed SH2 build was run by this isolated owner task;
those remain the integration owner's checks.
