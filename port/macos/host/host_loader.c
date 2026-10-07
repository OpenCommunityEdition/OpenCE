/*
HOST_LOADER.C

Loads the guest image: a statically linked AArch64 ELF executable (built
from ILP32 code, see tools/guest_build.py) whose segments are copied to the
guest addresses it was linked at, inside the guest's 4 GB. The image starts
with a struct halo_guest_header naming its import table, which is filled
with the host functions of the same names.
*/

#include "host.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <libkern/OSCacheControl.h>

/* host_memory.c */
int host_image_map(uint32_t address, uint32_t size, int protection);
int host_image_protect(uint32_t address, uint32_t size, int protection);

/* the ELF structures read here (macOS has no <elf.h>) */
struct elf_header
{
	unsigned char ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t program_headers;
	uint64_t section_headers;
	uint32_t flags;
	uint16_t header_size;
	uint16_t program_header_size;
	uint16_t program_header_count;
	uint16_t section_header_size;
	uint16_t section_header_count;
	uint16_t section_names;
};

struct elf_program_header
{
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t address;
	uint64_t physical_address;
	uint64_t file_size;
	uint64_t memory_size;
	uint64_t alignment;
};

#define ELF_CLASS_64 2
#define ELF_EXECUTABLE 2
#define ELF_AARCH64 183
#define ELF_LOAD 1
#define ELF_EXECUTE 1
#define ELF_WRITE 2

struct host_guest_image host_image;

static void missing_import(void)
{
	host_fatal("the guest called a host function that is not available");
}

int host_load_image(const void *file, size_t size)
{
	const struct elf_header *elf = file;
	const struct elf_program_header *segments;
	uint64_t low = ~0ULL, high = 0;
	const struct halo_guest_header *header;
	uint64_t *table;
	const char *name;
	uint32_t count, index;
	int missing = 0;

	if (size < sizeof(*elf) || memcmp(elf->ident, "\177ELF", 4) || elf->ident[4] != ELF_CLASS_64 ||
		elf->machine != ELF_AARCH64 || elf->type != ELF_EXECUTABLE ||
		elf->program_headers + (uint64_t)elf->program_header_count * sizeof(*segments) > size)
	{
		host_logf(HOST_LOG_ERROR, "the guest image is not an AArch64 executable");
		return -1;
	}
	segments = (const struct elf_program_header *)((const char *)file + elf->program_headers);
	for (index = 0; index < elf->program_header_count; index++)
	{
		if (segments[index].type != ELF_LOAD)
			continue;
		if (segments[index].address < low)
			low = segments[index].address;
		if (segments[index].address + segments[index].memory_size > high)
			high = segments[index].address + segments[index].memory_size;
	}
	low &= ~0xfffULL;
	high = (high + 0xfff) & ~0xfffULL;
	if (low != HALO_GUEST_IMAGE_BASE || high > HALO_MACOS_GUEST_SPAN)
	{
		host_logf(HOST_LOG_ERROR, "the guest image spans %llx-%llx", (unsigned long long)low, (unsigned long long)high);
		return -1;
	}
	if (host_memory_initialize((uint32_t)low, (uint32_t)(high - low)) != 0)
		return -1;
	if (host_image_map((uint32_t)low, (uint32_t)(high - low), PROT_READ | PROT_WRITE) != 0)
		return -1;
	for (index = 0; index < elf->program_header_count; index++)
	{
		const struct elf_program_header *segment = &segments[index];

		if (segment->type != ELF_LOAD)
			continue;
		if (segment->offset + segment->file_size > size)
			return -1;
		memcpy(G2H(segment->address), (const char *)file + segment->offset, segment->file_size);
	}

	header = G2H(low);
	if (header->magic != HALO_GUEST_MAGIC || header->abi_version != HALO_GUEST_ABI_VERSION)
	{
		host_logf(HOST_LOG_ERROR, "the guest image header does not match this host");
		return -1;
	}
	host_image.header = header;
	host_image.base = (uint32_t)low;
	host_image.end = (uint32_t)high;

	table = G2H(header->import_table);
	name = G2H(header->import_names);
	count = *(const uint32_t *)G2H(header->import_count);
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

	/* each segment gets its own protection: the code read-only and
	executable, the constants read-only. A 4 KB page two segments share gets
	both's, and so (host_memory.c) does a 16 KB host page: the code's last
	shares one with the constants, which must not be writable for it to be
	executable (the linker script starts the data on a 64 KB boundary) */
	sys_icache_invalidate(G2H(low), high - low);
	{
		uint32_t page_count = (uint32_t)((high - low) / 0x1000), page, run;
		int pass;
		uint8_t *protections = calloc(page_count, 1);

		if (!protections)
			return -1;
		for (index = 0; index < elf->program_header_count; index++)
		{
			const struct elf_program_header *segment = &segments[index];
			uint64_t first = ((segment->address & ~0xfffULL) - low) / 0x1000;
			uint64_t last = ((segment->address + segment->memory_size + 0xfff) & ~0xfffULL) - low;
			int protection = PROT_READ;

			if (segment->type != ELF_LOAD)
				continue;
			if (segment->flags & ELF_WRITE)
				protection |= PROT_WRITE;
			if (segment->flags & ELF_EXECUTE)
				protection |= PROT_EXEC;
			for (page = (uint32_t)first; page < last / 0x1000; page++)
				protections[page] |= (uint8_t)protection;
		}
		/* (the code last, so that a host page it shares is never writable
		and executable at once on the way) */
		for (pass = 0; pass < 2; pass++)
		{
			for (page = 0; page < page_count; page = run)
			{
				for (run = page + 1; run < page_count && protections[run] == protections[page]; run++)
				{
				}
				if (protections[page] != (PROT_READ | PROT_WRITE) && !(protections[page] & PROT_EXEC) == !pass)
					host_image_protect((uint32_t)(low + page * 0x1000ULL), (run - page) * 0x1000,
						protections[page] ? protections[page] : PROT_READ);
			}
		}
		free(protections);
	}
	return 0;
}
