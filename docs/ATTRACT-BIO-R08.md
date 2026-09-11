# Attract Biography R08

Authority: `WWF-WrestleFest-Saturn-Attract-Bio-ROM-EXACT-FINAL-AUDITED(1).zip`.

Imported pure presentation controller only; the legacy patch/main.c adapter is deliberately not imported.

ROM-EXACT controller contract:
- active attract states `0x02..0x0D`;
- exact 14-state -> 12-fighter mapping;
- three portrait frames;
- initial dwell 8;
- subsequent dwell `(RNG & 0x0F)+8` only on dwell expiry;
- tick 0..255 toggles frame 0/1;
- tick 256+ forces frame 2;
- tick 512 preserves one portrait-only tail while text is removed;
- next update removes overlay;
- exact pack schema: 37,696 bytes per `BIOxx.PAK`;
- no direct audio command;
- no local START handler;
- no gameplay mutation.

Local revalidation against the user's authoritative ROM:
- deterministic generation of all 12 `BIOxx.PAK`: PASS;
- upstream controller/pack test: PASS;
- direct ROM/assets audit: PASS.

The generated protected `BIOxx.PAK` payloads are intentionally excluded. Only their reproducible metadata/hashes are retained in the manifest.

Critical fail-closed integration rule:
- do NOT import the upstream isolated visual RNG bridge;
- the exact shared RNG routine `$21B4` must be supplied by the canonical gameplay/runtime service;
- consume its low nibble only when the dwell expires, preserving call ordering;
- until that service is available, the master integration is `BLOCKED_BY_RUNTIME_OWNER` for exact random sequence, while all non-RNG bio lifecycle behavior is integration-ready.

Asset load policy:
- preload exactly one required `BIOxx.PAK` for the active demo state;
- filename fits WP33 CDFS root-name policy;
- 37,696-byte pack is compatible with HWRAM staging;
- do not keep all 12 packs resident simultaneously without a proven need.

No BIN/CUE generated. Hardware PASS not claimed for the integrated master path.
