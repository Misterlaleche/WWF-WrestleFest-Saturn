# WP12 R02 — dependency report

| Dependency | Owner / authority | R02 treatment | Status |
|---|---|---|---|
| `$C98` common actor/video setup | master/common setup + ROM | exact external call; post-call WP12 fields validated | REQUIRED |
| `$98BA` TAG presentation composition | presentation/graphics | exact TAG-only call operation | PROVEN, visual semantics partly OPEN |
| `$21B4` shared RNG | shared RNG owner | caller supplies low nibble when AAEA requests it | REQUIRED / fail-closed |
| `$2052` raw audio dispatch | WP24 audio | R02 emits only proven raw commands | REQUIRED, no audio backend here |
| `$AE20 D0=0` optional interstitial | Presentation FINAL | exact external call for `$1C0163==4/9` | PROVEN |
| `$264E2`, `$7B70`, `$2A06`, `$8AE6` | common presentation/master | exact ordered external operations | PROVEN |
| presenter set `$1F` composition | WP25 | numeric selector/composition join consumed by renderer | CLOSED numeric / human names OPEN |
| machine->raster/camera transform | WP26 + runtime camera producer | consume unchanged externally | RUNTIME-DEPENDENT |
| wrestler names/red bands/WWF logo literal join | `$98BA` + graphics evidence | no substitute drawn by WP12 | OPEN |
| gameplay after `$F8C` | GAMEPLAY/WP28 etc. | only handoff boundary emitted | BLOCKED_BY_GAMEPLAY_OWNER |
| referee gameplay/pin/rules | gameplay/referee owner | explicitly excluded | OUT OF SCOPE |
| Royal in-match entrants/targets/elimination | WP07/gameplay | explicitly excluded | OUT OF SCOPE |

Raw WP12 audio producers are enumerated in `manifests/audio-hooks.csv`. Human semantics are not invented; integration remains owned by WP24.
