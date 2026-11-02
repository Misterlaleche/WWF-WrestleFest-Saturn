# Active OKI ROM bank changes

**Fidelity status: BLOCKED.** The original OKI bank is global, and the observed
64-command replay contains
a case where changing it alters an already playing voice. The current native
asset bank selected at voice start does not reproduce that case. This remains
a fidelity blocker; no production banking policy is changed by this audit.

## Primary source boundary

The pinned MAME `ddragon3.cpp` WWF configuration sends the bank bit to
`set_rom_bank(data & 1)`. `okim6295.h` selects
`device_rom_interface<18>`. In `dirom.ipp`, the installed read bank spans
address0 through the18-bit mask and advances its backing pointer by one whole
window. Thus the model banks the entire `00000–3FFFF` address window, not only
the top64 KiB. `okim6295.cpp` fetches each active voice's next encoded byte
through that current ROM interface; it does not save a private bank at key-on.

All of these files are pinned to MAME commit
`5c60f61d321dd7869f6cc2e7a6f78191a16b4d84`. The verified Git blob IDs and complete
SHA-256 identities are in `tools/reference/clock-source-manifest.json` and
`tools/reference/bank-source-manifest.json`. These are primary source code
observations, not a claim that an analog PCB recording was obtained.

## Causal replay measurement

The full original Z80/Nuked/OKI reference replays the64 measured native API
admissions over138 seconds. Five bank changes occur while PCM is active.
That count alone does not prove an audible difference, so a separate host
diagnostic runs a second decoder history for each voice. The only difference
is that this counterfactual retains the bank selected at the start of the
voice. Actual and counterfactual histories use the same original command
stream, addresses, nibble clock, reset value, levels and mix gains.

The diagnostic's actual mixed PCM and original trace are byte-identical to
the full-board reference. The following differences are therefore isolated
to bank selection, rather than a new command sequence or instrument mapping:

| Physical OKI voice | Nibble reads after its original bank changed | Reads whose byte differs | Nibbles that differ | Decoded samples that differ |
|---|---:|---:|---:|---:|
|0 |3,561 |0 |0 |0 |
|1 |2,667 |2,641 |2,406 |2,663 |
|2 |0 |0 |0 |0 |
|3 |0 |0 |0 |0 |

The byte-read count counts fetches, not unique addresses: the same byte may
be fetched for both of its nibbles. Voice0's changed-bank reads are harmless
aliases in this replay. Voice1's first differing relative ROM address is
`3B1B2`. Its altered decoder history changes14,680 of the44.1 kHz mixed frames,
with a maximum difference of5,420 raw PCM units. Neither output is normalized.
The difference spans output frames 2,451,772 through 2,466,473, inclusive:
55.595737 through 55.929093 seconds from the declared reference origin. This
does not mean every frame inside that interval differs; the exact count is
14,680. The counterfactual is not presented as a Saturn capture or as arcade audio.

## Native implication and remaining work

The native engine keeps the selected bank/sample in each PCM voice and fills
its ring from the packed asset. A global bank write currently selects future
starts; it does not redirect the encoded-byte address of every active voice
or repair already prepared samples. Exact behavior therefore needs an
original-address mapping and an explicit policy for active decode state and
prepared/ring data at the bank boundary. Simply changing the stored bank field
would not repair the PCM already buffered ahead of playback.

This audit establishes the defect's existence in the actual replay and the
source of the difference. The storage audit below establishes a small feasible
address-map extension; it does not establish a timing-safe runtime fix. The
known common-bank aliases explain voice0's result but do not justify assuming
all active voices are aliases.

## Original address coverage: no 512 KiB shadow is needed for these ranges

`audit_oki_storage.py` checks all 256 original sample-table entries, including
the 18-bit address mask and strict `start < end` rule. Exactly 73 entries in
bank 0 and 41 in bank 1 are valid. Their complete bytes agree with the current
SFX.DAT assets. The union of their relative address ranges is 258,725 bytes.
An active voice may read each of those addresses through either global bank.

| Quantity | Bank 0 | Bank 1 |
| --- | ---: | ---: |
| Original addresses already represented by its own sample assets | 258,388 | 258,725 |
| Addresses required under arbitrary bank selection inside the valid ranges | 258,725 | 258,725 |
| Missing original bytes | 337 | 0 |

The missing bytes form one bank-0 range, `0x3F554..0x3F6A4` inclusive. The first
different address in the observed replay, `0x3B1B2`, is already represented in
both banks. Thus that measured failure is a routing/state failure, not missing
sample content. All **517,450 required physical address values** were verified
byte for byte using the existing pack plus the private missing-byte payload.

A feasibility extension can retain the current 114 asset entries and add:

| Storage item | Bytes |
| --- | ---: |
| 114 original start addresses, three bytes each | 342 |
| One additional range descriptor | 8 |
| Missing original bytes | 337 |
| Audit-format header | 16 |
| Alignment padding | 1 |
| **Total** | **704** |

The current 440,710-byte pack has 1,658 bytes free within its existing
442,368-byte window. This extension would leave 954 bytes in that same window.
The map, payload and full table are generated privately. The source package
contains only the generator, hashes and aggregate verification metadata.
This is a storage construction with complete byte verification, **not an
installed pack ABI or native resolver**. It does not account for new decoder
code, per-voice mapping caches or rollback control.

## Full-ROM independent block compression

The same audit compresses the complete 524,288-byte original ROM in independent
256-, 512- and 1,024-byte blocks. Every block and the reconstructed complete
address space pass exact equality checks. Totals include a 16-byte header,
32-bit offset/raw-fallback tables, and, where applicable, a 16-bit logical-page
dictionary. Compressed data larger than its original block is stored raw.
Measurements use host LZ4 1.9.4 and zlib 1.3.2; zlib level 9 is a conceptual
storage comparison, not a proposed or timed 68000 decompressor.

| Block bytes | LZ4 independent | LZ4 deduplicated pages | zlib independent | zlib deduplicated pages |
| ---: | ---: | ---: | ---: | ---: |
| 256 | 525,131 | 449,454 | 508,732 | 435,044 |
| 512 | 520,343 | 443,575 | 500,464 | 426,100 |
| 1,024 | 518,192 | 441,255 | 484,947 | 412,036 |

Raw-deflate variants are also measured in the JSON report; their deduplicated
totals are 432,789, 423,530 and 409,420 bytes respectively. No case is a native
performance result. The LZ4 1,024-byte deduplicated total leaves only 1,113 bytes
in the current pack window **before** command/event metadata, decode state,
checkpoints or caches. One decompressed block per voice would require another
4,096 bytes. The address-map construction above is smaller for the actual
valid sample ranges and avoids adding a second decompressor to the analysis.

## Checkpoint storage and the unproved timing budget

The existing 68000 ADPCM state is 14 bytes, but the irreducible history at an
implicit sample position is a signed signal and step index. A proposed aligned
four-byte checkpoint could hold those plus a tag. For four 4,096-sample rings,
including one boundary checkpoint per voice:

| PCM spacing | Checkpoints, four voices | Four-byte state total | Re-decode before boundary, per voice |
| ---: | ---: | ---: | ---: |
| 32 | 516 | 2,064 B | At most 31 samples |
| 64 | 260 | 1,040 B | At most 63 samples |
| 128 | 132 | 528 B | At most 127 samples |
| 256 | 68 | 272 B | At most 255 samples |
| 512 | 36 | 144 B | At most 511 samples |

For example, 528 bytes of checkpoint records would fit after the 704-byte map
extension, leaving 426 bytes of the current pack window. That arithmetic
excludes per-voice bookkeeping, bank-event history, decoder code and a proven
read/write handoff. A checkpoint every 128 samples cannot blindly re-decode
across earlier bank changes: it needs the intervening bank history or an
additional exact checkpoint at each committed bank boundary. Saturating ADPCM
state cannot generally be recovered by reversing already decoded PCM.

Most significantly, restoring the state at the boundary does not fix PCM
already written ahead of that boundary. The ring holds up to **4,096 samples
at 8,000 Hz = 512 ms**. Music transport lead is **4,096 SCSP frames at 44,100 Hz
= 92.879819 ms**, equivalent to about 743.04 original PCM sample intervals.
These are different units. The current lookahead can decode farther than the
known music-event horizon. Regenerating as many as 4,096 samples per active
voice is a substantial burst; no 68000 deadline proof exists for it.

## Actual bank routes and the next bounded experiment

Music bank writes originate in `wf_music_seq.c` at original address `E800`.
`wf_music_module.c` publishes them as kind-2 timestamped events, using the
timeline initialized at current DSP clock plus 4,096 SCSP frames. At the due
time, `wf_music_firmware.c` changes only `music_bank`; the preview can inspect
future bank metadata for an upcoming prepared music sample.

Game SFX commands take a different path. `wf_audio2_command()` posts the raw
word to the mailbox; firmware executes `wf_scsp_sfx_command()` when servicing
that request. It changes `s->bank` immediately and copies that bank into the
new logical sequence. It is not a bank event scheduled 4,096 frames in advance.
Existing voices retain their original packed asset. The two variables do not
constitute one original global bank, and the immediate path cannot assume a
future event was available before refill.

The next bounded step is a host resolver/state-replay prototype using the
generated map. It should pass every valid original address, bank changes at
both nibble parities and every sample/ring boundary, repeated changes inside a
checkpoint span, and the proved 64-command mixed trace. Only then should an
isolated native implementation measure maximum repair work and playback
deadlines. It must specify how both bank routes affect every active voice and
invalidate prepared PCM. No shared runtime code was changed in this audit;
the complete native bank fix remains open.

## Reproduce privately

```sh
python3 tools/reference/audit_active_banks.py \
  --model-run /private-output/full-board-mix \
  --out /private-output/active-bank-audit
python3 tools/reference/audit_oki_storage.py \
  --rom-zip /private-input/wwfwfest.zip \
  --pack /private-output/full-board-mix/inputs/SFX.DAT \
  --out /private-output/oki-storage-audit
```

The input is a complete `reproduce_mix_reference.py` result. The audit creates
its own instrumented host sources/output, verifies original input hashes,
retains the production reference sources, and requires exact preservation of
the actual PCM and trace. Metadata is recorded separately from ROM/audio.
