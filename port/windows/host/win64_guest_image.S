/* the guest image (tools/linux_guest_build.py's halo_guest.elf), inside the
Windows host's executable; win64_main.c loads it */

	.section .rdata, "dr"
	.p2align 4
	.globl halo_guest_image_start
	.globl halo_guest_image_end
halo_guest_image_start:
	.incbin GUEST_IMAGE
halo_guest_image_end:
