/*
HALO_ANDROID_ABI.H

The contract between the two halves of the Android port (see
port/android/README.md):

- the guest: the game, the platform layer shared with the Linux port and a
  small C runtime, compiled as ILP32 AArch64 code (32-bit pointers) and
  linked into a static image that runs in the low 4 GB of the process;
- the host: an ordinary 64-bit Android library (libmain.so) that loads the
  image, owns the process (SDL3, OpenGL ES, bionic) and serves the guest's
  requests.

The guest calls the host through import stubs that jump through a table of
64-bit function pointers the host fills in at load time. Only types whose
layout and register treatment agree between the two ABIs cross this
boundary: 32-bit integers, 64-bit integers, floats, and pointers (which
arm64_32 always passes zero-extended). Structures shared here are made of
fixed-width members only.

This header is included by both halves.
*/

#ifndef __HALO_ANDROID_ABI_H
#define __HALO_ANDROID_ABI_H

#include <stdint.h>

/* the guest image is linked to run here. The window is 256 MB-aligned and
	128 MB long, so it can neither cover nor reach this address, and the host
	reserves the range before the guest's own pools take any of it */
#define HALO_GUEST_IMAGE_BASE 0x40000000u
#define HALO_GUEST_IMAGE_RESERVE 0x01000000u

/* the Xbox contiguous memory window (port/linux/src/platform.h). The game
data is linked to the addresses of the window, so the host looks for
HALO_GUEST_WINDOW_SIZE of free space below 4 GB at start-up, prefers
HALO_GUEST_WINDOW_BASE, hands the guest the address it found
(halo_guest_boot.contiguous_base), and the port moves the data with it */
#define HALO_GUEST_WINDOW_BASE 0x80000000u
#define HALO_GUEST_WINDOW_SIZE 0x08000000u
/* the window is a whole number of these, so the game's arithmetic on
physical addresses (an offset, or an address masked with ~base) still
works wherever it lands */
#define HALO_GUEST_WINDOW_ALIGNMENT 0x10000000u

#define HALO_GUEST_MAGIC 0x4f4c4148u /* 'HALO' */
#define HALO_GUEST_ABI_VERSION 1

/* at HALO_GUEST_IMAGE_BASE */
struct halo_guest_header
{
	uint32_t magic;
	uint32_t abi_version;
	uint32_t image_end;          /* end of .bss */
	uint32_t import_table;       /* uint64_t[import_count], filled by the host */
	uint32_t import_names;       /* import_count NUL-terminated names */
	uint32_t import_count;       /* address of a uint32_t holding the count */
	uint32_t start;              /* void __guest_start(struct halo_guest_boot *) */
	uint32_t thread_start;       /* void __guest_thread_start(uint32_t thread) */
	uint32_t thread_attach;      /* uint32_t __guest_thread_attach(void) */
	uint32_t init_array_start;   /* void (*)(void) entries, 4 bytes each */
	uint32_t init_array_end;
};

/* the host's description of the process, handed to __guest_start */
struct halo_guest_boot
{
	uint32_t argc;
	uint32_t argv;               /* char ** in guest memory */
	uint32_t environment;        /* char ** in guest memory, NULL-terminated */
	uint32_t page_size;
	uint32_t contiguous_base;     /* where the host put the Xbox window */
};

#endif
