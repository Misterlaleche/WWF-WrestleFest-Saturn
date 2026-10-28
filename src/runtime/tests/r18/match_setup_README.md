# Raw match setup and paid controller

`wf_r18_match_setup_1086c` executes the original TAG branch through `$106DE`
and delegates Royal to the independent raw Royal module. Both accept the same
16 KiB big-endian RAM, live D0-D7/A0-A6 and original RNG/frame counter. TAG
preserves roster/history/replay semantics, the four weighted progress windows,
health rules, XY anchors and original MOVEM boundary. Palette `$2AEA` is the
only TAG setup effect. No `direct_boot`, identity replacement or reset occurs.

The apparent scratch alias at `$1C0598+$56/$57` is intentional ROM behavior.
The terminal history sentinel also clears the following word outside the ten
history entries. Replay paths retain incoming live register identities exactly.
Unknown indices and unsupported topologies remain guards; failing effects keep
their executed prefix and require a stop or checkpoint restore before retry.

The same file exposes pure `$D1C`, `$FFD2`, `$1004A` and a synchronous effect
cursor for `$C98..$D10`. `$D1C` uses the original scratch window and clears the
exact `$1C05AC..$1C1CD3` range before restoring the retained actor fields.
`$C98` delegates actual `$2988/$1FC0/$1F6C/$26E66/$2A06/$1F9E` owners. Any
VBlank polling belongs to those owners, without invented delays or inputs.

`wf_r18_match_flow` composes paid `$AC0/$BD2`, `$C6C` and `$8AE6` through the
original JMP `$F8C`. It exposes each CALL with its real call and return PCs.
Complete only after that owner finishes; pending requests are inert and failed
dependencies are fatal. `$6FC/$57E0` ENTRY requests require the actual reported
continuation PC. The outer driver owns dispatch and the RT import at `$F8C`.
Mandatory first-match `$7B70` is a separate presentation owner. It is called
even when replay makes its own body return early.

The intact-ROM differentials passed 2,181 TAG/dispatch fixtures, 473 common/join
fixtures and 156 paid/finalize fixtures plus 18 protocol controls. They compare
full RAM, live registers/RNG and each external call context/order/region digest.
Join/flow also compare BG64 KiB, FG16 KiB, palette36 KiB and board regions,
including padding. All delegated owners execute their unchanged ROM bodies;
flow suppresses only `$2052` sound output. Paid flow fixtures use replay for
the `$7B70` fast return; full card presentations have a separate oracle.

The independent input-owner review reran TAG/join and added 18 cases each,
covering solo P2/P4, paired participants, replay, progress4/9, mode02/FE and
credit WORD8000. Reports are `match_setup_verify_rom-independent-extra.json`
and `match_join_verify_rom-independent-extra.json`. Evidence and source hashes
are recorded in `match_setup_manifest.json` and `match_flow_manifest.json`.
The public referee wrapper appended to Royal reuses the already tested private
owner; `royal-setup-public-wrapper.json` verifies its prior source body hash.

These tests prove the stated raw owners and composition protocol, including
explicit external seams. They do not prove native backend composition, CCR/A7
temporary stack, the complete game, SH2/hardware timing or global ROM equality.
No entry, Makefile, bootstrap or active VM was changed by this owner task.

Run from runtime18 after `. ../toolchains/host-env.ps1`:

```powershell
& $taskPython src/runtime/tests/r18/match_setup_verify_rom.py --report validation/local-20261024/match-setup-green.json
& $taskPython src/runtime/tests/r18/match_join_verify_rom.py --report validation/local-20261024/match-join-green.json
& $taskPython src/runtime/tests/r18/match_flow_verify_rom.py --report validation/local-20261024/match-flow-green.json
```
