/*
GUEST.LD

Layout of the Android guest image (tools/android_build.py): linked to run at
HALO_GUEST_IMAGE_BASE (port/android/include/halo_android_abi.h), with the
image header first. The host copies the loadable segments to these
addresses and makes the code read-only.
*/

ENTRY(__guest_header)

SECTIONS
{
	. = 0x88000000;
	.guest_header : { KEEP(*(.guest_header)) }
	.text : { *(.text .text.*) }
	.rodata : { *(.rodata .rodata.*) }
	. = ALIGN(0x10000);
	.data : { *(.data .data.*) }
	.guest_init : {
		__guest_init_start = .;
		KEEP(*(.guest_init))
		__guest_init_end = .;
	}
	.bss : { *(.bss .bss.*) *(COMMON) }
	. = ALIGN(0x1000);
	__guest_image_end = .;
	/DISCARD/ : { *(.comment) *(.note.*) *(.guest_fini) *(.eh_frame) }
}
