# TITLE visual correlation R01

This closes the bridge between the exact ROM timing contract and the independently reconstructed original arcade visual reference used by the project.

## Exact ROM timing

Dedicated title routine `$0081BA`:
- 128 synchronized prephase;
- 12 objects;
- each object = 4 + 4 + 4 + 4 + 26 = 42 synchronized iterations;
- object build = 504;
- main phase = 1480;
- total = 2112.

## Capture correlation

The project's 451-state original-title reconstruction is sampled at approximately one image for every two arcade VBlanks.

The first stable appearances of the 12 wrestler cards occur at capture indices:

```
77, 98, 119, 140, 161, 182, 203, 224, 245, 266, 287, 308
```

Every adjacent appearance differs by exactly 21 captured images.

```
21 captured images * 2 VBlanks/image = 42 VBlanks
```

This exactly matches the ROM object loop and removes the prior ambiguity about card order/cadence.

## Card order and machine-visible positions

1. Hulk Hogan — x104 y88
2. Ultimate Warrior — x168 y88
3. Hawk — x40 y88
4. Animal — x232 y88
5. Big Boss Man — x104 y8
6. Jake Roberts — x168 y168
7. Sgt. Slaughter — x168 y8
8. Ted DiBiase — x104 y168
9. Earthquake — x232 y8
10. Demolition Smash — x40 y168
11. Mr. Perfect — x40 y8
12. Demolition Crush — x232 y168

The final geometry is the original 4x3 title roster composition.

## Per-object visible phase

The capture also matches the ROM's internal 4/4/4/4/26 structure: the card begins as successive solid/transition rectangles and resolves to the wrestler card before the long hold. Saturn must reproduce these five phases as renderer events; it must not replace the entire title with a pre-rendered video.

## Architecture

`wf_title_visual_exact.c` owns only the exact title clock and object ordering.

The Saturn renderer owns:
- starfield/background animation;
- card phase drawing;
- WWF/WrestleFest logo;
- logo reflection/highlight animation;
- ROM-derived text layer.

WP19/master exclusively owns credit value and prompt selector.

The old `TITLE.ANM` remains reference/reconstruction evidence and is not the final renderer architecture.
