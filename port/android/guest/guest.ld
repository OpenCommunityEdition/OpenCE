/*
GUEST.LD

Layout of the guest image (tools/android_build.py): linked to run at
HALO_GUEST_IMAGE_BASE (port/android/include/halo_android_abi.h), which the
link gives as __guest_image_base (tools/guest_arch.py, image_base), with the
image header first. The host copies the loadable segments to these
addresses and makes the code read-only.

The constructors are the converted Mach-O ones (.guest_init) or, for a guest
that clang compiles to ELF itself, .init_array's, in their priority order:
both 4-byte pointers, which __guest_start calls. Which of them a guest has
is its architecture's guest_init_array.ld, in the folder the link names
(-L, tools/android_build.py: ld/elf or ld/macho). A guest compiled to Mach-O
takes no .init_array: what it would come from is the 64-bit runtime library
(compiler-rt's AArch64 builtins), whose 8-byte entries __guest_start would
read as two, and it is discarded. tools/guest_image_check.py checks that
every entry is the address of code. Nothing is ever finalized
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
		INCLUDE guest_init_array.ld
		__guest_init_end = .;
	}
	.bss : { *(.bss .bss.*) *(COMMON) }
	. = ALIGN(0x1000);
	__guest_image_end = .;
	/DISCARD/ : { *(.comment) *(.note.*) *(.guest_fini) *(.fini_array .fini_array.*) *(.eh_frame)
		*(.init_array .init_array.*) }
}
