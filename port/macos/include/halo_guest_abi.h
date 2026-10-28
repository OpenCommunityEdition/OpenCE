#ifndef __HALO_GUEST_ABI_H
#define __HALO_GUEST_ABI_H

#include <stdint.h>

#define HALO_GUEST_IMAGE_BASE 0x70000000u
#define HALO_GUEST_IMAGE_RESERVE 0x10000000u

#define HALO_GUEST_WINDOW_BASE 0x80000000u
#define HALO_GUEST_WINDOW_SIZE 0x20000000u

#define HALO_GUEST_MAGIC 0x4f4c4148u
#define HALO_GUEST_ABI_VERSION 0x101

#ifdef __aarch64__
#define HALO_GUEST_BIAS 0x10000000000ull
#else
#define HALO_GUEST_BIAS 0ull
#endif
#define HALO_GUEST_ADDRESS(address) ((void *)(uintptr_t)(HALO_GUEST_BIAS | (uint32_t)(address)))

struct halo_guest_header
{
	uint32_t magic;
	uint32_t abi_version;
	uint32_t image_end;
	uint32_t import_table;
	uint32_t import_names;
	uint32_t import_count;
	uint32_t start;
	uint32_t thread_start;
	uint32_t thread_attach;
	uint32_t thread_detach;
	uint32_t init_array_start;
	uint32_t init_array_end;
};

struct halo_guest_boot
{
	uint32_t argc;
	uint32_t argv;
	uint32_t environment;
	uint32_t page_size;
};

#endif
