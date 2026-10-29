# Independent native-audio integration review

Read-only review of `source/wp01_audio.c`, `source/wp01_audio_queue.c` and
the audio-loading/restart sections of `source/main.c`.

## Corrections identified

### 1. Global SCSP key-execute latch

The first driver guarded only writes carrying KYONEX. A following FM key
group could change another slot's KYONB while the previous global KYONEX
was still pending. Delaying only the next KYONEX did not protect the
previous group of key bits. This is particularly relevant when a delayed
IRQ has multiple events already due.

The reviewed revised driver now waits before **any slot-control write**
(`offset % 32 == 0`), and updates the guard timestamp only for KYONEX.
That addresses the identified grouping race. The guard of 39 FRT ticks
exceeds two 44,100 Hz SCSP sample periods at the corrected CPU clock.

### 2. FRT clock constant

The initial driver/program header used `295312500/352` FRT ticks/second.
This is about0.102459% below the verified Mednafen320-mode clock.

The primary emulator source in `clock_reference/ss.cpp:1405` sets NTSC
master clock to1746818181.8181818Hz; `clock_reference/smpc.cpp:63–64`
uses divisor65 for320-mode and61 for352-mode. Thus:

- CPU320: `3843000000/143 Hz` =26,874,125.874125876Hz.
- FRT/32: `120093750/143 Hz` =839,816.4335664336Hz.
- FRT ticks per NTSC320 frame:14,037.625.

Use the corrected ratio in register-program timestamp compilation and
header verification. The existing video cadence based on1820/263 is not
the faulty value and does not require the same change.

The old ratio advances events about7.27ms early over7.1seconds relative
to the verified reference. This is a timing-fidelity issue, not a cause
of gross slowdown. Reference URLs and content hashes are stored in
`clock_reference/provenance.json`.

### 3. SDK FRT dispatch / HLE BIOS

The supplied libyaul dispatcher calls the handler at index0 for an inactive
compare channel. That index points at the BIOS no-op entry. The root's
observed HLE crash follows this dependency. The revised local compare-A
handler acknowledges only OCFA and directly invokes the audio handler;
it avoids the inactive-compare BIOS call without editing the SDK.

## Checks with no identified fault

- SOUNDRAM is first read into LOWRAM and fully copied to sound RAM.
  AUDIOREG then reuses LOWRAM and remains resident there; the two payloads
  do not destructively alias during playback.
- `cpu_frt_oca_set` does not reset the free-running timer. The only
  `cpu_frt_init` calls occur before playback, preserving the graphics
  telemetry counter during runtime.
- The heartbeat is0x4000ticks (about19.5ms), comfortably below one
  16-bit counter wrap (about78ms), provided interrupts continue to execute.
- `wp01_audio_stop` masks interrupts while disabling compareA, muting all
  outputs and clearing KYONB. Queue preparation occurs while playback and
  compareA are disabled; playback begins from the VBlank callback.
- Event header lengths are bounded before pointer traversal; writes are
  restricted to even slot registers below0x400 and offsets0..0x16.
- Initial actual program contained488 initial register writes and2224
  scheduled writes, with a maximum14 records at one timestamp. The timing
  tests and integrated emulator run must still measure actual IRQ costs.

## Metric qualification

`queue.max_lateness` uses the timestamp captured on entry to
`queue_advance`. It is a dispatch-lateness metric: waits inside individual
register writes can add delay that this value does not measure. It should
not be presented as a bound on exact hardware-register-write timing unless
per-write timestamps are also recorded.

This review does not certify audible FM fidelity, hardware performance,
or a completed physical Saturn test.
