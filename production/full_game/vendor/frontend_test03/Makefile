ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT)
endif
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
BUILTIN_ASSETS+= assets/MODEX.RLE;asset_modex
BUILTIN_ASSETS+= assets/MODE.PAL;asset_modepal
BUILTIN_ASSETS+= assets/CURSOR0.BIN;asset_cursor0
BUILTIN_ASSETS+= assets/CURSOR0.PAL;asset_cursor0pal
BUILTIN_ASSETS+= assets/SELBG.RLE;asset_selbg
BUILTIN_ASSETS+= assets/SELWORLD.RLE;asset_selworld
BUILTIN_ASSETS+= assets/SELPAL.BIN;asset_selpal
SH_PROGRAM:= wrestlefest-saturn-mode-roster-integration-test03
SH_SRCS:= main.c
SH_CFLAGS+= -Os -I. -Wall -Wextra
IP_VERSION:= V0.018
IP_RELEASE_DATE:= 20260911
IP_AREAS:= U
IP_PERIPHERALS:= J
IP_TITLE:= WF MODE ROSTER T03
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
