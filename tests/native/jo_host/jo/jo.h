#ifndef WF_HOST_MAIN_JO_H
#define WF_HOST_MAIN_JO_H
/* Test-only hardware surface for compiling the actual port/main.c. */
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "wf_jo_frontend_test_platform.h"
#define JO_COMPILE_WITH_FS_SUPPORT
typedef struct wf_host_unavailable_file jo_file; /* Only pointer prototypes used. */
#define __jo_force_inline inline
#include "../../../../sdk/jo_engine/jo/fs.h"
#define JO_MIN(a,b) ((a)<(b)?(a):(b))
/* Native gameplay uses host asset files and never enters the CD driver.
 * Preserve its diagnostic ABI, with zero device activity at this boundary. */
static inline void wf_host_cd_diagnostics(jo_fs_cd_diagnostics *out)
{if(out)*out=(jo_fs_cd_diagnostics){0};}
#define jo_fs_get_cd_diagnostics wf_host_cd_diagnostics

extern volatile uint32_t wf_host_jo_registers[4];
#define JO_RAM_CARTRIDGE_ID wf_host_jo_registers[0]
#define JO_RAM_CARTRIDGE_ENABLE wf_host_jo_registers[1]
#define JO_SCU_ASR0 wf_host_jo_registers[2]
#define JO_SCU_AREF wf_host_jo_registers[3]
#define JO_RAM_CARTRIDGE_ID_32MBITS 0x5cu
#define JO_RAM_CARTRIDGE_ASR0 0u
#define JO_RAM_CARTRIDGE_AREF 0u
#define JO_COLOR_Black 0u
#define JO_PROFILE_USER(n) (n)
static inline void jo_profile_begin(unsigned n){(void)n;}
static inline void jo_profile_end(unsigned n){(void)n;}
static inline void jo_clear_screen(void){}
static inline int jo_core_get_refresh_rate(void){return 60;}
void jo_printf(int x,int y,const char *format,...);
bool jo_is_pad1_key_down(unsigned key);
void jo_goto_boot_menu(void);
void jo_core_init(unsigned color);
void jo_core_add_callback(void (*callback)(void));
void jo_core_run(void);
#endif
