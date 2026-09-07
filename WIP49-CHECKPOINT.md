# Test255 WIP49 — secondary +$42 probe-site separation

## Scope
Prevent later/preparatory callback probe writes from being mis-associated with an action's first `$FF00` held gate while the inherited probe lifecycle is still being recovered.

## ROM-EXACT / evidence-preserving code
- Added `arcade_prone_secondary_probe_profile(raw,...)` for the three already byte-pinned later/preparatory `+$42=+$20` paths:
  - raw `$23`: write `$15EA8`, clear `$15EBE`, initial held gate `$15DFE`;
  - raw `$46`: write `$146E6`, clear `$14706`, initial held gate `$144F0`;
  - raw `$47`: write `$146E6`, clear `$14706`, initial held gate `$144F0`.
- These write addresses are later than the first held gate and are explicitly represented as secondary/preparatory probe sites.
- They MUST NOT be reused as the probe source for the initial `$FF00` release of `$23/$46/$47`.
- This preserves the WIP39 boundary: their initial held gates still depend on inherited `+$42` state unless another exact pre-gate producer is demonstrated.

## Tests
- Test333 PASS: exact later/preparatory probe sites and clear addresses are represented separately from the initial held gate.
- Regression PASS: Tests319,320,322-333 plus static Test321.
- No ROM-dependent test was rerun because the checkpoint source contains no `program.bin`.

## Runtime status
- No gameplay timing changed.
- Live `$FF00` `release=true` remains **REGISTERED BUT NOT IMPLEMENTED**.
- Legacy 32-tick held fallback remains **PROVISIONAL**.
- Legacy ground `tick==12` damage remains **PROVISIONAL** and must not be removed before physical contact timing is safe.

## Remaining inherited-input closure
1. Find exact pre-gate `+$42` producers/lifetime for raw `$0B`, raw `$0E` phases 0/1, raw `$13`, raw `$23`, raw `$46/$47`.
2. Find exact inherited raw `+$0E` transitions for raws without target-Z rebase.
3. Keep secondary `$23/$46/$47` +$20 probes isolated from their initial holds.
4. Only then connect resolver `contact37` to live callback release and remove the 32-tick fallback.

## Fidelity
- **ROM-EXACT:** secondary probe write/clear addresses and their separation from the first held gate.
- **REGISTERED BUT NOT IMPLEMENTED:** live release bridge and secondary-probe runtime application.
- **PROVISIONAL:** 32-tick held fallback and `tick==12` damage scheduling.

Source checkpoint:
- Library: `/WWF-Saturn/WWF-Saturn-Test255-WIP49-source.zip`
- SHA256: `166bdb1489692cfc2eda6a9d2a60df9127e07f7935b2df305fa6ce2c7231a50a`

No ROM image included. No BIN/CUE generated.
