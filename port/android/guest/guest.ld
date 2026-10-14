/*
GUEST.LD

Layout of the guest image (tools/android_build.py): linked to run at
HALO_GUEST_IMAGE_BASE (port/android/include/halo_android_abi.h), which the
link gives as __guest_image_base (tools/guest_arch.py, image_base), with the
image header first. The host copies the loadable segments to these
addresses and makes the code read-only.

The constructors are the converted Mach-O ones (.guest_init) or, for a guest
that clang compiles to ELF itself, .init_array's, in their priority order:
both 4-byte pointers, which __guest_start calls. Nothing is ever finalized
(the game ends the process), and there is no thread-local storage: the
guest's thread pointer belongs to the host (-femulated-tls).
*/

ENTRY(__guest_header)

SECTIONS
{
	. = __guest_image_base;
	.guest_header : { KEEP(*(.guest_header)) }
	.text : { *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	. = ALIGN(0x10000);
	.data : { *(.data .data.*) }
	.guest_init : {
		__guest_init_start = .;
		KEEP(*(.guest_init))
		KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) .init_array))
		__guest_init_end = .;
	}
	.bss : { *(.bss .bss.*) *(COMMON) }
	. = ALIGN(0x1000);
	__guest_image_end = .;
	/DISCARD/ : { *(.comment) *(.note.*) *(.guest_fini) *(.fini_array .fini_array.*) *(.eh_frame) }
}
