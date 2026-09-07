# WIP75 — expand exact named finisher victim subtype producers

- Extended the width-aware state4 victim shadow to five finisher impacts whose ROM producer event is pinned:
  - Hogan raw `$35` / legdrop family -> `$17858`, word `$001A`.
  - Jake DDT -> `$17542`, word `$0023` (Test190 evidence).
  - Boss Man Slam -> `$17100`, word `$002A`.
  - Crush raw `$44` -> `$186E6`, word `$0005`.
  - Perfect Plex -> `$15154`, word `$0021`.
- Branch-dependent/unresolved finisher producers remain deliberately unwired (Warrior, Smash raw43, Slaughter, Earthquake, DiBiase).
- Tests359/360 PASS; `game.c` syntax PASS.
- Shadow still does not control prone selection or contact timing until exact state4 transitions are wired.
- Source ZIP SHA256: `b179de58841a08cbc9c72312f954a4a57c430db913b0ab5c5b9651f6ec70f7e1`.
- No held release, no `tick==12` change, no BIN/CUE, no ROM files.
