# Third-party notices

This page indexes notices included with particular dependencies. It does not
assign a license to the whole repository or grant rights to the original game,
its assets, or the project's implementation as a whole. The complete notices in
the linked files apply to their respective material.

## Included source dependencies

| Component and location | Recorded notice | Scope and reference |
| --- | --- | --- |
| Jo Sega Saturn Engine, `sdk/jo_engine/` | The notice in `core.c` names Copyright (c) 2012–2026 Johannes Fetz and contains three-clause BSD-style terms. | The engine sources retain their own notices. Read the complete [notice in core.c](../sdk/jo_engine/core.c) and any additional notices in the files being used. |
| zlib 1.2.11 subset, `port/zlib/` | Copyright (C) 1995–2017 Jean-loup Gailly and Mark Adler; zlib license. | [LICENSE.txt](../port/zlib/LICENSE.txt) contains the notice. [PROVENANCE.txt](../port/zlib/PROVENANCE.txt) records the upstream archive used for the subset. The current port includes marked SH-2 adaptations. |
| Z80 core, `tools/audio_reference/z80.c` and `z80.h` | MIT License; Copyright (c) 2019 Nicolas Allemand. | Host audio reference dependency. The full [LICENSE.z80](../tools/audio_reference/LICENSE.z80) accompanies the sources. |
| Nuked-OPM, `tools/audio_reference/opm.c` and `opm.h` | GNU Lesser General Public License, version 2.1 or later; the source notice names Copyright (C) 2020, 2026 Nuke.YKT. | Host audio reference dependency. See [the source notice](../tools/audio_reference/opm.c) and the full [LICENSE.Nuked-OPM](../tools/audio_reference/LICENSE.Nuked-OPM). |

Jo's included notice requires retaining its copyright, conditions and disclaimer
with source redistribution, reproducing them with binary redistribution, and
obtaining permission before using the named author or contributors to endorse a
derived product. The linked source contains the complete wording.

The zlib notice requires preserving attribution, identifying altered versions
and retaining the notice. Its historical provenance file describes the subset
at import; the current code has SH-2-specific changes documented in
[adler32.c](../port/zlib/adler32.c), [inffast.c](../port/zlib/inffast.c) and
[wf_sh2_bits.h](../port/zlib/wf_sh2_bits.h). It should not be described as an
entirely unmodified upstream zlib tree.

## Audio reference provenance

[tools/audio_reference/PROVENANCE.json](../tools/audio_reference/PROVENANCE.json)
records the host-only scope, source paths and SHA-256 identities. It pins the
project audio reference to `6d6a5705c87e9042529116415770f4bc74c0e2e8` and
Nuked-OPM to `f209e6ed3712032b641d53ce8fb24824eae6adc3`. These notices belong to
the corresponding source dependencies; they do not license generated game audio
or transfer rights to the original sound ROM.

## External tools and original game material

The external compiler/SDK archive and other separately installed build tools
retain their respective licenses and notices. This index does not replace the
notices supplied with those distributions.

WrestleFest, WWF and Sega Saturn names and branding, the original game code,
graphics, music and other original assets remain subject to the rights of their
respective rights holders. This is an independent port project with no claimed
affiliation or endorsement. Original ROM archives, BIOS files and protected
graphics/audio/disc payloads are not supplied by this source repository; build
instructions and manifests refer to separately supplied inputs.

The presence of third-party licensed code does not establish a blanket license
for every file in this project. Preserve the applicable file-level notices and
provenance when copying source or distributing a build.
