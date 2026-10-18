# ASSEMBLER HANDOFF — CLEAN-R01 Workpack 09

## Authority
- Main 68000 SHA1: `a726c46f350723ea73158bc78055d17d418b14e2`.
- Depend on Referee/Announcer FINAL for referee state machines, target/follow, count program, TAG/Royal branch and count-out.
- Depend on Animation FINAL for action `$48` descriptor/callback ownership and animation schedules.
- Do not import WIP160's submission or kickout convenience logic as authority.

## Implementable now
- Contextual action resolver: for every internal wrestler, prone categories `$05/$06/$07`, physical button2 resolves to raw `$48`.
- Raw `$48` callback must preserve the demonstrated setup and assert wrestler `+$35.bit0` (`$133A2`); referee `$20556` consumes that trigger and stores the wrestler pointer in referee `+$56`.
- Referee primary/action dispatch tables exactly as supplied in `tables/`.
- Count action `$20022`: init `$1C169A=0`, `+$54=$3162`, `+$23=20`, do **not** reset `+$25`; on phase expiry increment phase, even phases increment count/display accumulator, call `$2067C`, emit/increment audio command; terminal at phase 7.
- Cancellation gate `$1F9D8`: while `+$25<6`, loss of linked `+$35.bit0` clears count display and returns referee to state5; do not apply that gate after `+$25>=6`.
- TAG terminal/result path and Royal branch remain separate exactly as Referee FINAL specifies.

## Must remain fail-closed
- Exact human kickout inputs/mash threshold.
- CPU kickout/escape cadence or probability.
- Submission stamina drain, mash/escape thresholds, release timing, tap/result producer.
- Dedicated rope break.
- Partner/opponent interruption rule beyond the proven generic trigger-loss cancellation.
- Exact visual tile/position program for numeric 1/2/3 until `$2067C` renderer join is closed.
- `+$AA/$10D04` kickout semantics.

## Exact fields / no-clobber
- Wrestler `+$35.bit0`: preserve bit-level behavior; do not replace with a high-level boolean that is cleared/set at guessed times.
- Referee `+$20/+21`, `+$1C/+1D`, `+$56`, `+$05`, `+$23`, `+$24/+25`, `+$54` and globals `$1C169A/$1C169B` must preserve width and phase ordering.
- Do not reset referee `+$25` on entry to action `$20022`.
- Do not collapse TAG and Royal result semantics.

## Ordering/timing
- Referee primary state6 cancel gate executes before action `$20022` in the canonical machine.
- Count action uses inherited phase, so implementation must preserve transition state rather than synthesize a new 0-based pin timer.
- Even-phase display/audio events and phase7 terminal are order-sensitive.

## Required integration tests
1. ROM matrix test: all 12 internal wrestler tables, categories 5/6/7, button2 => `$48`.
2. Raw48 trigger test: callback path that reaches `$133A2` asserts `+$35.bit0` and referee target scan can acquire it.
3. Cancel-before-lock: clear trigger with phase `<6` -> display cleared/state5.
4. No false cancel-after-lock: same trigger clear at phase `>=6` must not use `$1F9D8` cancellation branch.
5. Count event sequencing: only even phases advance display accumulator and audio command.
6. TAG/Royal divergence at terminal.
7. Fail-closed tests: no synthetic submission victory, kickout, rope break or partner-interruption behavior executes unless backed by a later FINAL authority.

## Runtime-dependent / not provable
See `UNRESOLVED.md`.
