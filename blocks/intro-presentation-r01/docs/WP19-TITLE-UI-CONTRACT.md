# WP19 -> INTRO/TITLE credit/prompt contract

## Ownership

WP19 is the sole owner of:
- coin accounting;
- credit value;
- coinage/DIP/service semantics;
- credit consumption;
- selection of the title prompt.

INTRO/TITLE owns only the visual representation of the values supplied by WP19/master.

The runtime boundary is a read-only snapshot:

```c
typedef struct wf_intro_title_ui_snapshot {
    uint8_t credit_value;
    uint8_t prompt_selector;
    bool prompt_visible;
} wf_intro_title_ui_snapshot_t;
```

The title block MUST NOT:
- maintain a local credit counter;
- increment on a Saturn COIN mapping;
- decrement on START;
- infer prompt_selector from credit_value;
- assume zero credits at scene entry;
- hard-code `CREDIT: 0` as a static bitmap.

## Visual contract

Every title UI refresh:
1. draw the ROM-derived `CREDIT:` label;
2. render the WP19/master-provided `credit_value` using the shared ROM-derived text/glyph renderer;
3. if `prompt_visible` is true, draw exactly the prompt selected by `prompt_selector`;
4. otherwise remove/hide the prompt through the renderer contract.

The value and selector are pass-through data. Their semantics stay with WP19/master.

## ROM-exact scope / blocked items

The ownership boundary is CLOSED.

Exact title text coordinates, glyph/tile IDs, palette-bank usage and layer priority must come from the audited Presentation/WP25/WP26 evidence. They are not guessed in this module. Until those renderer bindings are present, those details remain BLOCKED rather than being approximated.

No claim is made that a particular scene always displays `CREDIT: 0`.
