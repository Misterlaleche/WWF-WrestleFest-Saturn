# RUNTIME AUDIO RECONCILIATION — 2026-09-11

## Authority consumed

- original `wwfwfest.zip` ROM remains supreme;
- Audio FINAL / Complete Integration ROM evidence;
- CLEAN-R01 WP28 scheduler contract;
- CLEAN-R01 WP01 state5/action callback contract;
- WP24 Saturn audio transport.

## Critical finding

The private GitHub repository does not yet contain the current canonical gameplay runtime.
The only large historical runtime source available in the project sources is not accepted as a canonical base under the current project rules.

Therefore this reconciliation does not copy an obsolete game loop into `main`.
Instead it imports the exact runtime-facing audio boundary that can be attached to the canonical CLEAN-R01 callbacks when that runtime source is reconciled.

## Rejected adaptation

The historical `wf_audio_observer.c` is not imported. It contains native integration choices such as binding low-stamina to `fighters[0]` and deriving events from semantic runtime fields. The A12 documentation explicitly says that slot-0 role binding is a Saturn integration choice, not a newly recovered arcade role rule.

The replacement keeps raw ROM producer identity.

## Imported raw producer bindings

### Stage music table

ROM words at `0x000DEE..0x000E00`:

`3104,310A,310C,310F,3106,3104,310A,310C,310F,3106`

The low byte is dispatched as the original audio command. Invalid stage indices fail closed.

### Low-stamina producer

Recovered immediate command call site:

- producer PC: `0x0263CC`
- command: `0x0B`

The runtime layer does not infer "active human = slot 0". The owning CLEAN-R01 state must call the hook only when it reaches the equivalent recovered branch.

### Ring-out final count producer

Recovered dynamic/dataflow producer at `0x01FE44` uses raw `+$54`, initialized to `0x3165` and incremented.

Accepted raw values only:

- `0x3165 -> 0x65`
- `0x3166 -> 0x66`
- `0x3167 -> 0x67`
- `0x3168 -> 0x68`

No synthetic mapping from a generic UI count variable is required.

### Proven finisher/heavy call sites

Only the five ROM-supported move associations from `finisher_audio_evidence.csv` are admitted:

- Perfect Plex `0x0152BA -> 0x32`
- Gorilla Press `0x016DBE -> 0x32`
- DDT `0x01755A -> 0x32`
- Hogan Legdrop `0x017874 -> 0x2B`, `0x01787E -> 0x32`
- Smash Suplex `0x0164B8 -> 0x2B`, `0x0164C2 -> 0x32`

Other occurrences of command `0x32` are not globally named or auto-bound.

## Scheduler placement

WP28 remains unchanged. Audio hooks are side effects of already-proven producer/callback branches; they must not create a new frame phase or reorder:

`input -> state/CPU -> referee -> actor callbacks -> collision (branch A) -> damage (branch A) -> object/render -> publish -> VBlank ack`.

In particular:

- finisher audio stays inside the equivalent action callback at the recovered call location;
- low-stamina command stays at the equivalent recovered producer branch;
- ring-out command is emitted when the recovered raw `+$54` path calls audio;
- stage music is emitted by the matching scene/stage producer, not from a per-frame observer.

## Validation

Host tests cover all raw stage words, raw ring-out bounds, the exact low-stamina producer PC, all seven admitted finisher/heavy call sites, and rejection of nearby/unproven PCs.

Compile/test:

`cc -std=c11 -Wall -Wextra -Werror ...`

Result: PASS.

## Remaining integration gate

Actual insertion into gameplay source remains blocked only by the absence of the current canonical CLEAN-R01 gameplay runtime source in GitHub. Do not use WIP160/175/176 as the integration base merely to fill that gap.
