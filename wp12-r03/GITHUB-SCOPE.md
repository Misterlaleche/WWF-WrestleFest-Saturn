# WP12 R03 Git checkpoint scope

This branch is based directly on accepted R02 HEAD:
`99d316037af7508a896f0535d44dfa18775d0c47`.

It carries the R03 first-match presentation implementation core, exact audited C
source/header, host/SH-2 runners, host regression test, animated-preview generator,
and expected golden/preview hashes.

The complete audited evidence bundle (including ROM audit tools, WP25 graphics
helper, generated APNG/CSV/PNG golden files, logs, docs and SHA256SUMS) is the
delivered artifact:

`WP12-ENTRANCES-PREMATCH-R03-AUDITED.zip`
SHA-256 `f9a7d2155d2d06f0b0d690545a9cbfbb02f9184395bcf1140db3bfd5a91a73d8`.

Generated graphics are deliberately not promoted as hand-edited repository
assets. The ZIP contains the byte-identical regeneration/audit toolchain that
was replayed from a fresh extraction.

No `main.c`, gameplay core, WP28, CPU/AI, collision, physics, audio backend or
BIN/CUE is introduced by this branch.
