# Live audio VM dependencies

This directory contains project-owned glue only. The live VM includes `z80.h` and `opm.h`.

Use the exact dependency sources identified in `docs/AUDIO-ROM-EXACT-R04-CLOSURE.md`.

Do not silently replace either dependency with a different revision when validating ROM-EXACT output. Any dependency change requires re-running the 18/18 A53 hash audit.
