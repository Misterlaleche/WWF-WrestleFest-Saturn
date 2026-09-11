# WP24 Saturn music transport

Reusable source extracted from the hardware-proven audio diagnostic path.

Status as of 2026-09-11:
- slot-31 transport: HARDWARE-PASS on real NTSC-U Saturn + Terraonion MODE;
- long PCM16BE streaming: HARDWARE-PASS;
- game-state audio integration: OPEN.

These files implement transport only. They do not define final music lifetime, arcade command producers, the 0x1F fade lifecycle or final production music assets.

Do not add guessed PCM loop metadata. The arcade contract has no demonstrated short PCM loop object.

See:
- ../../../docs/AUDIO-HARDWARE-VALIDATION.md
- ../../../PROJECT-STATE.md
