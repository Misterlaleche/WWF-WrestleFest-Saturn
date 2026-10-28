# Readable Saturn fault screen

The Saturn/MODE diagnostic must freeze a snapshot at the first blocked runtime operation. A photograph of the first page must identify the phase, dependency, actor slot, character, target, raw action selector, actor state, and ROM action descriptor/callback. Other pages, selected with left/right, show the actor and target, all live participants, input and system context. The screen must remain readable on a 240-line NTSC CRT, remain static until a page change, and work without loading game assets or allocating memory at failure time.

The existing `F4C2/12614` fault means an action selector lacks a bounded implementation. The last photo did not include that selector. The new snapshot must copy the selector from the action trace as well as the actor's current raw selector, so later actor mutation cannot hide the dispatched value.

Use the existing NBG0 512x256 bitmap, a tiny built-in uppercase font, and a solid background. Preserve all gameplay and physical combat input behavior. Page navigation reads the pad only after freezing gameplay. Numeric fields are hexadecimal, as in the original ROM research. A page must label unknown actor/target data instead of inventing it. This diagnostic does not claim to fix the underlying action or combat speed.
