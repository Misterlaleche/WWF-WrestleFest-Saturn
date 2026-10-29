# WP01 R02 FRT counter and NTSC 320 clock review

This review is read-only with respect to the project firmware and SDK. All new research files are in this directory.

## Findings

The production counter should use byte accesses, with complete high/low pairs. The SH7604 Hardware Manual, table 11.2 note 3 (printed page 297, PDF page index 312), specifies byte access for every FRT register. A word load from `0xFFFFFE12` is therefore not the documented interface.

In actual SH7604 hardware, reading FRC high also latches the low byte into the shared TEMP register. The subsequent FRC low access reads TEMP. Section 11.3 (printed page 304, PDF index 319) explains this and requires upper-byte then lower-byte accesses. A low-byte carry between these accesses does not by itself tear a hardware read. An interrupting access to the shared TEMP mechanism can interfere, so callers must ensure no such preempting FRT/ICR read or FRT/OCR write.

The public Yabause source examined here omits that latch. Its `OnchipReadByte` cases `0x012` and `0x013` return the current `FRC.part.H` and `FRC.part.L` separately. A carry between those accesses can construct an incorrectly low value and make an unsigned interval appear almost 65536 ticks long. `OnchipReadWord` has no `0x012` case and returns zero for unhandled addresses; a word load would also fail in this core.

## Recommended bounded transaction

For the current single timer owner, use:

```c
static uint16_t counter(void)
{
    volatile cpu_ioregs_t *const io =
        (volatile cpu_ioregs_t *)CPU_IOREG_BASE;
    uint8_t high_before, low, high_after;
    do {
        high_before = io->frch;
        low = io->frcl;
        high_after = io->frch;
        (void)io->frcl; /* Complete the second documented high/low pair. */
    } while (high_before != high_after);
    return (uint16_t)(((uint16_t)high_before << 8) | low);
}
```

This works with the physical hardware latch and rejects a carry between the first high and low reads in Yabause. The second low byte is deliberately discarded: returning it would introduce a new carry window after the checked high byte. Completing the pair also avoids leaving an unfinished documented TEMP transaction.

The IRQ9 timer cannot preempt itself. The parent integration verified that its higher-priority scene-start path stops playback before reinitializing the audio timer. Under that ownership rule, extra interrupt masking in each getter is unnecessary. If that rule changes, protect the short register transaction by saving/restoring the previous CPU interrupt mask, or otherwise exclude preempting TEMP accesses. The helper does not make arbitrary concurrent users of the shared FRT safe.

The parent integration reports that changing the original two-byte getter to high/low/high retry reduced maximum handler duration from the impossible 65291 ticks to 96 ticks, with the scene and music completing and no missed VBlank. This is consistent with the missing latch observed in the Yabause source. That integrated run was performed by the parent agent; it was not rerun by this research task.

## NTSC 320 clock

The Sega SMPC User's Manual ST-169-R1-072694, table 1.1 (printed page 3, PDF index 12), gives the NTSC 320 PLL frequency as **26.8741 MHz**. The Sega Saturn Overview Manual table 3.1.1 agrees. This supports the Mednafen timing reference, not the older 26.84659 MHz SDK value.

Mednafen represents NTSC master frequency as 1746818182 Hz (commented exact nominal value 1746818181.8181818181) and selects divisor 65 for 320 mode and 61 for 352 mode. Using that nominal rational:

| Quantity | Rational | Decimal |
| --- | --- | --- |
| NTSC 320 SH2 clock | 3843000000 / 143 Hz | 26874125.874125876 Hz |
| FRT at SH2 / 32 | 120093750 / 143 Hz | 839816.433566434 Hz |
| Older FRT conversion | 295312500 / 352 Hz | 838955.965909091 Hz |
| New / old conversion | 976 / 975 | 1.001025641026 |
| CPU ticks per observed frame | 449204 | 449204 |
| FRT ticks per observed frame | 449204 / 32 | 14037.625 |
| Implied refresh | (120093750 / 143) / 14037.625 Hz | 59.826105453482 Hz |

The old deadline conversion schedules a nominal 7.102464-second interval about 7.277 milliseconds early on the new nominal clock. This is separate from the much larger occasional counter-read failure. The rational is a nominal hardware timing model corroborated by the Sega rounded specification, not an oscillator measurement of the user's physical console.

## Primary evidence and local copies

- Hitachi SH7604 Hardware Manual: https://antime.kapsi.fi/sega/files/sh7604.pdf — table 11.2, section 11.3; FRT input-clock choices in section 11.2.6.
- Sega SMPC User's Manual: https://antime.kapsi.fi/sega/files/ST-169-R1-072694.pdf — table 1.1.
- Sega Saturn Overview Manual: https://www.infochunk.com/saturn/segahtml_en/hard/intr/hon/p03_10.htm — table 3.1.1.
- Yabause primary source: https://raw.githubusercontent.com/libretro/yabause/master/yabause/src/sh2core.c — local `yabause_sh2core.c`, `OnchipReadByte` and `OnchipReadWord`.
- Mednafen primary implementation: https://raw.githubusercontent.com/OpenEmu/Mednafen-Core/master/mednafen/ss/ss.cpp and https://raw.githubusercontent.com/OpenEmu/Mednafen-Core/master/mednafen/ss/smpc.cpp — existing read-only copies in `../../research_inventory/clock_reference/`, with their hashes and origins in that directory's `provenance.json`.

The source URLs identify the implementations read during this review. Source hashes in `provenance.json` pin the inspected Yabause file and the calculated values.
