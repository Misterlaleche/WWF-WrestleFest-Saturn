# BLOCK-GAMEPLAY-MASTER-01 — RUNTIME-13 AUDIT

Status: **GAMEPLAY-R01 CANDIDATE**. Cumulative from frozen RUNTIME-11; RUNTIME-12/13 extend that tree. No BIN/CUE generated.

## Authority

`wwfwfest.zip` > audited FINAL ROM-EXACT packages > CLEAN-R01 workpacks > Saturn/Yaul documentation > current code > historical WIPs.

Canonical arcade main-program SHA1 remains `a726c46f350723ea73158bc78055d17d418b14e2`.
WIP160/175/176 are not gameplay bases.

## RUNTIME-12/13 additions

- WP01 State5 now has executable bodies for all 43 unique callback families in the 143-selector table; callbacks retain explicit fail-closed exits when a downstream ROM helper/state is not yet integrated.
- CPU-FINAL bridge is compiled into runtime code. CPU states 5/6/7/8/9/A have exact implemented/RTS paths where closed; RNG-dependent state6 requires externally valid arcade RNG/register context and blocks at ROM `$24CC` otherwise.
- State9 selector routing preserves the exact LoD/team path and does not synthesize choices.
- WP19 `$055A` payment dependency and WP28 `$8B3A` refill phase are implemented for the demonstrated branches.
- Real WP28 binding table currently owns `$514E`, `$8B3A`, `$1C150`, `$24E58`; remaining required phases still fail closed.
- Saturn link entrypoint is real but deliberately refuses synthetic direct-match bootstrap: it returns `WF_BLOCKED` until an audited bootstrap owner exists.

## Build gates

- HOST TESTS: PASS (`-Wall -Wextra -Werror`).
- SH-2 CROSS COMPILE: PASS, 37 translation units including Saturn entrypoint.
- FULL SATURN ELF LINK: PASS with Yaul specs; no BIN/CUE produced.
- UNRESOLVED SYMBOLS: 0.

The ELF link PASS does **not** mean gameplay can boot a fabricated match. It proves the cumulative source tree links against Yaul without unresolved symbols. The runtime entrypoint remains fail-closed because direct-match actor/global bootstrap is still not ROM-demonstrated in this block.

## WP28 frame status

`wf_runtime_frame()` does not yet complete a normal arcade gameplay frame with real owners. Branch-A currently advances through real `$514E` and `$8B3A` then stops at unbound `$2088E`. This is intentional and tested.

Real binding coverage at this checkpoint:

- `$00514E` input decode/latch -> `wf_phase_input_real`
- `$008B3A` regain-power/payment/refill -> `wf_phase_refill_8b3a`
- `$01C150` CPU controller -> `wf_phase_cpu_real`
- `$024E58` pending damage/tier -> `wf_phase_damage`

All other WP28 mandatory phases remain OPEN until their ROM/CLEAN-R01 owners are integrated.

## Fail-closed statement

No missing gameplay phase is silently treated as success. No RNG seed is synthesized. No direct-match actor/global bootstrap is invented. No frontend/audio backend was added.
