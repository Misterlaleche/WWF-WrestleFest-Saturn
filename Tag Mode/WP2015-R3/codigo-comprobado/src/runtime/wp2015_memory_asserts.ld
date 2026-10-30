/* Additional final-link assertions; keep the SDK's normal yaul.x script.
 * Installed 32-bit TLSF: control 0xC74 + sentinel overhead 8 + min block 12.
 * Everything from 0x060DA000 upwards belongs to the sound module/shared area,
 * its reserved stack and the fixed slave stack, never the main user heap. */
ASSERT((___end & 3) == 0, "WP2015: user TLSF pool start is not aligned")
ASSERT(___end + 0xC88 <= 0x060DA000,
       "WP2015: main image leaves insufficient bounded user TLSF space")
ASSERT(___slave_stack == 0x060FF000,
       "WP2015: slave stack top differs from the reserved map")
