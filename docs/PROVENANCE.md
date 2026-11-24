# Source provenance / Procedencia

## What this revision represents

The 25 November 2026 repository refresh imports the newest WrestleFest source package recovered for this update: **P1-03B BOOTFIX CANDIDATE**, dated 22 November. It also restores the adapted COMMON build support that later ZIPs omitted. It does not merge the historical live-audio branch into the game, alter the gameplay implementation, or produce a new disc.

La actualización incorpora P1-03B y restaura COMMON desde su procedencia verificada. El commit nuevo de GitHub identifica esta importación; no debe confundirse con el commit de trabajo registrado dentro del paquete original.

The machine-readable [source import record](provenance/source-import.json) contains the package identities, original executable identity, all recovered runtime-file hashes and the 15 COMMON file hashes. The runtime files are preserved byte for byte from P1-03B. Documentation, ignore rules and the controller host-test harness have separately documented maintenance changes.

## Verified source packages

| Package | Role | SHA-256 |
| --- | --- | --- |
| `WrestleFest_P1_03B_BOOTFIX_Fuentes_y_Pruebas_20261122.zip` | Current source and compact build/test evidence; 3,389,777 bytes | `e90338277f582fc6babf4c1f2c9543a079a91d0efc7537e2d10bb6da3adb707f` |
| `WrestleFest_Saturn_R03_fuentes.zip` | Exact inherited adapted COMMON files | `bbf57da48ad51cbb4a37c0260c1475087a5340b86f4d1c05af63ac3451308f4f` |
| `WrestleFest_P1_03_AUDIO_Fuentes_y_Pruebas_20261121.zip` | Predecessor comparison and continuity evidence | `9c55171b4b264b92300dcd30d15ded1f120d394c1a7aaef828aadc31f1b37614` |
| `WrestleFest_FIX2R_Fuentes_y_Pruebas_20261119.zip` | Earlier cumulative patch/continuity evidence | `032e1bad4b50a385306f7d15985fa72c63cbad749c8f312108034516563bd390` |

These are source-package identities, not promises that every historical package is stored inside Git. Newer-date/title searches found no later accessible matching source package for this refresh; that does not establish that no unshared local work exists.

## Recorded P1-03B build identity

The recovered [original commit record](../evidence/p1-03b-20261122/original-commit.txt) identifies:

| Field | Recorded value |
| --- | --- |
| Source commit | `467148a7afec20d6d0cf10427af4ce74022cf686` |
| Source base | `1aff90fed3b4e0dd306b0768eea9fe70ccdcbbe6` |
| Original working branch | `wrestlefest/p1-03-boot-asset-fix` |
| Original `game.raw` size | 529,504 bytes |
| Original `game.raw` SHA-256 | `92ae0c34b4c55983914e209710574327c9e77e58dc4df78919d931149138064e` |
| Recorded linker end | `0x060D9FF0` |
| Reserved boundary | `0x060DC000` |
| Recorded static margin | 8,208 bytes |

The archive's raw executable was hashed during recovery. Compiled executables are not imported into this source repository. The original [linker map](../evidence/p1-03b-20261122/game.map) and [build log](../evidence/p1-03b-20261122/make_bootfix_frontend.log) are retained as **recovered evidence**, not as a fresh build of this GitHub commit. Original absolute paths in logs describe the original environment and are not build prerequisites.

## COMMON restoration

The P1-03B archive contained the current source but omitted `sdk/Compiler/COMMON/`, despite including its makefile. The compiler installer only extracts `LINUX/` and `WINDOWS/`, so running it alone could not repair that omission.

The 15 adapted COMMON files were recovered from the hash-verified R03 source archive and checked against its original source manifest. The inspected cumulative P0/GFX1/P1 patches do not modify COMMON. The restored files are inherited build inputs; they are not replacements taken from a newer stock SDK.

Key identities:

| File | SHA-256 |
| --- | --- |
| `sdk/Compiler/COMMON/jo.linker` | `c2505a94b005ce5178e6a8b1a9f96d560e8617afedb7fa7b81889f79a08b5274` |
| `sdk/Compiler/COMMON/jo_engine_makefile` | `16200f0b6db9257b7d6b0b2ea5a8080e29b3a76c88000176dcade10610b34629` |
| `sdk/Compiler/COMMON/IP.BIN` | `490b2a86c91a32fb98e1c246f966507fb9890d180de8b6372bbabd90f04334b6` |

`IP.BIN` is Jo's boot template, not a console BIOS. The revised `.gitignore` retains COMMON while excluding the external compiler executables.

## Repository cleanup and retained evidence

The previous GitHub main was `aedca6827c7801a88dbb1eae280830cd4776b95f`. The new publication descends from that commit. Earlier independent source trees and obsolete root instructions are removed from the active layout; [HISTORY.md](HISTORY.md) links to their unchanged contents at immutable commits. Experimental branches and previous commits are retained.

The current source's frozen test references, generated runtime tables, small RAM fixtures, third-party notices and relevant technical records remain. Superseded planning documents and misleading R03-only root recovery directions are omitted from the active checkout; their original bytes remain in the identified source packages.

The original arcade ROM, full generated assets, sound tracks, compiler executables, BIOS and disc images remain external. `asset-lock.json` identifies the prepared game payloads required by the current build. [THIRD-PARTY.md](THIRD-PARTY.md) indexes the applicable source notices.
