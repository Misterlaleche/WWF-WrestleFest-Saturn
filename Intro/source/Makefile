ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Set YAUL_INSTALL_ROOT to the supplied SH-2 Yaul toolchain)
endif
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
SH_PROGRAM := WF-WP0-R04
SH_SRCS := main.c wp0_cd_transfer32.c wp0_rice.c wp0_audio_player.c wp0_scsp.c wp0_framebuffer.c wp0_error_screen.c wp0_sequence_cache.c wp0_timer.c
SH_CFLAGS += -O2 -I. -Wall -Wextra
IP_VERSION := V1.000
IP_RELEASE_DATE := 20261029
IP_AREAS := JTUBKAEL
IP_PERIPHERALS := J
IP_TITLE := WWF WRESTLEFEST WP0
IP_MASTER_STACK_ADDR := 0x06004000
IP_SLAVE_STACK_ADDR := 0x06001E00
IP_1ST_READ_ADDR := 0x06004000
IP_1ST_READ_SIZE := 0
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
