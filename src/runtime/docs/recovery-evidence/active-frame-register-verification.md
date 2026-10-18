# Active frame: direct-ROM register and RNG verification

## Evidence boundary

Reference ROM: `main68k.bin`, SHA-1 `a726c46f350723ea73158bc78055d17d418b14e2`.
The oracle executes the original MC68000 bytes with Unicorn 2.1.4, explicitly
selecting `UC_CPU_M68K_M68000`. This emulator is a host-test dependency only;
no production source links, embeds or executes a CPU emulator.

`tools/verify_runtime17_frame_rom.py ROM build/probe_active_frame` runs the native
fixture `tests/r17/active_frame_driver.c`, captures its initial raw RAM, then
executes the ROM scheduler itself from `$F9A` through the `$10E8` VBlank wait.
Observation hooks at the 29 real owner return addresses compare all eight full
32-bit data registers plus `$1C005C`, rather than checking only that RNG changed.
No ROM gameplay calls are skipped or replaced. Input hardware reads are released
(`$FFFF`), and video memory is writable mapped memory. The instruction limit
fails the test if the ROM does not reach the intended boundary.

After setting the hardware ACK bit, the ROM executes `$10E8` back to `$F9A`.
An observation hook stops before the new frame begins, because reusing Unicorn's
backward end address after executing a cached block did not reliably stop there.
This hook does not modify RAM, registers or game execution. The native post-ACK
D0-D7/RNG tuple and counter/branch alias are compared separately.

Observed result: **all 29 owner tuples and the post-ACK tuple match**. Final RNG
is `AF7C5969`. This is one bounded active frame, not proof of all possible game
states.

## Fixture conditions

- Credited TAG route, branch A, counter zero, RNG seed `13579BDF`.
- Actor 0 is active, unsuppressed at `$F4C2`, in the terminal callback of raw
  action `$7C`. The actual `$1A5A0` callback and `$10BD0` movement execute:
  X changes from `$0200` to `$01E0`, selector changes to `$0017`, and Z=`$0140`
  makes the actor visible. This is not the old state-7/F4C2-suppressed fixture.
- Actor 1 is an active passive target; its own action remains suppressed. This
  fixture does not prove active CPU decision-making or two simultaneous actions.
- The referee and fixed-four owners execute. Four actors enter the nonempty
  render batch: the moving wrestler, referee and two fixed objects.
- The render adapter resolves actual record bytes from the pinned ROM tables and
  invokes `wf_r17_render_record_shadow`, including admitted record preconditions.
  It never assigns the expected D6/D7 tuple from fixture constants.
- `$18C4` uses the native `$6E34` owner. Its only supplied participant callback is
  raw hardware input; there is no substitute TAG gameplay callback.
- `$169E=$3000` and `$167B=1` select real early-return paths in some later owners.
  The comparison establishes those paths, not their unentered alternatives.
- The next branch selector after ACK is B: `$1C0083` aliases the low byte of the
  incremented long at `$1C0080`. The oracle does not claim a complete second
  branch-B frame.

## Corrections exposed by the oracle

| ROM boundary | Correction |
|---|---|
| `$514E`, `$5170`, `$5188`, `$5194..$51CE` | Respect role bit 7; clear the whole derived-input word above its low direction nibble; reproduce WORD/BYTE input register writes and iterator D0. |
| `$8B44`, `$8B9A`, `$8BA2`, `$8CEC` | Reproduce MOVEQ/WORD refill-loop register state and DBRA D1. |
| `$2088E..$208E8` | Reproduce iterator, table index, position registers and full-width MULU result in D4. |
| `$20958`, `$20A24..$20AA0` | Reproduce outer-loop D5/D6 and active TAG role masking in D0. Nontrivial candidate-helper shadow paths remain outside this fixture. |
| `$26936..$26A40` | Use active bit 7, WORD accumulation, signed DIVS quotient/remainder layout, clamping and persistent D3-D7 state. |
| `$29842..$2987E` | Reproduce scene-index WORD D0 and final camera-Y WORD D1. |
| `$F574`, `$FD00..$FDB2` | Reproduce pair iterator and timed-flags register writes/DBRA D4. |
| `$F5E4` | Restore D0-D4 at the relation callback's MOVEM boundary while retaining the RNG change in RAM. Caller facing/distance computations now feed the callback registers. |
| `$F18A`, `$F1D0..$F1D8` | Reproduce iterator and the WORD state dispatch offset. |
| `$1C150..$1C1E8` | Respect the low-byte global gate and reproduce the outer CPU DBRA D6. |
| `$1F91A`, `$1FB04`, `$204B2`, `$20556` | Reproduce the exercised referee dispatch, midpoint and acquisition-loop register state. |
| `$205AC..$20604` | Apply the missing right shift in the direction-table index; use the ROM's unsigned wrapped distance test; preserve facing when the ROM does not clear it; reproduce D0/D1 writes. |
| `$1A5E8/$1A5EA` | Raw `$7C` terminal action clears only the low WORDs of D1/D2. These changes survive the dispatcher, which saves D0 but not D1/D2. |
| `$7A28..$7A30` | Write the WORD dispatch index after storing the random delay; the delay itself is not the owner-exit D0. |
| `$10F2` | Synchronize legacy `$0082/$0083` aliases after incrementing the frame-counter LONG. External initializers must seed these aliases coherently. |

The register work is performed at the corresponding native operations. There
is no per-owner replacement with precomputed golden exit tuples.

## Smaller reproducible regressions

`tests/test_runtime17_active_rom.c` and `tools/verify_active_rom.py` independently
exercise an active `$F4C2` raw `$7C` terminal action with nonzero upper register
words. The oracle compares D0-D7 and both complete actor records, then executes
`$21B4` and verifies RNG `8AD04688`. Before the fix, the native D1 assertion failed.

The same tool regenerates the `$F560` reference used in `test_runtime17.c`:
D0-D7=`00000009,00000000,00000113,00000008,0000FFFF,00000000,00000000,00000000`,
RNG=`C1020305`. The former test incorrectly expected RNG D0 to escape a MOVEM.
The camera test now uses active bit 7 and also checks that active byte `1` does
not qualify. The input test poisons the high byte before checking its clearance.

## Limits

This verifies native register/RNG composition for the stated frame and supporting
fixtures. It does not establish full framebuffer or sprite-pixel equality,
Saturn hardware timing, VBlank interrupt register behavior, all metasprite
record kinds, every action, all target/referee helper branches, or complete
branch-B gameplay. The render adapter verifies the admitted native register
contract, not a complete Saturn renderer. The ROM is supplied externally to the
host oracle and is not embedded in the production runtime.

## Independent review follow-up: gather and payment child clobbers

A separate read-only ROM review found two paths outside the original frame fixture:
`$204FA` clears D1 with MOVEQ before counting matching actors, and `$055A` payment
mode 2 changes the D0 low word to 3 at `$0582`. The native gather helper and refill
binding now perform those writes. The gather also models its D0 MOVEQ/DBRA result.

`tests/r17/register_edge_driver.c` and `tools/verify_register_edges_rom.py` provide
four direct-ROM regressions. They start with nonzero upper halves in every D
register and execute unmodified `$204B2` (including `$204FA`) or `$8B3A` (including
`$055A`), without gameplay hooks. All eight full D registers match for midpoint
bit 6 clear, midpoint bit 6 set, successful paid refill and final-slot failed
payment. The entire referee record is compared for midpoint cases; credit and
stamina results are compared for payment cases. All four pass. Before the fixes,
assertions failed with D1=`ABC00160` instead of `00000160` and D0=`2` instead of `3`.

The test-only driver includes the referee source to access its private helper;
it must link the normal CORE list **excluding** `src/wf_r17_referee.c`, avoiding
both duplicate symbols and any new production test API. Its source dependency
list should still include that referee file. Invoke the differential script with
canonical ROM and compiled driver paths, with the host-only Unicorn package on
PYTHONPATH. These cases supplement the bounded frame evidence; they do not expand
its whole-game or hardware claims.
