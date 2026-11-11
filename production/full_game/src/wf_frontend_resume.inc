/* Included only in the --postmatch projection of the locked original driver,
 * so the reconnect uses its private callbacks rather than copied owners. */
#include "wf_frontend_resume.h"

wf_status_t wf_frontend_resume_bind(wf_r18_frontend_driver_t *d,
                                   const wf_r18_frontend_driver_config_t *c){
 if(!d||!c||!c->ram||c->ram_bytes<0x4000u||!c->runtime||!c->video||
    !c->platform.read_rom||!c->platform.read_bus||!c->platform.store_bus||
    !c->platform.wait_level||!c->platform.wait_vblank||!c->platform.commit||
    !c->platform.bind_world||!c->platform.submit_records||
    d->phase!=WF_R18_DRIVER_AC0||!d->regs.valid||d->irq_active||d->coin_active||
    d->call_depth||d->active_owner||d->fade.active||d->blocked_dependency||d->blocked_address||
    d->pc>=0x80000u||(d->pc&1u)||d->credited_start>1u||d->world_visible>1u||
    d->control.ssp<0x1c0000u||d->control.ssp>0x1c4000u||
    d->control.usp<0x1c0000u||d->control.usp>0x1c4000u||
    ((d->control.ssp|d->control.usp)&1u))return WF_BAD_STATE;

 /* No owner, callback or allocation runs between validation and completion.
  * In particular, init would erase the checkpoint and reject real user SR. */
 d->ram=c->ram;d->ram_bytes=c->ram_bytes;d->runtime=c->runtime;d->video=c->video;
 d->platform=c->platform;
 d->control.ram_1c0000=d->ram;d->control.ram_bytes=d->ram_bytes;
 d->control.regs=&d->regs;d->control.a=d->a;d->control.effect=control_effect;d->control.user=d;
 d->fade.ram=d->ram;d->fade.regs=&d->regs;
 d->fade.read_palette=read_palette;d->fade.write_palette=write_palette;d->fade.user=d;
 d->video->read_rom=read_rom;d->video->read_ram=read_ram;d->video->read_user=d;
 d->video->ring_cell=NULL;
 return WF_OK;
}
