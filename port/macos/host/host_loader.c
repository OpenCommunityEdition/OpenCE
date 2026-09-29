/*
HOST_LOADER.C

Loads the guest image: a statically linked x32 ELF executable (32-bit ELF,
x86-64 machine; tools/macos_build.py) whose segments are copied to the
addresses it was linked at, below 4 GB. The image starts with a
struct halo_guest_header naming its import table, which is filled with the
host functions of the same names.
*/

#include "host.h"

#include <string.h>
#include <sys/mman.h>

/* the parts of <elf.h> this needs (macOS has none) */
typedef struct
{
	unsigned char e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} elf32_header;

typedef struct
{
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} elf32_segment;

#define ELF_CLASS32 1
#define ELF_EXECUTABLE 2
#define ELF_MACHINE_X86_64 62
#define SEGMENT_LOAD 1
#define SEGMENT_EXECUTE 1

struct host_guest_image host_image;

static void missing_import(void)
{
	host_fatal("the guest called a host function that is not available");
}

int host_load_image(const void *file, size_t size)
{
	const elf32_header *elf = file;
	const elf32_segment *segments;
	uint64_t low = ~0ULL, high = 0;
	const struct halo_guest_header *header;
	uint64_t *table;
	const char *name;
	uint32_t count, index;
	int missing = 0;

	if (size < sizeof(*elf) || memcmp(elf->e_ident, "\177ELF", 4) || elf->e_ident[4] != ELF_CLASS32 ||
		elf->e_machine != ELF_MACHINE_X86_64 || elf->e_type != ELF_EXECUTABLE)
	{
		host_logf(HOST_LOG_ERROR, "the guest image is not an x32 executable");
		return -1;
	}
	segments = (const elf32_segment *)((const char *)file + elf->e_phoff);
	for (index = 0; index < elf->e_phnum; index++)
	{
		if (segments[index].p_type != SEGMENT_LOAD)
			continue;
		if (segments[index].p_vaddr < low)
			low = segments[index].p_vaddr;
		if ((uint64_t)segments[index].p_vaddr + segments[index].p_memsz > high)
			high = (uint64_t)segments[index].p_vaddr + segments[index].p_memsz;
	}
	low &= ~0xfffULL;
	high = (high + 0xfff) & ~0xfffULL;
	if (low != HALO_GUEST_IMAGE_BASE || high > 0x100000000ULL)
	{
		host_logf(HOST_LOG_ERROR, "the guest image spans %llx-%llx", (unsigned long long)low, (unsigned long long)high);
		return -1;
	}
	if (host_memory_initialize((uint32_t)low, (uint32_t)(high - low)) != 0)
		return -1;
	if (mmap((void *)low, high - low, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) != (void *)low)
		return -1;
	for (index = 0; index < elf->e_phnum; index++)
	{
		const elf32_segment *segment = &segments[index];

		if (segment->p_type != SEGMENT_LOAD)
			continue;
		if ((uint64_t)segment->p_offset + segment->p_filesz > size)
			return -1;
		memcpy((void *)(uintptr_t)segment->p_vaddr, (const char *)file + segment->p_offset, segment->p_filesz);
	}

	header = (const struct halo_guest_header *)low;
	if (header->magic != HALO_GUEST_MAGIC || header->abi_version != HALO_GUEST_ABI_VERSION)
	{
		host_logf(HOST_LOG_ERROR, "the guest image header does not match this host");
		return -1;
	}
	host_image.header = header;
	host_image.base = (uint32_t)low;
	host_image.end = (uint32_t)high;

	table = (uint64_t *)(uintptr_t)header->import_table;
	name = (const char *)(uintptr_t)header->import_names;
	count = *(const uint32_t *)(uintptr_t)header->import_count;
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
	host_logf(HOST_LOG_INFO, "guest image %08llx-%08llx, %u imports (%d unavailable)",
		(unsigned long long)low, (unsigned long long)high, count, missing);

	/* code becomes read-only and executable */
	for (index = 0; index < elf->e_phnum; index++)
	{
		const elf32_segment *segment = &segments[index];
		uint64_t start = segment->p_vaddr & ~0xfffULL;
		uint64_t end = ((uint64_t)segment->p_vaddr + segment->p_memsz + 0xfff) & ~0xfffULL;

		if (segment->p_type == SEGMENT_LOAD && (segment->p_flags & SEGMENT_EXECUTE))
			mprotect((void *)start, end - start, PROT_READ | PROT_EXEC);
	}
	return 0;
}
