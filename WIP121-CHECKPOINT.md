# WIP121 CHECKPOINT — raw0B exact contact and terminal lifecycle

## Scope
ROM-EXACT closure of raw `$0B` contact outcomes and `+$25==$FE` terminal behavior. Research/checkpoint only; no live timing migration and no BIN/CUE.

## Closed
At the exact held contact gate `$13AA8`:

1. If linked target `+$21==$04` and target state4 low subtype is `$08` or `$09`:
   - target remains main state4;
   - high flags are cleared/preserved-low via `ANDI.W #$00FF,+$64(A2)`;
   - exact low retype `08->06` / `09->07` is performed;
   - target pending damage `+$68=$000B` (11).

2. Otherwise callback calls `$11192`:
   - carry set -> linked target main state5, target `+$60=$0077`, target pending damage 11;
   - carry clear -> attacker main state4 + word `$0005`, attacker pending damage 3 fallback.

At `+$25==$FE`, `$13B8A` writes attacker main state7.

## Consequence
Raw `$0B` post-contact machine behavior is structurally closed. Its remaining live barrier is inherited Z provenance and faithful contact/release timing, not unknown target damage or terminal state.

## ROM anchors
- contact gate `$13AA8`
- target state4/retype path `$13AD2..$13AF4`
- target state5 path `$13B18..$13B3E`
- attacker fallback `$13B46..$13B7A`
- terminal state7 `$13B82..$13B94`

## Safety
- no physical release enabled;
- no held fallback removed;
- no `tick==12` timing migration;
- no ROM bytes persisted.

Main program SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
