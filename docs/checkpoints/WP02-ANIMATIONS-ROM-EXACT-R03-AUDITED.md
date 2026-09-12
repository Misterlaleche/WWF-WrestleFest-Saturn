# WP02-ANIMATIONS-ROM-EXACT-R03-AUDITED

Checkpoint status: **ANIMATIONS CLOSED for WP02-owned scope**.

Artifact SHA-256: `0c96d4049d2b08a9c799d42457cea9da470b052143102c080a93b147ccb72329`

ROM program SHA-256: `265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306`

Recovered gfx2 SHA-256: `a3b5e1fedd6c41cd398ba0cb437cddb2f50b3ae0a0dc178a10b70a7ed5d010ba`

Audit coverage: 216 ROM-verified descriptor/subdescriptor records, 629 frame entries, 73 indefinite-hold descriptors / 78 FF00 entries, 18 cyclic kind-2 descriptors, 399 unique non-sentinel base poses, 399/399 metasprite pose coverage, 10,204 valid character/partner compositions, 425,714 exact piece records.

Corrections relative to R02 include the kind-2 finite-duration bug, missing Hawk/Animal resident pose mappings, inclusion of internal banks 10/11, and inclusion of 36 auxiliary/subdescriptor records omitted from the old 180-descriptor manifest.

HOST strict compile: PASS. SH-2 strict compile: PASS. Direct ROM audit: PASS. Package clean-extraction checksum audit: PASS. Hardware: NOT TESTED.

External Gameplay/TAG/Rumble/Renderer/GFX blockers remain explicitly separate and are not reclassified as WP02 animation behavior.