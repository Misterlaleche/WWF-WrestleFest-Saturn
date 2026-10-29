ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Set YAUL_INSTALL_ROOT to the supplied SH-2 Yaul toolchain)
endif
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
SH_PROGRAM := WF-WP01-R02
SH_SRCS := main.c wp01_scene.c wp01_audio.c wp01_audio_queue.c cd_transfer32.c
SH_CFLAGS += -O2 -I. -Wall -Wextra
BUILTIN_ASSETS += assets/BG0.TEX;asset_bg0
BUILTIN_ASSETS += assets/BG1.TEX;asset_bg1
BUILTIN_ASSETS += assets/FONT.TEX;asset_font
BUILTIN_ASSETS += assets/CROWD0.BIN;asset_crowd0
BUILTIN_ASSETS += assets/CROWD1.BIN;asset_crowd1
BUILTIN_ASSETS += assets/CROWD2.BIN;asset_crowd2
BUILTIN_ASSETS += assets/MAP1.BIN;asset_map1
BUILTIN_ASSETS += assets/REFERENCE_TEXT.BIN;asset_text
BUILTIN_ASSETS += assets/ENVPAL.C32;asset_envpal
BUILTIN_ASSETS += assets/ENTRANCE.TEX;asset_sprites
BUILTIN_ASSETS += assets/ENTRPAL.C32;asset_spritepal
IP_VERSION := V1.002
IP_RELEASE_DATE := 20261029
IP_AREAS := JTUBKAEL
IP_PERIPHERALS := J
IP_TITLE := WWF WRESTLEFEST WP01 NATIVE AUDIO
IP_MASTER_STACK_ADDR := 0x06004000
IP_SLAVE_STACK_ADDR := 0x06001E00
IP_1ST_READ_ADDR := 0x06004000
IP_1ST_READ_SIZE := 0
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
