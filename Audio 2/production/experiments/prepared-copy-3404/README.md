# Prepared PCM copy — candidate3404

**Private native experiment preserved as source; not selected at this checkpoint.**
The selected runtime remains d9bd/cddd until the independent review and the
new native catalogue/long-music gates finish. No gameplay or graphics changes.

The unchanged d9bd/cddd64-command replay reproduces two transport warnings.
An external read-only observer leaves its entire recorded WAV unchanged and
finds overdue music events pending while the68000 copies prepared PCM after
launching SFX0x3144. An earlier native observer perturbed phase and is excluded
from that attribution. See `evidence.json` for quantified bounds and identities.

`candidate.patch` changes only `src/wf_scsp_sfx.c`: after the original first
word and guard,31 groups of8 and7 final word moves copy the remaining255
samples in ascending order. No queue, lead, deadline, buffer or clock changes.
The output source hash is
`74578e6610677070e46ca4208f1b16abfef29b99060f8a4543118abe0d4a50e5`.
The compiler produces firmware
`3404eac10581871b4afe5ee30d1c58e415430db263845228c5c82ddec7647749`.
The low firmware segment grows18 bytes to12218, below12288; high/BSS/shared
addresses and stack remain unchanged. This arithmetic is not a universal
execution deadline proof.

The native A/B delivers64/64 commands. The candidate has zero lateness, FM
loss, rejects, underruns and worker/API errors. Portable tests compare14592
cases and reject an omitted-word negative control. Native observation checks
the256-word prefix, guard, prepared cache and decoder state. Independent
binary/ABI review and long music/SFX regression remain in progress here.

To reproduce in a new private copy of the source package, apply the patch
with `patch -p1`, then use `tools/build_firmware.py --package` with that copy,
a new `--out`, `--no-timing`, and the pinned68000 GCC runtime source described
in `tools/environment/README.md`. Build the isolated harness with the same
copy and firmware, and replay the64 original TAG words; do not run or change
TAG gameplay. All source and resource identities must match the evidence.

Protected ROM, PCM, WAV, ELF, F5 and raw traces are intentionally absent here.
`HARDWARE PENDING`; owner listening and active-global OKI bank fidelity remain
unresolved independently of this copy experiment. Do not merge or call the
module production ready on the strength of one replay.
