# Test255 / V0.270 WIP — ROM-exact attract tables

Not canonical yet. Test254/V0.269 remains the validated baseline.

## ROM-EXACT recovered and executable
- 14 state durations at program ROM `$119A`.
- Exact four-wrestler roster for every state at `$0F3E`.
- Featured wrestler/portrait table at `$8E9A`: `FFFF, FFFF, Hogan, Warrior, Jake, Boss Man, Slaughter, Perfect, Earthquake, DiBiase, Hawk, Animal, Smash, Crush`.
- Overlay routine around `$8CF8`: TAG text ID `$29`, gfx block `$15`, palette bank 7; Royal text ID `$2A`, gfx block `$16`, palette bank 6.

The recovered state rosters are:
0 Hogan/Warrior/Smash/Crush; 1 Boss Man/Slaughter/Perfect/Earthquake; 2 Hogan/Warrior/Slaughter/Perfect; 3 Warrior/Hogan/DiBiase/Earthquake; 4 Jake/Boss Man/Hawk/Animal; 5 Boss Man/Jake/Slaughter/DiBiase; 6 Slaughter/Perfect/Hogan/Warrior; 7 Perfect/DiBiase/Jake/Hogan; 8 Earthquake/Slaughter/Boss Man/Jake; 9 DiBiase/Earthquake/Warrior/Boss Man; 10 Hawk/Animal/Jake/Warrior; 11 Animal/Hawk/Jake/Warrior; 12 Smash/Crush/Hogan/Warrior; 13 Crush/Smash/Hogan/Warrior.

## Still pending
- exact Demolition A/B speaker animation/audio mapping;
- exact TAG-records/Royal-records sub-sequence timing and presentation;
- Hawk/Animal Saturn packs needed before states containing IDs 10/11 can be made live safely;
- final replacement of the old fixed 1800-tick Saturn demo path.

No BIN/CUE generated.
