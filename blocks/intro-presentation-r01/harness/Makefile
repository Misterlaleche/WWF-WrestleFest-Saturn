ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT)
endif
include $(YAUL_INSTALL_ROOT)/share/build.pre.mk
BUILTIN_ASSETS += assets/INTRO.RLE;asset_intro
SH_PROGRAM := wf-intro-presentation-r01-candidate
SH_SRCS := main.c ../src/wf_intro_presentation.c
SH_CFLAGS += -Os -I../src -Wall -Wextra -Werror -DWF_HARNESS_FBI_ENABLED=1
IP_VERSION := V0.001
IP_RELEASE_DATE := 20260911
IP_AREAS := JTUBKAEL
IP_PERIPHERALS := J
IP_TITLE := WWF WF INTRO R01
IP_MASTER_STACK_ADDR := 0x06004000
IP_SLAVE_STACK_ADDR := 0x06001E00
IP_1ST_READ_ADDR := 0x06004000
IP_1ST_READ_SIZE := 0
include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
