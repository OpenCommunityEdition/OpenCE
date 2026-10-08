/*
GUEST.LD

Layout of the macOS guest image (tools/macos_build.py): linked to run at
HALO_GUEST_IMAGE_BASE (port/android/include/halo_android_abi.h), with the
image header first. The host copies the loadable segments to these
addresses and makes the code read-only (port/macos/host/host_loader.c).

The image must end below 2 GB: x32 code built without position
independence holds absolute addresses in sign-extended 32-bit fields
(R_X86_64_32S).

Constructors come from ELF's .init_array, ordered by priority (the Xbox
window must be reserved, at priority 101, before anything maps memory);
the guest's __guest_start runs them from __guest_init_start to
__guest_init_end. Destructors never run, as on Android.
*/

ENTRY(__guest_header)

SECTIONS
{
	. = 0x10000000;
	.guest_header : { KEEP(*(.guest_header)) }
	.text : { *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	. = ALIGN(0x10000);
	.data : { *(.data .data.*) }
	.guest_init : {
		__guest_init_start = .;
		KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.init_array)))
		__guest_init_end = .;
	}
	.bss : { *(.bss .bss.*) *(COMMON) }
	. = ALIGN(0x1000);
	__guest_image_end = .;
	/DISCARD/ : { *(.comment) *(.note.*) *(.fini_array .fini_array.*) *(.eh_frame) *(.eh_frame_hdr) }
}
