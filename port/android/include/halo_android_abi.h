/*
HALO_ANDROID_ABI.H

The contract between the two halves of the Android port (see
port/android/README.md), and of the Linux builds made the same way (the
64-bit ARM and x86 ones, port/linux/host):

- the guest: the game, the platform layer shared with the Linux port and a
  small C runtime, compiled as ILP32 code of the processor (32-bit pointers:
  arm64_32 on AArch64, x32 on x86-64) and linked into a static image that
  runs in the low 4 GB of the process;
- the host: an ordinary 64-bit Android library (libmain.so), or Linux
  executable, that loads the image, owns the process (SDL3, OpenGL, the C
  library) and serves the guest's requests.

The guest calls the host through import stubs that jump through a table of
64-bit function pointers the host fills in at load time. Only types whose
layout and register treatment agree between the two ABIs cross this
boundary: 32-bit integers, 64-bit integers, floats, and pointers (which
arm64_32 and x32 pass zero-extended; the host passes the guest its
arguments as 64-bit values, port/android/host/host_thread.c). Structures shared here are made of
fixed-width members only.

This header is included by both halves.
*/

#ifndef __HALO_ANDROID_ABI_H
#define __HALO_ANDROID_ABI_H

#include <stdint.h>

/* the guest image is linked to run here (tools/guest_arch.py's image_base,
which must agree). AArch64: just above the Xbox window, as Android's ART
keeps its heaps low in the address space and fills it upwards. x86-64 (x32):
below 2 GB, where the 32-bit sign-extended addresses of x32's static code
can reach it; the window above it is reached through registers. */
#if defined(__x86_64__)
#define HALO_GUEST_IMAGE_BASE 0x70000000u
#else
#define HALO_GUEST_IMAGE_BASE 0x88000000u
#endif
/* the room the host reserves for the image there (port/android/host/host_memory.c) */
#define HALO_GUEST_IMAGE_RESERVE 0x04000000u

/* the Xbox contiguous memory window (port/linux/src/platform.h) */
#define HALO_GUEST_WINDOW_BASE 0x80000000u
#define HALO_GUEST_WINDOW_SIZE 0x08000000u

#define HALO_GUEST_MAGIC 0x4f4c4148u /* 'HALO' */
/* 3: the system call interface of halo_guest_syscall.h (_llseek) */
#define HALO_GUEST_ABI_VERSION 3

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
	uint32_t thread_detach;      /* void __guest_thread_detach(void) */
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
};

#endif
