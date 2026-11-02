# The wrapper boundary needs native SDK relocations. Preserve game LTO and use
# only the existing native half of Yaul's allocator fat object.
WF_MEMORY_PYTHON?= python3
WF_SDK_ALLOCATOR_OBJ:= $(SH_BUILD_PATH)/wf_sdk_allocator_internal.o
WF_ALLOCATOR_ADAPTER_OBJ:= $(SH_BUILD_PATH)/wf_saturn_allocator_partition.o
WF_SDK_ALLOCATOR_PREPARE:= ../../tools/audio/native_probe/prepare_allocator_sdk.py
SH_SPECS:= yaul.specs yaul-main.specs ldscripts/wf_memory.specs
SH_DEFSYMS+= -Wl,--defsym=___master_stack=0x06004000 -Wl,--wrap=tlsf_pool_create -Wl,--wrap=tlsf_pool_get -Wl,--undefined=_wf_saturn_tlsf_pool_create -Wl,--undefined=_wf_saturn_tlsf_pool_get -Wl,--undefined=_tlsf_size -Wl,--undefined=_tlsf_pool_overhead
SH_LDFLAGS+= $(WF_SDK_ALLOCATOR_OBJ) $(WF_ALLOCATOR_ADAPTER_OBJ)

$(WF_SDK_ALLOCATOR_OBJ): $(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/lib/libyaul.a $(WF_SDK_ALLOCATOR_PREPARE)
	$(ECHO)$(WF_MEMORY_PYTHON) -B $(WF_SDK_ALLOCATOR_PREPARE) --archive $< --objcopy $(SH_OBJCOPY) --out $@

$(WF_ALLOCATOR_ADAPTER_OBJ): src/wf_saturn_allocator_partition.c
	$(ECHO)$(SH_CC) -std=c11 -m2 -mb -ffreestanding -fno-builtin -fno-lto -Os -Wall -Wextra -Werror -I$(YAUL_INSTALL_ROOT)/$(YAUL_ARCH_SH_PREFIX)/include/yaul -c $< -o $@

$(SH_BUILD_PATH)/$(SH_PROGRAM).elf: $(WF_SDK_ALLOCATOR_OBJ) $(WF_ALLOCATOR_ADAPTER_OBJ) ldscripts/wf_memory.x ldscripts/wf_memory.specs
