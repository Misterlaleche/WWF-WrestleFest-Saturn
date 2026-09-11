#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
logic=(root/'src/wf_intro_presentation.c').read_text()
harness=(root/'harness/main.c').read_text()
assert 'wf24_command_dispatch' not in logic+harness
assert 'smpc_peripheral' not in logic+harness
assert 'WF_INTRO_EVENT_TITLE_HANDOFF' in logic and 'WF_INTRO_SCENE_TECHNOS' in logic
assert 'WF_ARCADE_BOOT_WAIT_VBLANKS' in logic
assert 'WF_ARCADE_BOOT_FADE_FRAMES_PER_STEP' in logic
assert 'WF_INTRO_EVENT_FAULT' in logic and 'vdp2_tvmd_display_clear' in harness
print('PASS: no private input/audio routing, title boundary explicit, fail-closed harness')
