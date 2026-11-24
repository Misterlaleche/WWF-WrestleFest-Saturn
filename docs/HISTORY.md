# Historical checkpoints / Historial

La renovación deja una sola raíz de código actual. Las seis carpetas antiguas,
sus documentos y sus pruebas siguen disponibles en el historial de Git mediante
los enlaces inmutables de esta página. La validación de un bloque aislado no
acredita automáticamente la versión integrada actual.

## Previous repository layout

The previous main snapshot is
[`aedca6827c7801a88dbb1eae280830cd4776b95f`][previous-main]. The renewal removes
the independent snapshot directories from the active tree and preserves the
preceding commits and experimental branches. Each link below opens the original
README at that commit, including its external package identities and recovery
instructions.

| Historical directory | Preserved checkpoint and scope |
| --- | --- |
| [Intro][intro] | WP0 R04 standalone intro. The recorded user response is “Intro ok.”; it applies to this intro. [Original provenance][intro-provenance] preserves the confirmation and package hashes. |
| [Entrada Ring][entry] | WP01 R02 standalone TAG entrance and native SCSP audio experiment. User acceptance does not identify a physical test platform. |
| [Escena Presentadora][presenter] | WP02 R02 standalone TAG presentation. Preserved validation covers host tests and Mednafen; it does not establish a physical Saturn pass. |
| [Combate real en Saturn][combat] | WP1979 native TAG fixture, Runtime18 reference and selected research. The published checkpoint is CANDIDATE / EMULATOR-PASS / HARDWARE-PENDING. |
| [Tag Mode][tag] | Duplicate WP1979 reference plus recovered WP2015 R3 modules and restoration instructions. The [checkpoint index][tag-index] distinguishes recovered R3 sources, an external R5 disc, and unrecovered R6 sources/disc/video. |
| [modo Royal][royal] | WP1980 HW1 standalone Royal fixture and its measurements. HW1 physical validation and the minimum arcade speed remained pending. |

These records describe their original checkpoints. They do not certify the
current build, and this cleanup does not recover the missing WP2015 R6 material.
Historical binary resources remain in the external packages identified by those
records; their presence in a manifest does not mean they are stored in Git.

## Hardware observations and audio research

The following evidence remains useful when investigating regressions or revisiting
an implementation decision:

- [MODE + ROSTER TEST03][mode-roster]: the isolated combined block passed on real
  Saturn NTSC-U + MODE on 11 September 2026. The record describes the display
  synchronization, transparent background and localized confirmation update.
- [Audio hardware validation][audio-hardware] and the [TEST05 audit][audio-test05]:
  the isolated WP24 transport passed long PCM playback on real Saturn. The
  evidence covers the SCSP slot-31 ring and CA-driven refill.
- [Audio R04 software closure][audio-r04] and [R05 integration readiness][audio-r05]:
  live Z80/Nuked-OPM research, A53 comparisons, producer mappings and unresolved
  integration/performance gates. The [historical audio source][audio-source] and
  [tests][audio-tests] remain available with these records. This live-VM path is a
  separate implementation from the current Jo finite-WAV playback path.
- [The former root documentation][old-docs] and [handoff][old-handoff] preserve
  the earlier ownership and integration context. Statements about the runtime
  being absent describe that older repository state.

## Pinned audio source used by the current tools

The current host audio reference derives from `audio-2-production-ready` at
[`6d6a5705c87e9042529116415770f4bc74c0e2e8`][audio-pin]. Its exact file identities
and Nuked-OPM pin are recorded in
[tools/audio_reference/PROVENANCE.json](../tools/audio_reference/PROVENANCE.json).
Use the immutable commit and recorded hashes when reproducing that reference.
The applicable notices are indexed in [THIRD-PARTY.md](THIRD-PARTY.md).

[previous-main]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/tree/aedca6827c7801a88dbb1eae280830cd4776b95f
[intro]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/Intro/README.md
[intro-provenance]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/Intro/PROCEDENCIA.json
[entry]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/Entrada%20Ring/README.md
[presenter]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/Escena%20Presentadora/README.md
[combat]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/Combate%20real%20en%20Saturn/README.md
[tag]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/Tag%20Mode/README.md
[tag-index]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/Tag%20Mode/INDICE-CHECKPOINTS.md
[royal]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/modo%20Royal/README.md
[mode-roster]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/docs/MODE-ROSTER-HARDWARE-VALIDATION.md
[audio-hardware]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/docs/AUDIO-HARDWARE-VALIDATION.md
[audio-test05]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/tree/aedca6827c7801a88dbb1eae280830cd4776b95f/tests/audio/complete-test-05
[audio-r04]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/docs/AUDIO-ROM-EXACT-R04-CLOSURE.md
[audio-r05]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/docs/AUDIO-INTEGRATION-READINESS-R05.md
[audio-source]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/tree/aedca6827c7801a88dbb1eae280830cd4776b95f/src/audio
[audio-tests]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/tree/aedca6827c7801a88dbb1eae280830cd4776b95f/tests/audio
[old-docs]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/tree/aedca6827c7801a88dbb1eae280830cd4776b95f/docs
[old-handoff]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/blob/aedca6827c7801a88dbb1eae280830cd4776b95f/HANDOFF.md
[audio-pin]: https://github.com/Misterlaleche/WWF-WrestleFest-Saturn-Private/tree/6d6a5705c87e9042529116415770f4bc74c0e2e8
