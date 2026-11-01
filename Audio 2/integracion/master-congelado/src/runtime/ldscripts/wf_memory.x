/*
 * Copyright (c)
 * See LICENSE for details.
 *
 * Joe Fenton <jlfenton65@gmail.com>
 * Israel Jacquez <mrkotfw@gmail.com>
 */

OUTPUT_FORMAT ("elf32-sh")
OUTPUT_ARCH (sh)
EXTERN (_start)
ENTRY (_start)
SEARCH_DIR ("$YAUL_INSTALL_ROOT/$YAUL_ARCH_SH_PREFIX/lib");

MEMORY {
  ram (Wx) : ORIGIN = 0x06004000, LENGTH = 0x000FC000
}

SECTIONS
{
  .text :
  {
     *(.text)
     *(.text.*)
     *(.gnu.linkonce.t.*)

     INCLUDE ldscripts/yaul-c++.x

     . = ALIGN (4);
  }

  .rodata :
  {
     . = ALIGN (16);

     *(.rdata)
     *(.rodata)
     *(.rodata.*)
     *(.gnu.linkonce.r.*)
  }

  .data :
  {
     . = ALIGN (16);

     *(.data)
     *(.data.*)
     *(.gnu.linkonce.d.*)
     SORT (CONSTRUCTORS)
     *(.sdata)
     *(.sdata.*)
     *(.gnu.linkonce.s.*)
  }

  /* SDK storage partition: the adapter limits TLSF to 14 KiB; the following 18 KiB is fixed music workspace. */
  .wf_sdk_private_pool (NOLOAD) : ALIGN(16)
  {
     . = ALIGN (16);
     ___wf_sdk_private_pool_start = .;
     KEEP (*(.bss._private_pool))
     ___wf_sdk_private_pool_end = .;
     ___wf_music_hot_start = ___wf_sdk_private_pool_start + 0x3800;
     ___wf_music_hot_end = ___wf_sdk_private_pool_start + 0x8000;
     ___wf_slave_stack_bottom = ___wf_music_hot_end;
     ___slave_stack = ___wf_sdk_private_pool_end;
  }

  .bss :
  {
     . = ALIGN (16);
     PROVIDE (___bss_start = .);

     *(.bss)
     *(.bss.*)
     *(.gnu.linkonce.b.*)
     *(.sbss)
     *(.sbss.*)
     *(.gnu.linkonce.sb.*)
     *(.scommon)
     *(COMMON)

     . = ALIGN (16);
     PROVIDE (___bss_end = .);
  }

  .uncached (0x20000000 | ___bss_end) : AT (___bss_end)
  {
     *(.uncached)
     *(.uncached.*)

     . = ALIGN (4);
  }

  /* Back to cached addresses */
  ___end = ___bss_end + SIZEOF (.uncached);
}

/* Both sides of the ownership contract must be linked together. */
/* SH BFD adds the target's leading underscore around C --wrap names. */
___wrap_tlsf_pool_create = _wf_saturn_tlsf_pool_create;
___wrap_tlsf_pool_get = _wf_saturn_tlsf_pool_get;
ASSERT(___wf_sdk_private_pool_end - ___wf_sdk_private_pool_start == 0xa000,
       "SDK private pool changed; review the allocator/stack partition")
ASSERT(___slave_stack - ___wf_slave_stack_bottom == 0x2000,
       "Slave stack requires an exclusive 8 KiB reservation")

ASSERT(___wf_music_hot_start - ___wf_sdk_private_pool_start == 0x3800,
       "Private TLSF requires its exclusive 14 KiB reservation")
ASSERT(___wf_music_hot_end - ___wf_music_hot_start == 0x4800,
       "Music hot workspace requires its exclusive 18 KiB reservation")
ASSERT(___wf_music_hot_end == ___wf_slave_stack_bottom,
       "Music hot workspace must end before the slave stack")
