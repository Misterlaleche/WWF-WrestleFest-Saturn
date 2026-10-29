# Residency extension for the observed controller takeover

This is an asset-only extension of the frozen gameplay revision 4. It includes
the final 7372-step demonstration and the ordinary controller path recorded in
`measure/candidate-2-controller/report.json`. It does not change gameplay source,
the main input tape, the native working copy, or the 211-page ROM header.

## File to integrate

Use `HOTSPR-ROM-v4-plus-controller-32.BIN` as the disc's `HOTSPR.BIN`.
Keep the existing gameplay sources, main tape and ROM-page header.

| Item | Original v4 | Union with captured controller path |
|---|---:|---:|
| Tile IDs | 6357 | 6474 |
| Exact dictionary chunk size | 32 bytes | 32 bytes |
| Atlas file | 698304 bytes | 711616 bytes |
| Resident LWRAM | 631040 bytes | 643456 bytes |
| Available cold tile slots | 150 | 53 |

The union adds 117 tile IDs. It remains below the existing 650240-byte LWRAM
allocation and 114688-byte HWRAM index limit. The 51792 map bytes are padded to
51808; 20107 dictionary chunks occupy 643424 bytes, rounded to 643456 resident
bytes. The header remains 16384 bytes. Every reconstructed tile equals the
canonical `SPRITES.PAK` byte-for-byte. `additional-tile-ids.txt` lists the exact
117 additions, and `union-tile-ids.txt` lists the complete sorted selection.

Atlas SHA-256:

```text
ba8a5be1869cb6a8e101eaf06bff8506bb51f7d5258201b5290b9aea1ca377d9
```

## Reconstructed ordinary input

`takeover-input.csv` is the immutable host input sequence, including the genuine
demo prefix before takeover. Format: decimal zero-based logical input frame and
active-low byte, no header. A value persists until the next event. There are 22
events, including eight prefix events from the unchanged main tape.

The emulator applies the first physical LEFT event at frame 540. Its observed
demo flag becomes zero at emulator frame 541 with game counter 66. Input row 66
therefore takes over. The emulator snapshots occur while waiting for VBlank
acknowledgement: actors at game counter N correspond to host completed step N+1.
Consequently, the final recorded game counter 512 is validated through host step
513. This alignment is established by matching the observed actor states, not by
assuming that emulator frame numbers always advance one logical step.

| Logical input frame | Physical level | Active-low byte |
|---:|---|---:|
| 66 | LEFT | 253 |
| 113 | release | 255 |
| 125 | UP | 247 |
| 161 | release | 255 |
| 173 | RIGHT | 254 |
| 220 | release | 255 |
| 232 | DOWN | 251 |
| 268 | release | 255 |
| 292 | A | 239 |
| 300 | release | 255 |
| 328 | B | 223 |
| 336 | release | 255 |
| 364 | C | 191 |
| 372 | release | 255 |

`event-mapping.json` retains the corresponding physical/emulator timestamps.
`input-provenance.json` records hashes of the original report, physical schedule,
unchanged main tape and reconstructed input.

## Validation

All 527 started emulator snapshots, covering four actors each, match the host:
2108 actor records × 17 observed fields = **35836 exact field comparisons**.
The fields include state, action, coordinates, stamina, pose, legal/control role
flags, relationship link, animation counters and fresh input. The observations
are retained in `emulator-actor-observations.csv`.

With the original v4 atlas, this logical path reads 117 additional sprite tiles
and zero ROM pages. The Saturn report records three CD window misses and 24
sectors. The host's individual tile reads occur at game counters 394, 401, 451,
456, 463 and 468; a CD window can satisfy more than one individual tile read.

With the union atlas:

- The reconstructed manual path completes all 513 host steps with zero ROM and
  sprite reads and no guarded owner block. Its full role/position/health trace
  is byte-identical to the original-atlas host run.
- The unchanged main tape completes at 7372 with the same two legal TAGs, human
  win, final counters and 131-VBlank result wait. It also has zero ROM and sprite
  reads and no guarded owner block. Its full trace is byte-identical to the
  frozen v4 trace.

Run `python3 wp1979/audit_gameplay/residency_takeover_v1/verify_union.py` within the
full pinned workspace. It verifies the frozen hashes, rebuilds the exact atlas,
checks every decoded tile, replays both paths and repeats all observed-field
comparisons. It only writes its `verification/` outputs.

## Scope

This extension covers these two logical input paths. It does not establish
residency for arbitrary manual play. The larger atlas can change the initial
preload duration; removing CD pauses can also change absolute emulator frame
offsets. Preserve logical input frames when reproducing the identical route, or
measure a new ordinary physical-controller sequence separately. Final Saturn
image performance remains a native-emulator measurement by the integration task.

The original v4 atlas, tape, manifests and code remain frozen in their existing
locations. This directory's `wp1979-residency-takeover-v1.json` describes only the
new residency revision.
