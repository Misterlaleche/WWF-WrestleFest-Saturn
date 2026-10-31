/* HW1's optional ELF weak audit hook needs a definition on Mach-O hosts. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
uint32_t wf_session_host_rom_demands;
void wp1980_host_rom_demand(uint32_t address,size_t bytes){
 assert(address<=0x80000u&&bytes<=0x80000u-address);
 ++wf_session_host_rom_demands;
}
