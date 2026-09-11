# RUNTIME AUDIO RECONCILIATION — 2026-09-11

## Authority consumed

- original `wwfwfest.zip` ROM remains supreme;
- Audio FINAL / Complete Integration ROM evidence;
- CLEAN-R01 WP28 scheduler contract;
- CLEAN-R01 WP01 state5/action callback contract;
- WP24 Saturn audio transport.

## Critical finding

The private GitHub repository does not yet contain the current canonical gameplay runtime.
The available CLEAN-R01 FINAL workpacks define contracts, scheduler order and many exact behaviors, but do not form one complete linked gameplay source tree.

Therefore this reconciliation does not copy an obsolete game loop into `main`.
Instead it imports the exact runtime-facing audio boundary that can be attached to canonical CLEAN-R01 callbacks when that runtime source is reconciled.

## Audit correction — producer PC versus JSR PC

The first reconciliation used the term "callsite PC" for addresses such as
`0x0152BA`. The ROM evidence shows that these are the **producer instructions**
(`MOVE.W #command,D0`). The following `JSR $00002052` is four bytes later.

The API now uses `producer_pc` explicitly.

Examples:

- Perfect Plex producer `0x0152BA`; JSR `0x0152BE`;
- Gorilla Press producer `0x016DBE`; JSR `0x016DC2`;
- DDT producer `0x01755A`; JSR `0x01755E`;
- low-stamina producer `0x009088`; JSR `0x00908C`.

This prevents later SH-2 integration from treating the immediate-load address as the actual 68000 JSR address.

## Rejected adaptation

The historical `wf_audio_observer.c` is not imported. It contains native integration choices such as binding low-stamina to `fighters[0]` and deriving events from semantic runtime fields. The A12 documentation says that slot-0 role binding is a Saturn integration choice, not a newly recovered arcade role rule.

The replacement keeps raw ROM producer identity.

## Imported raw producer bindings

### Stage music table

ROM words at `0x000DEE..0x000E00`:

`3104,310A,310C,310F,3106,3104,310A,310C,310F,3106`

The low byte is the recovered FM command. Invalid stage indices fail closed.

### Low-stamina producer

Recovered producer:

- `MOVE.W #$310B,D0` at `0x009088`;
- `JSR $00002052` at `0x00908C`;
- command `0x0B`;
- active actor `+$66 < $0018`;
- latch `$1C0166` prevents every-frame replay.

A03 and `maincpu_sound_calls_complete.csv` identify this path as the recovered low-stamina warning producer.

The complete manifest also contains `0x0263CC -> 0x0B`, but that producer remains `REGISTERED_NOT_IDENTIFIED`. Equal command identity is not semantic proof, so the runtime hook rejects `0x0263CC` as low-stamina.

The runtime layer does not infer "active human = slot 0". The owning CLEAN-R01 state must invoke the hook only when it reaches the equivalent recovered branch/latch condition.

### Ring-out final count producer

Recovered dynamic/dataflow producer:

- send occurs at `0x01FE44`;
- raw `+$54` is initialized to `0x3165` and incremented;
- accepted range is exactly `0x3165..0x3168`.

The API now requires **both** producer PC `0x01FE44` and a valid raw `+$54` value. A matching low byte elsewhere is not sufficient.

### Proven finisher/heavy producers

Only the ROM-supported producer associations from `finisher_audio_evidence.csv` are admitted:

- Perfect Plex `0x0152BA -> 0x32`;
- Gorilla Press `0x016DBE -> 0x32`;
- DDT `0x01755A -> 0x32`;
- Hogan Legdrop `0x017874 -> 0x2B`, `0x01787E -> 0x32`;
- Smash Suplex `0x0164B8 -> 0x2B`, `0x0164C2 -> 0x32`.

Other occurrences of `0x32` are not globally named or auto-bound.

## Scheduler placement

WP28 remains unchanged. Audio hooks are side effects of already-proven producer/callback branches; they must not create a new frame phase or reorder the frame.

In particular:

- finisher audio stays inside the equivalent action callback;
- low-stamina command stays at the equivalent recovered branch/latch;
- ring-out command is emitted only from the equivalent `0x01FE44` producer;
- stage music is emitted by the matching scene/stage producer, never by a per-frame observer.

## Validation

Two independent host tests are required:

### Command bus

Covers:
- uninitialized/null bus fail-closed;
- STOP `0x00`;
- MUSIC boundaries `0x01/0x12`;
- registered-no-producer boundaries `0x13/0x1E`;
- special fade `0x1F`;
- SFX boundaries `0x20/0x81`;
- invalid `0x82`;
- backend failure propagation.

### Runtime producers

Covers:
- all ten raw stage words;
- invalid stage index;
- exact ring-out producer PC and both raw bounds;
- rejection of nearby producer/raw values;
- exact low-stamina producer PC and rejection of the following JSR PC;
- all seven admitted finisher/heavy producers;
- rejection of an unproven producer.

Compile/test target:

`cc -std=c11 -Wall -Wextra -Werror`

Status after this audit correction: source/test set reviewed; integration into the canonical gameplay runtime remains open.

## Remaining integration gate

Actual insertion into gameplay source remains blocked by the absence of the current canonical CLEAN-R01 gameplay runtime source in GitHub.

Do not use WIP160/175/176 as the integration base merely to fill that gap.
