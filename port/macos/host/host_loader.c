/*
HOST_LOADER.C

Loads the guest image (the Android port's: a statically linked AArch64 ELF
executable built from ILP32 code, linked at HALO_GUEST_IMAGE_BASE; see
port/android/README.md and tools/macos_build.py) into the virtual machine:
every loadable segment is copied into guest memory at the address it was
linked at, and the header's import table is filled with the host's
doorbell-capable entry points (unused on this port: the stubs doorbell by
import index, but the header contract is the Android image's and the
table's names identify each index for the host's dispatch table).
*/

#include "host.h"

#include <Hypervisor/Hypervisor.h>

/* (macOS has no <elf.h>; only what the guest image's own header needs) */
typedef struct { unsigned char e_ident[16]; unsigned short e_type, e_machine;
	unsigned e_version; unsigned long long e_entry, e_phoff, e_shoff;
	unsigned e_flags, e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx; } Elf64_Ehdr;
typedef struct { unsigned p_type, p_flags; unsigned long long p_offset, p_vaddr, p_paddr,
	p_filesz, p_memsz, p_align; } Elf64_Phdr;
#define EI_CLASS 4
#define ELFMAG "ELF"
#define SELFMAG 4
#define ELFCLASS64 2
#define EM_AARCH64 183
#define ET_EXEC 2
#define PT_LOAD 1
#include <string.h>
#include <sys/mman.h>

struct host_guest_image host_image;

static uint8_t *image_host; /* the host memory of the image */

uint8_t *host_image_host_memory(void)
{
	return image_host;
}

/* the doorbell dispatch's name table (host_imports.c generated it from
the same lists the image's stubs came from) */
void *host_resolve_import(const char *name)
{
	unsigned index;

	for (index = 0; index < host_import_count; index++)
	{
		if (!strcmp(host_import_table[index].name, name))
			return host_import_table[index].function;
	}
	return NULL;
}

int host_load_image(const void *file, size_t size)
{
	const Elf64_Ehdr *elf = file;
	const Elf64_Phdr *segments;
	uint64_t low = ~0ULL, high = 0;
	const struct halo_guest_header *header;
	uint64_t *table;
	const char *name;
	uint32_t count, index;
	int missing = 0;

	if (size < sizeof(*elf) || memcmp(elf->e_ident, ELFMAG, SELFMAG) || elf->e_ident[EI_CLASS] != ELFCLASS64 ||
		elf->e_machine != EM_AARCH64 || elf->e_type != ET_EXEC)
	{
		host_logf(HOST_LOG_ERROR, "the guest image is not an AArch64 executable");
		return -1;
	}
	segments = (const Elf64_Phdr *)((const char *)file + elf->e_phoff);
	for (index = 0; index < elf->e_phnum; index++)
	{
		if (segments[index].p_type != PT_LOAD)
			continue;
		if (segments[index].p_vaddr < low)
			low = segments[index].p_vaddr;
		if (segments[index].p_vaddr + segments[index].p_memsz > high)
			high = segments[index].p_vaddr + segments[index].p_memsz;
	}
	low &= ~0xfffULL;
	high = (high + 0xfff) & ~0xfffULL;
	if (low != GUEST_IMAGE_BASE || high > GUEST_IMAGE_BASE + GUEST_IMAGE_MAXIMUM)
	{
		host_logf(HOST_LOG_ERROR, "the guest image spans %llx-%llx (the host builds for %x..%x)",
			(unsigned long long)low, (unsigned long long)high, GUEST_IMAGE_BASE,
			GUEST_IMAGE_BASE + GUEST_IMAGE_MAXIMUM);
		return -1;
	}
	{
		uint32_t map_size = (uint32_t)(high - low);

		image_host = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (image_host == MAP_FAILED)
			return -1;
		if (hv_vm_map(image_host, (uint32_t)low, map_size,
			HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC) != HV_SUCCESS)
			return -1;
		for (index = 0; index < elf->e_phnum; index++)
		{
			const Elf64_Phdr *segment = &segments[index];

			if (segment->p_type != PT_LOAD)
				continue;
			if (segment->p_offset + segment->p_filesz > size)
				return -1;
			memcpy(image_host + (segment->p_vaddr - low),
				(const char *)file + segment->p_offset, segment->p_filesz);
		}
		{
			uint32_t page;

			for (page = 0; page < map_size / GUEST_PAGE_SIZE; page++)
				host_mirror_install((uint32_t)low + page * GUEST_PAGE_SIZE,
					image_host + page * GUEST_PAGE_SIZE);
		}
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

	/* the import table: the names tell the host which doorbell index is
	which function (the stubs hard-code their indexes) */
	table = (uint64_t *)(uintptr_t)header->import_table;
	name = (const char *)(uintptr_t)header->import_names;
	count = *(const uint32_t *)(uintptr_t)header->import_count;
	if (count != host_import_count)
		host_fatal("the image imports %u functions, the host serves %u", (unsigned)count,
			(unsigned)host_import_count);
	{
		const char *dispatch_name;
		unsigned table_index;

		for (index = 0, dispatch_name = (const char *)(uintptr_t)header->import_names;
			index < count; index++)
		{
			void *function = host_resolve_import(dispatch_name);

			if (!function && !strncmp(dispatch_name, "hostgl_", 7))
				function = host_gl_resolve(dispatch_name + 7);
			/* (the stubs doorbell by index, never through this table;
			it exists so a debugger can name each doorbell) */
			table[index] = (uint64_t)(uintptr_t)function;
			if (!function)
			{
				host_logf(HOST_LOG_WARN, "guest import %s is not available", dispatch_name);
				missing++;
			}
			/* the host's dispatch order must be the image's import order */
			if (strcmp(host_import_table[index].name, dispatch_name))
				host_fatal("import %u is %s here, %s in the image", (unsigned)index,
					host_import_table[index].name, dispatch_name);
			dispatch_name += strlen(dispatch_name) + 1;
		}
		(void)name;
		(void)table_index;
	}
	host_logf(HOST_LOG_INFO, "guest image %08llx-%08llx, %u imports (%d unavailable)",
		(unsigned long long)low, (unsigned long long)high, count, missing);
	return 0;
}