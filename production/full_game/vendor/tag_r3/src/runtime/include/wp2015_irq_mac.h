#ifndef WP2015_IRQ_MAC_H
#define WP2015_IRQ_MAC_H
/* Install after SDK startup and smpc_peripheral_init(), before the game starts.
 * Keeps each complete registered handler behind a local SH-2 MAC guard.
 * Returns 0 on success; negative on missing/already-replaced handler state.
 * No interrupt masks, SCU acknowledgements or SDK/BIOS bytes are changed.
 */
int wp2015_irq_mac_install(void);
int wp2015_irq_mac_verify(void);
#endif
