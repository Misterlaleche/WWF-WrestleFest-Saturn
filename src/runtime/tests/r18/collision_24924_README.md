# Collision second2 /24924 facing mismatch

Natural nativeF8C TAG→HOST basic frame970 reaches a pair with source slot5/ID3 retained profile7 and target slot4/ID9 profile10. The production phase reports24408 because its diagnostic prefers the first callback; test-only instrumentation identifies first1/24408 ACCEPT followed by second2/24924 BLOCKED. The phase restores the failed pair before returning, so blocked actor/global/D dumps provide the literal pre-pair context. Source profileWORD is already0 after a previous accepted target; retained row7 must remain the dispatcher input.

The ROM24930 branch for unequal facing BYTEs goes to2497C, writes stateWORD8005 and returns. It does not consume the linked pointer or alter subtype. The production change admits exactly that continuation. The complete2435A pair-dispatch matrix also exposed an earlier first1/24408 ordering error: profile-high.bit7 skips both24D7A and24DDC. Native pending+1 is now inside that conditional, matching the ROM.

RED natural: `validation/local-20261024/collision-24924-red.log` (native BLOCKED versus ROM ACCEPT). Second RED: `collision-24408-profile80-red.log` (pending damageC2/ROM1). GREEN: `collision-24924-green.json/log`,4167 full pair calls, intact2435A, complete first/second callbacks and helpers, no intercepts. RAM8KiB/FG16KiB/D0-D7/RNG and dispatch carry/result match.2435A performs its original MOVEM save/restore, so D0-D7 are compared at its real return.

The matrix includes natural970; all unequal BYTEfacing combinations from00/01/80/FF; first1 acceptance/rejection, profile-high40/80, flags33 and timer52 boundaries; raw subtypes/states and unused invalid/self/tagged links through no-op first0; and retained row7/25 with current sourceprofile0. Equal-facing second2 is unchanged and outside this proof.

`collision_24924_capture_host.py` reproduces the read-only fixture projection. `collision_24924_verify_rom.py --report ...` runs the oracle after loading the host environment. The diagnostic wrapper and reproduction are `collision970_diagnostic.c` and `validation/local-20261024/run-collision970-diagnostic.py`; neither changes production.

Requested6000-frame probes import the exact nativeF8C capture without reseeding. TAG advances970→1524/raw12. Royal remains600, and instrumentation identifies a different pair: source4/ID0 row7→target6/ID4 row12, second3/24984. These are retained boundaries, not6000PASS. Global phase24062 register/timing behavior, equal-facing branches, physical hardware and identical HOST/SH2 trajectories are not claimed.
