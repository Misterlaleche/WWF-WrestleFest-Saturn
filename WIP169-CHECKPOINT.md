# WIP169 CHECKPOINT — additive integration overlay from audited parallel handoffs

## Scope
First implementation checkpoint that consumes the audited parallel research/decompilation packages without regressing the newer WIP160+ gameplay/Ground lineage. No BIN/CUE generated.

## Integration strategy
The current branch stores Test255 source as an older WIP9 archive, while Ground/held research has advanced through WIP168. The WIP160+ source archive is not materialized in the current runtime. Therefore WIP169 is an **additive overlay**: it contains modules and contracts that can be applied to the latest canonical source without replacing `game.c`, `arcade_ai.c`, `arcade_animation.c`, Ground Actions, grapple, collision, scheduler, or victim-state4 code.

## Implemented modules
### Records / Rankings
- `wf_records.c/.h`
- exact TAG and Royal default tables;
- descending insert/sort with new equal entry before old equal;
- qualification/clamp/display rules from FINAL Records handoff;
- exact initials alphabet/wrap;
- WFRK v1 serializer/deserializer, 148-byte logical payload and CRC32.

### Saturn internal Backup RAM
- `wf_records_bup.c/.h`
- internal BUP device backend using libyaul `bup_init/bup_dir/bup_read/bup_status/bup_write/bup_verify`;
- filename `WWFRECORDS`, comment `WF RECORDS`, English;
- temporary `bup_t` allocation avoids a permanent 24,576-byte BSS reservation;
- missing/corrupt/no-space/I/O paths remain non-fatal to gameplay.

### Credits
- `wf_credits.c/.h`
- ENTRY_FADE -> SETUP -> 9 FIXED_PAGES -> STAFF_SCROLL -> EXIT_FADE -> FRONTEND_RETURN;
- 24-VBlank entry/exit fades;
- page setup acknowledgement separated from the 129-VBlank page dwell;
- six staff blocks and 1281-update scroll contract;
- skip mask and audio-entry/exit events represented without inventing rendering/audio producers.

### Attract biographies
- audited `attract_bio.c/.h` imported additively;
- no ROM artwork included; BIO packs remain generated from user-owned ROM outside this source package.

### Audio
- recovered SFX/PCM command/backend modules imported from the integration handoff;
- A44 safe Saturn music transport runtime imported;
- A49 music metadata is authoritative over lifetime/loop semantics;
- new `wf_audio_exact_adapter.c/.h` maps A49 FINITE->A44 FINITE and A49 CONTINUOUS->A44 LONG;
- exact adapter always leaves PCM loop metadata zero and rejects any A49 asset that claims PCM loop fields.

### Referee / announcer
- `wf_ref_announcer.c/.h` materializes the audited numeric state/callback tables;
- announcer exit 15->16->17->18 choreography is represented with exact update counts, selector groups/dwell and state15 +0.5 Y/update movement;
- actor-dependent announcer states remain explicitly actor-dependent; no fixed timeout is invented.

### Presentation / animations / victim state4 / AI boundaries
- Presentation FINAL headers imported under `presentation_contracts/` as compile-time authority without replacing existing frontend code.
- Animation FINAL CSV/JSON imported under `animation_contracts/` as numeric reconciliation authority; no blind replacement of newer WIP160 animation/Ground code.
- victim +$65 FINAL report included as authority: byte-vs-word writes, FIRST->SECOND, scheduler/current-turn contact and stale subtype semantics must be preserved.
- AI canonical audited FINAL remains the authority for future reconciliation; it supersedes earlier AI overclaims. No old AI donor source replaces current WIP gameplay.

## Installer
`tools/apply_overlay.py` is idempotent and additive. It copies the modules/contracts/assets and adds new C modules to `SH_SRCS` without replacing canonical gameplay files.

## Validation
Host suite compiled with `-std=c11 -Wall -Wextra -Werror`:
- Records/WFRK PASS
- Credits state/timing PASS
- attract_bio compile PASS
- A44 router/runtime PASS
- A49 contract PASS
- A49->A44 exact adapter PASS
- referee/announcer contract PASS
- Presentation header contract compile PASS
- overlay installer applied twice without duplicate `SH_SRCS` entries PASS

## Package
`WWF-Saturn-Test255-WIP169-integration-overlay.zip`

SHA256:
`cea00879277ef3942afc5bfd9373abd4c50112dcf1ae5fa450b53c60caaba22c`

Archive validation: PASS. No `wwfwfest.zip`, `program.bin`, arcade ROM chips, BIN/CUE or ISO included.

## Safety / authority
- WIP168 raw23 held-release contract is not globally enabled here.
- `tick==12` unchanged.
- 32-tick held fallback unchanged.
- No PROVISIONAL or REGISTERED item is silently promoted to ROM-EXACT.
- No donor gameplay source replaces WIP160+ logic.

## Next
Materialize the latest WIP160+ canonical source, apply WIP169 overlay, then wire additive frontend services in this order: Records load/defaults at boot and save on new ranking; Credits terminal transitions; attract state adapter; announcer/referee actor bridge; audio exact music adapter. Reconcile AI/animation numerically against their FINAL contracts instead of replacing current runtime wholesale.
