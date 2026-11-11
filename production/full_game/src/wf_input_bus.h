#ifndef WF_INPUT_BUS_H
#define WF_INPUT_BUS_H
#include "wf_r18_video.h"

/* $517C reads the live bus address from actor+$8A. The original Royal
 * $20292 auxiliary keeps source0 until its first raw8E update, so this is
 * also a ROM read (WORD001C), not exclusively one of the four pad ports.
 * Preserve the port mapping and reject unrepresented/unaligned addresses.
 * A failed provider may have written partial bytes; publish only on success. */
static inline wf_status_t wf_input_bus_word(uint32_t at,uint16_t *out,
 const uint8_t port_low[4],wf_r18_rom_read_fn read_rom,void *user){
 at&=0x00ffffffu;
 if(!out||(at&1u))return WF_BAD_STATE;
 if(at>=0x140020u&&at<=0x140026u){
  if(!port_low)return WF_BAD_STATE;
  *out=(uint16_t)(0xff00u|port_low[(at-0x140020u)/2u]);return WF_OK;
 }
 if(at>=0x80000u||!read_rom)return WF_BAD_STATE;
 uint8_t bytes[2];wf_status_t status=read_rom(user,at,bytes,sizeof bytes);
 if(status==WF_OK)*out=(uint16_t)((uint16_t)bytes[0]<<8|bytes[1]);
 return status;
}
#endif
