#include "host.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef __aarch64__
typedef uint64_t elf_word;
#define ELF_CLASS 2
#define ELF_MACHINE 183
#else
typedef uint32_t elf_word;
#define ELF_CLASS 1
#define ELF_MACHINE 62
#endif
#define ELF_EXECUTABLE 2
#define ELF_SEGMENT_LOAD 1
#define ELF_SEGMENT_EXECUTE 1

struct elf_header
{
	unsigned char identification[16];
	uint16_t type, machine;
	uint32_t version;
	elf_word entry, program_headers, section_headers;
	uint32_t flags;
	uint16_t header_size, program_header_size, program_header_count;
	uint16_t section_header_size, section_header_count, section_names;
};

struct elf_segment
{
	uint32_t type;
#ifdef __aarch64__
	uint32_t flags;
#endif
	elf_word offset, address, physical_address, file_size, memory_size;
#ifndef __aarch64__
	uint32_t flags;
#endif
	elf_word alignment;
};

struct host_guest_image host_image;

static void missing_import(void)
{
	host_fatal("the guest called a host function that is not available");
}

int host_load_image(const void *file, size_t size)
{
	const struct elf_header *elf = file;
	const struct elf_segment *segments;
	uint32_t low = ~0u, high = 0;
	const struct halo_guest_header *header;
	uint64_t *table;
	const char *name;
	uint32_t count, index;
	int missing = 0;

	if (size < sizeof(*elf) || memcmp(elf->identification, "\177ELF", 4) || elf->identification[4] != ELF_CLASS ||
		elf->machine != ELF_MACHINE || elf->type != ELF_EXECUTABLE ||
		elf->program_headers + (size_t)elf->program_header_count * sizeof(*segments) > size)
	{
		host_logf(HOST_LOG_ERROR, "the guest image is not this Mac's");
		return -1;
	}
	segments = (const struct elf_segment *)((const char *)file + elf->program_headers);
	for (index = 0; index < elf->program_header_count; index++)
	{
		if (segments[index].type != ELF_SEGMENT_LOAD)
			continue;
		if (segments[index].offset + (size_t)segments[index].file_size > size)
			return -1;
		if (segments[index].address < low)
			low = (uint32_t)segments[index].address;
		if (segments[index].address + segments[index].memory_size > high)
			high = (uint32_t)(segments[index].address + segments[index].memory_size);
	}
	low &= ~0xfffu;
	high = (high + 0xfff) & ~0xfffu;
	if (low != HALO_GUEST_IMAGE_BASE || high - low > HALO_GUEST_IMAGE_RESERVE)
	{
		host_logf(HOST_LOG_ERROR, "the guest image spans %x-%x", low, high);
		return -1;
	}
	if (host_memory_initialize(low, high - low) != 0)
		return -1;
	if (mmap(HALO_GUEST_ADDRESS(low), high - low, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) !=
		HALO_GUEST_ADDRESS(low))
	{
		return -1;
	}
	for (index = 0; index < elf->program_header_count; index++)
	{
		if (segments[index].type == ELF_SEGMENT_LOAD)
		{
			memcpy(HALO_GUEST_ADDRESS(segments[index].address), (const char *)file + segments[index].offset,
				segments[index].file_size);
		}
	}

	header = HALO_GUEST_ADDRESS(low);
	if (header->magic != HALO_GUEST_MAGIC || header->abi_version != HALO_GUEST_ABI_VERSION)
	{
		host_logf(HOST_LOG_ERROR, "the guest image header does not match this host");
		return -1;
	}
	host_image.header = header;
	host_image.base = low;
	host_image.end = high;

	table = HALO_GUEST_ADDRESS(header->import_table);
	name = HALO_GUEST_ADDRESS(header->import_names);
	count = *(const uint32_t *)HALO_GUEST_ADDRESS(header->import_count);
	for (index = 0; index < count; index++)
	{
		void *function = host_resolve_import(name);

		if (!function && !strncmp(name, "hostgl_", 7))
			function = host_gl_resolve(name + 7);
		if (!function)
		{
			host_logf(HOST_LOG_WARN, "guest import %s is not available", name);
			function = (void *)missing_import;
			missing++;
		}
		table[index] = (uint64_t)(uintptr_t)function;
		name += strlen(name) + 1;
	}
	host_logf(HOST_LOG_INFO, "guest image %08x-%08x, %u imports (%d unavailable)", low, high, count, missing);

	for (index = 0; index < elf->program_header_count; index++)
	{
		uint32_t start = (uint32_t)segments[index].address & ~(uint32_t)(getpagesize() - 1);
		uint32_t end = (uint32_t)(segments[index].address + segments[index].memory_size + getpagesize() - 1) &
			~(uint32_t)(getpagesize() - 1);

		if (segments[index].type == ELF_SEGMENT_LOAD && (segments[index].flags & ELF_SEGMENT_EXECUTE))
			mprotect(HALO_GUEST_ADDRESS(start), end - start, PROT_READ | PROT_EXEC);
	}
	__builtin___clear_cache((char *)HALO_GUEST_ADDRESS(low), (char *)HALO_GUEST_ADDRESS(low) + (high - low));
	return 0;
}
