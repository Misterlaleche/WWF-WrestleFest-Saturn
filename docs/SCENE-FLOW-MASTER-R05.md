# Scene Flow Master R05

This module resolves WP30 O30-06 by providing one root-state arbitration point without absorbing local scene state machines.

It does not render, load assets, poll gameplay input, implement local timers/fades, call audio directly, or invent transitions.

A root transition is accepted only when (from,to,evidence) matches a proven authority row. Unknown edges fail closed.

Local ownership remains unchanged:
- INTRO/PRESENTATION owns its local sequence and hands off to TITLE;
- MODE+ROSTER TEST03 behavior is imported under CLEAN-R01, not by replacing main.c;
- WP12 owns prematch/entrances;
- gameplay branch owns gameplay/CPU/rules/objects;
- WP13 owns post-win interstitial/progression semantics;
- WP14 owns Continue/Game Over semantics;
- Records FINAL owns ranking/initials;
- Credits FINAL owns credits body;
- WP17 + Attract/Bio FINAL own demo/bio semantics while master owns root orchestration.

R05 is a routing contract, not a claim that all destination modules are already integrated.

Validation:
- host compile/test PASS with -Wall -Wextra -Werror;
- SH-2 freestanding object compile PASS;
- full Saturn ELF link not claimed;
- hardware PASS not claimed;
- no BIN/CUE/ISO generated.
