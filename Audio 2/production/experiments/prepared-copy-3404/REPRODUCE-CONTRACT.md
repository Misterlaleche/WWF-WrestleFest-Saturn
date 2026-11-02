# Reproduce the prepared-copy memory contract

The two adjacent source files contain no original ROM or decoded sample array.
`test_copy_contract.c` reads the caller's private `SFX.DAT` at execution time;
its initialized RAM values are synthetic canaries. Generated C, object files,
executables, logs and disassembly are written only to a new external directory.
The runner disables core dumps before its deliberately failing negative test.

Use the pinned d9bd package, or its prepared-copy candidate, and the privately
built `3404eac10581871b4afe5ee30d1c58e415430db263845228c5c82ddec7647749`
firmware. The runner checks these source/firmware identities. A host C compiler
and GNU objdump supporting the 68000 ELF must be available. With the reproduced
environment loaded, run:

```sh
python3 -B experiments/prepared-copy-3404/run_copy_contract.py \
  --package /path/to/private/pinned-package \
  --sfx-data /path/to/private/resources/SFX.DAT \
  --firmware /path/to/private/candidate-firmware/SCSP.ELF \
  --out /path/to/new/private/contract-output \
  --cc cc \
  --objdump m68k-linux-gnu-objdump
```

The reference is the unchanged portable C copy. The candidate model uses the
same first word and optional guard, then 31 groups of eight ascending word
writes and seven final words. The deliberately wrong model copies only six
tail words and must fail the destination/ordered-write comparison before the
correct model is accepted.

The fixture covers 114 locally decoded prefixes, four voices, two strides and
16 volume values: **14,592 cases**. It compares **328,786,944 RAM words**, the
ordered writes, register/control state, engine and decoder state, immutable
prepared input, guard and non-target words. The runner then disassembles the
supplied ELF itself and checks its 31-by-8-plus-7 instruction sequence. Input
and host-tool hashes, every invoked command, the negative result and the final
aggregate are preserved in the private output.

The portable model does not execute the 68000 instructions. The opcode check
does not establish their timing or prove a universal deadline. Native Mednafen
captures and the independent instruction-execution review are separate
evidence. No waveform equality or physical-hardware acceptance follows from
this contract test.
