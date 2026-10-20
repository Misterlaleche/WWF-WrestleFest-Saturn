# Runtime18 disc build

Run `DOCKER=/path/to/docker validation/build-runtime18.sh /path/to/sh2eb-elf`.
The script uses the pinned Yaul Docker image, the supplied SDK read-only, and host
Python 3. Outputs are under `validation/results/runtime18/`. The build emits a
single MODE1/2352 track with no CD audio. Place runtime CDFS files under
`src/runtime/audit/runtime18-cd/` before building.

`Makefile.saturn-disc` uses the SDK's `make-ip` and `make-iso`, the boot path
recorded in WP33 and WIP160: master stack and initial load address 0x06004000,
slave stack 0x06001E00. The encoder is copied from the read-only WP33 FINAL
package's `tools/iso_to_bin_exact.py`; the verifier comes from that package,
adapted only for Runtime18 paths plus an initial-load-address check.

Validation compares every raw sector including sync, address, EDC and ECC,
checks the Saturn/IP and ISO9660 signatures, and compares every CDFS payload
against its source, including A.BIN against the linked binary. This is a disc
structure check, not evidence of gameplay or hardware execution.

The reference archive is `sources/zips finales.zip`, nested
`zips finales/WF-CLEAN-R01-33-SATURN-BOOT-CDFS-BUILD-DISC-LAYOUT-FINAL.zip`.
