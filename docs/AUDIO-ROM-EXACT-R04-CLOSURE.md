# AUDIO ROM-EXACT SOFTWARE CLOSURE R04

Status: **SOFTWARE CLOSED / REAL SATURN FINAL VALIDATION PENDING**.

## Closure

R04 closes the remaining software-side music transport gap without introducing PCM loops.

- Music commands `0x01..0x12`: live Z80 + pinned Nuked-OPM VM.
- First 12 seconds: **18/18 byte-exact** against canonical A53 hashes.
- Output: signed mono PCM16BE, 44.1 kHz.
- WP24 keeps the hardware-proven slot31 / 64 KiB / two-half / CA-driven transport.
- The only transport change is an optional live renderer callback used to fill the safe half.
- `0x1F` is sent back into the original Z80 driver VM. No PCM-domain linear fade is permitted in ROM-EXACT mode.
- SFX/voice commands `0x20..0x81` keep the audited A15/A16 model and bank semantics.
- Existing producer gating and CLEAN-R01 owner rules remain authoritative.

## Hardware gate

The live VM has not yet been validated for real-time CPU budget under full VDP1/VDP2/gameplay load on physical Saturn.

Therefore do **not** mark AUDIO hardware-final until real Saturn + MODE validation passes.

## Canonical R04 package

The original R04 ZIP is **superseded as an audited package** because its `SHA256SUMS.txt` used absolute paths and referenced a non-existent `SHA256SUMS.txt.tmp`. This was a packaging/audit-manifest defect only; implementation was not changed.

Canonical master checkpoint:

`WWF-SATURN-AUDIO-ROM-EXACT-SOFTWARE-CLOSED-R04-MASTER-REAUDITED.zip`

SHA256:

`c4898b654eb105a88cc24500de96c00b68d188582aa7d7f7dbb13f1b3315ae36`

Superseded original package SHA256:

`b23f43424407c8379a45c1e6fcbb482065bf0c429856ad988ca896d2e596e5c0`

Do not use the superseded ZIP as the canonical audited handoff.

## Project-owned source SHA256

- `wf_audio_live_vm.c`: `f1500df193b7aec9470b965fdd72c2aa3784b54f8457ea3b5fe45bf1443892a6`
- `wf_audio_live_vm.h`: `fa7438da92efcf6d1999de64f41d93baf18dc8139731a0b39efa235aa289dd3b`
- `wf24_livevm_adapter.c`: `73ae67b7cafad38e1a44aa40a9951c86fd698068261e53f7e23c04d1b5806ae8`
- `wf24_livevm_adapter.h`: `0c6345988e2634bc570c944324b6f82032f4e282c8554f1210b34aefc76067b5`
- `wf24_music_stream.c`: `a633bf3f3747c4894b12f578b0ec6edc548f0ab7182f5d4d409b8ac2b8d0977e`
- `wf24_music_stream.h`: `d6d6b441af229e48db9349fc72158c3452fd97a8629f7b8ae5a45932fe959e41`
- live-stream host test: `2674b227bc089d25ab83b3c1f6d3b245b7df441707e76f398245fbd211358bd3`

## Dependency provenance

The VM depends on exact unmodified sources from the user's supplied dependency archives:

- Nuked-OPM `opm.c` SHA256 `983beecc471aebd8884fcc3aa48c50b1a26d4280ddbf1025b5196b540cf24188`
- Nuked-OPM `opm.h` SHA256 `186e042d1cd80c8fdf2e1a07d7c123bbe9a0d42e39518c50889db140a9ec4c39`
- z80 `z80.c` SHA256 `d82e99d025b4ec0d0d0dac29ef83e4bb41c7d2fc15592e13be4057bbf17f0698`
- z80 `z80.h` SHA256 `d27e46daf7d3c661ace78918fe5315632bc82c79e351126709c541b80e01f5aa`

These third-party sources are not duplicated here. Preserve their upstream licenses when vendoring them into a build tree.

## Runtime integration

1. Initialize the live VM once from the recovered 64 KiB sound ROM, decoded AUDPCM bank0 material and index.
2. For a proven music command `0x01..0x12`, start the VM with that exact byte and then call `wf24_stream_start_live()`.
3. Bind `wf24_stream_ops_t.render_live_be` to `wf24_livevm_render_be`.
4. Keep existing serialized slot31 prepare/start/stop/CA callbacks.
5. Do not add a new WP28 scheduler phase.
6. On `0x1F`, call `wf24_livevm_send(adapter, 0x1F)`; do not apply a final-PCM scalar fade.
7. Keep SFX/voice semantics under the audited `0x20..0x81` model.
8. Unidentified producers remain fail-closed.

## Audit verdict

- A53 live-VM comparison: **18/18 PASS**
- host project-owned compile with `-Wall -Wextra -Werror`: **PASS**
- host live-stream transport test: **PASS**
- no short PCM loop metadata introduced: **PASS**
- no fixed-time TEST-03 refill restored: **PASS**
- CA-driven transport preserved: **PASS**
- `0x1F` original-driver path preserved: **PASS**

Final verdict: **AUDIO ROM-EXACT SOFTWARE CLOSED / HARDWARE FINAL VALIDATION PENDING**.
