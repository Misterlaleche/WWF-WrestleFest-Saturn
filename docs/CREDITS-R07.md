# Ending Credits R07

Authority: `WWF-WrestleFest-Saturn-Credits-ROM-EXACT-FINAL-package(1).zip`.

Implemented as a renderer/audio-backend-neutral controller:
- TAG and Royal converge on the same Credits controller contract;
- 24-VBlank entry fade gate;
- entry audio command `0x11` at setup;
- 9 fixed pages with exact text IDs, poses, anchors and text coordinates;
- separate setup synchronization before each 129-VBlank timed hold;
- six staff blocks with exact anchors/type/resource metadata;
- 1281 staff updates at `+0x0000C000` 16.16 per update;
- four-player active-low skip mask `0x0030`;
- exit command `0x1F` at common EXIT path;
- 24-VBlank exit fade;
- cleanup/front-end return token `0x000006FC`.

The controller emits events only. It does not call SCSP, render graphics, load protected assets, or replace the Scene Flow Master.

Remaining integration gates:
- connect fixed-page/background/staff events to WP25/WP26 renderer assets;
- route command `0x11/0x1F` through WP24 at the emitted points;
- bind TAG/Royal entry predicates to their actual runtime owners;
- map return token through the presentation/frontend owner;
- full Saturn ELF/source integration and real-hardware validation.

No BIN/CUE generated. Hardware PASS not claimed.
