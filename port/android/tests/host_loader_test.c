/*
HOST_LOADER_TEST.C

Tests of the guest image's checks (port/android/host/host_loader.c:
host_image_validate and host_image_validate_imports) on ELF files and
images made here, of both classes whatever the build machine is. Built and
run by tools/test_host_units.py; exits nonzero on a failure.
*/

#include "host.h"

#include <elf.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- what host_loader.c calls besides */

static char last_error[512];

void host_logf(int priority, const char *format, ...)
{
	va_list arguments;

	(void)priority;
	va_start(arguments, format);
	vsnprintf(last_error, sizeof(last_error), format, arguments);
	va_end(arguments);
}

void host_fatal(const char *format, ...)
{
	(void)format;
	abort();
}

int host_memory_initialize(uint32_t base, uint32_t size)
{
	(void)base;
	(void)size;
	return -1;
}

void *host_resolve_import(const char *name)
{
	(void)name;
	return NULL;
}

void *host_gl_resolve(const char *name)
{
	(void)name;
	return NULL;
}

/* ---------- checking */

static int failures;

static void expect(int condition, const char *what, int line)
{
	if (!condition)
	{
		printf("line %d: %s (last log: %s)\n", line, what, last_error);
		failures++;
	}
}

#define EXPECT(condition) expect((condition), #condition, __LINE__)

/* ---------- ELF files */

#define FILE_SIZE 0x3000

struct segment_spec
{
	uint32_t type, flags;
	uint64_t offset, address, file_size, memory_size;
};

/* an executable of the class with the segments, headers right after the
file header */
static size_t make_elf(unsigned char *file, int elf_class, int machine, const struct segment_spec *segments, int count)
{
	int index;

	memset(file, 0, FILE_SIZE);
	if (elf_class == ELFCLASS64)
	{
		Elf64_Ehdr header;

		memset(&header, 0, sizeof(header));
		memcpy(header.e_ident, ELFMAG, SELFMAG);
		header.e_ident[EI_CLASS] = ELFCLASS64;
		header.e_ident[EI_DATA] = ELFDATA2LSB;
		header.e_ident[EI_VERSION] = EV_CURRENT;
		header.e_type = ET_EXEC;
		header.e_machine = (Elf64_Half)machine;
		header.e_version = EV_CURRENT;
		header.e_phoff = sizeof(header);
		header.e_ehsize = sizeof(header);
		header.e_phentsize = sizeof(Elf64_Phdr);
		header.e_phnum = (Elf64_Half)count;
		memcpy(file, &header, sizeof(header));
		for (index = 0; index < count; index++)
		{
			Elf64_Phdr program;

			memset(&program, 0, sizeof(program));
			program.p_type = segments[index].type;
			program.p_flags = segments[index].flags;
			program.p_offset = segments[index].offset;
			program.p_vaddr = segments[index].address;
			program.p_filesz = segments[index].file_size;
			program.p_memsz = segments[index].memory_size;
			memcpy(file + sizeof(header) + index * sizeof(program), &program, sizeof(program));
		}
	}
	else
	{
		Elf32_Ehdr header;

		memset(&header, 0, sizeof(header));
		memcpy(header.e_ident, ELFMAG, SELFMAG);
		header.e_ident[EI_CLASS] = ELFCLASS32;
		header.e_ident[EI_DATA] = ELFDATA2LSB;
		header.e_ident[EI_VERSION] = EV_CURRENT;
		header.e_type = ET_EXEC;
		header.e_machine = (Elf32_Half)machine;
		header.e_version = EV_CURRENT;
		header.e_phoff = sizeof(header);
		header.e_ehsize = sizeof(header);
		header.e_phentsize = sizeof(Elf32_Phdr);
		header.e_phnum = (Elf32_Half)count;
		memcpy(file, &header, sizeof(header));
		for (index = 0; index < count; index++)
		{
			Elf32_Phdr program;

			memset(&program, 0, sizeof(program));
			program.p_type = segments[index].type;
			program.p_flags = segments[index].flags;
			program.p_offset = (Elf32_Off)segments[index].offset;
			program.p_vaddr = (Elf32_Addr)segments[index].address;
			program.p_filesz = (Elf32_Word)segments[index].file_size;
			program.p_memsz = (Elf32_Word)segments[index].memory_size;
			memcpy(file + sizeof(header) + index * sizeof(program), &program, sizeof(program));
		}
	}
	return FILE_SIZE;
}

#define BASE HALO_GUEST_IMAGE_BASE

/* code at the base, data a page above it */
static const struct segment_spec good_segments[] =
{
	{ PT_LOAD, PF_R | PF_X, 0x1000, BASE, 0x1000, 0x1000 },
	{ PT_LOAD, PF_R | PF_W, 0x2000, BASE + 0x1000, 0x800, 0x4000 },
	{ PT_GNU_STACK, PF_R | PF_W, 0, 0, 0, 0 },
};

static int validate(int elf_class, int machine, const struct segment_spec *segments, int count,
	struct host_image_layout *layout)
{
	static unsigned char file[FILE_SIZE];
	size_t size = make_elf(file, elf_class, machine, segments, count);

	last_error[0] = 0;
	return host_image_validate(file, size, elf_class, machine, layout);
}

static void test_classes(void)
{
	static unsigned char file[FILE_SIZE];
	struct host_image_layout layout;
	size_t size;

	EXPECT(validate(ELFCLASS64, EM_AARCH64, good_segments, 3, &layout) == 0);
	EXPECT(layout.low == BASE && layout.high == BASE + 0x5000 && layout.segment_count == 2);
	EXPECT(validate(ELFCLASS32, EM_X86_64, good_segments, 3, &layout) == 0);
	EXPECT(layout.low == BASE && layout.high == BASE + 0x5000 && layout.segment_count == 2);
	EXPECT(layout.segments[1].memory_size == 0x4000 && layout.segments[1].offset == 0x2000);

	/* the wrong class or machine for the host */
	size = make_elf(file, ELFCLASS64, EM_AARCH64, good_segments, 3);
	EXPECT(host_image_validate(file, size, ELFCLASS32, EM_X86_64, &layout) != 0);
	size = make_elf(file, ELFCLASS32, EM_X86_64, good_segments, 3);
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
	size = make_elf(file, ELFCLASS32, EM_386, good_segments, 3);
	EXPECT(host_image_validate(file, size, ELFCLASS32, EM_X86_64, &layout) != 0);
	/* not ELF, too short, not an executable, big-endian */
	size = make_elf(file, ELFCLASS64, EM_AARCH64, good_segments, 3);
	EXPECT(host_image_validate(file, 10, ELFCLASS64, EM_AARCH64, &layout) != 0);
	EXPECT(host_image_validate(file, sizeof(Elf64_Ehdr) - 1, ELFCLASS64, EM_AARCH64, &layout) != 0);
	EXPECT(host_image_validate(NULL, 0, ELFCLASS64, EM_AARCH64, &layout) != 0);
	file[0] = 0;
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
	size = make_elf(file, ELFCLASS64, EM_AARCH64, good_segments, 3);
	((Elf64_Ehdr *)file)->e_type = ET_DYN;
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
	size = make_elf(file, ELFCLASS64, EM_AARCH64, good_segments, 3);
	file[EI_DATA] = ELFDATA2MSB;
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
}

static void test_headers_in_file(void)
{
	static unsigned char file[FILE_SIZE];
	struct host_image_layout layout;
	size_t size;

	/* program headers past the end of the file */
	size = make_elf(file, ELFCLASS64, EM_AARCH64, good_segments, 3);
	((Elf64_Ehdr *)file)->e_phoff = FILE_SIZE - sizeof(Elf64_Phdr);
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
	((Elf64_Ehdr *)file)->e_phoff = ~0ULL - 8;
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
	/* program headers of another size */
	size = make_elf(file, ELFCLASS32, EM_X86_64, good_segments, 3);
	((Elf32_Ehdr *)file)->e_phentsize = sizeof(Elf64_Phdr);
	EXPECT(host_image_validate(file, size, ELFCLASS32, EM_X86_64, &layout) != 0);
	/* none, or a great many */
	size = make_elf(file, ELFCLASS64, EM_AARCH64, good_segments, 0);
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
	size = make_elf(file, ELFCLASS64, EM_AARCH64, good_segments, 3);
	((Elf64_Ehdr *)file)->e_phnum = 0xffff;
	EXPECT(host_image_validate(file, size, ELFCLASS64, EM_AARCH64, &layout) != 0);
}

static void test_segments(void)
{
	struct segment_spec segments[3];
	struct host_image_layout layout;
	int elf_class;

	for (elf_class = ELFCLASS32; elf_class <= ELFCLASS64; elf_class++)
	{
		int machine = elf_class == ELFCLASS64 ? EM_AARCH64 : EM_X86_64;

		/* not static */
		memcpy(segments, good_segments, sizeof(segments));
		segments[2].type = PT_DYNAMIC;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		EXPECT(strstr(last_error, "PT_DYNAMIC") != NULL);
		segments[2].type = PT_INTERP;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		/* no loadable segment */
		EXPECT(validate(elf_class, machine, &good_segments[2], 1, &layout) != 0);
		/* the file's part past the end of the file, or overflowing */
		memcpy(segments, good_segments, sizeof(segments));
		segments[1].file_size = FILE_SIZE;
		segments[1].memory_size = FILE_SIZE;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		memcpy(segments, good_segments, sizeof(segments));
		segments[1].offset = FILE_SIZE + 1;
		segments[1].file_size = 0;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		memcpy(segments, good_segments, sizeof(segments));
		segments[1].offset = elf_class == ELFCLASS64 ? ~0ULL - 0x10 : 0xfffffff0u;
		segments[1].file_size = 0x20;
		segments[1].memory_size = 0x20;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		/* larger in the file than in memory */
		memcpy(segments, good_segments, sizeof(segments));
		segments[1].memory_size = 0x400;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		/* below the image's range, past its reserve, wrapping around */
		memcpy(segments, good_segments, sizeof(segments));
		segments[0].address = BASE - 0x1000;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		memcpy(segments, good_segments, sizeof(segments));
		segments[1].memory_size = HALO_GUEST_IMAGE_RESERVE;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		memcpy(segments, good_segments, sizeof(segments));
		segments[1].address = BASE + HALO_GUEST_IMAGE_RESERVE - 0x1000;
		segments[1].memory_size = 0x1000;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) == 0);
		segments[1].memory_size = 0x1001;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		if (elf_class == ELFCLASS64)
		{
			memcpy(segments, good_segments, sizeof(segments));
			segments[1].address = ~0ULL - 0x100;
			segments[1].memory_size = 0x1000;
			EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
		}
		/* not starting at the base */
		memcpy(segments, good_segments, sizeof(segments));
		segments[0].address = BASE + 0x1000;
		segments[1].address = BASE + 0x2000;
		EXPECT(validate(elf_class, machine, segments, 3, &layout) != 0);
	}
}

/* ---------- the header and the imports */

#define IMAGE_SIZE 0x5000

static uint8_t image[IMAGE_SIZE];

/* an image whose code (executable) is its first page, holding its header;
the import count, names and table in the data above */
static void make_image(struct host_image_layout *layout, uint32_t count)
{
	struct halo_guest_header header;
	uint32_t names = BASE + 0x1100;
	uint32_t index;

	EXPECT(validate(ELFCLASS64, EM_AARCH64, good_segments, 3, layout) == 0);
	memset(image, 0, sizeof(image));
	memset(&header, 0, sizeof(header));
	header.magic = HALO_GUEST_MAGIC;
	header.abi_version = HALO_GUEST_ABI_VERSION;
	header.image_end = BASE + IMAGE_SIZE;
	header.import_table = BASE + 0x2000;
	header.import_names = names;
	header.import_count = BASE + 0x1000;
	header.start = BASE + 0x100;
	header.thread_start = BASE + 0x200;
	header.thread_attach = BASE + 0x300;
	header.thread_detach = BASE + 0x400;
	header.init_array_start = BASE + 0x1800;
	header.init_array_end = BASE + 0x1808;
	memcpy(image, &header, sizeof(header));
	memcpy(image + 0x1000, &count, sizeof(count));
	for (index = 0; index < count; index++)
	{
		char name[32];

		snprintf(name, sizeof(name), "host_import_%u", index);
		memcpy(image + (names - BASE), name, strlen(name) + 1);
		names += (uint32_t)strlen(name) + 1;
	}
}

static struct halo_guest_header *image_header(void)
{
	return (struct halo_guest_header *)image;
}

static int validate_imports(const struct host_image_layout *layout, uint32_t *count)
{
	last_error[0] = 0;
	return host_image_validate_imports(image, BASE, BASE + IMAGE_SIZE, layout, count);
}

static void test_imports(void)
{
	struct host_image_layout layout;
	uint32_t count = 0;

	make_image(&layout, 3);
	EXPECT(validate_imports(&layout, &count) == 0);
	EXPECT(count == 3);
	make_image(&layout, 0);
	EXPECT(validate_imports(&layout, &count) == 0 && count == 0);

	/* the header */
	make_image(&layout, 3);
	image_header()->magic++;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->abi_version++;
	EXPECT(validate_imports(&layout, &count) != 0);
	EXPECT(host_image_validate_imports(image, BASE, BASE + 8, &layout, &count) != 0);
	/* an entry point outside the code (data, or outside the image) */
	make_image(&layout, 3);
	image_header()->start = BASE + 0x1800;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->thread_detach = 0x1000;
	EXPECT(validate_imports(&layout, &count) != 0);
	/* the initializers */
	make_image(&layout, 3);
	image_header()->init_array_end = BASE + IMAGE_SIZE + 4;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->init_array_end = BASE;
	EXPECT(validate_imports(&layout, &count) != 0);

	/* the count */
	make_image(&layout, 3);
	image_header()->import_count = BASE + IMAGE_SIZE - 2;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->import_count = 0x10;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->import_count = BASE + 0x1001;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	count = 0x7fffffff;
	memcpy(image + 0x1000, &count, sizeof(count));
	EXPECT(validate_imports(&layout, &count) != 0);

	/* the table: outside the image, too long for it, misaligned */
	make_image(&layout, 3);
	image_header()->import_table = 0x20000000;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->import_table = BASE + IMAGE_SIZE - 16;
	EXPECT(validate_imports(&layout, &count) != 0);
	image_header()->import_table = BASE + IMAGE_SIZE - 24;
	EXPECT(validate_imports(&layout, &count) == 0);
	image_header()->import_table = BASE + 0x2004;
	EXPECT(validate_imports(&layout, &count) != 0);

	/* the names: outside the image, unterminated, empty, fewer than the count */
	make_image(&layout, 3);
	image_header()->import_names = BASE + IMAGE_SIZE;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->import_names = BASE + IMAGE_SIZE - 4;
	memset(image + IMAGE_SIZE - 4, 'x', 4);
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	image_header()->import_names = BASE + 0x1500;
	EXPECT(validate_imports(&layout, &count) != 0);
	make_image(&layout, 3);
	count = 5000;
	memcpy(image + 0x1000, &count, sizeof(count));
	EXPECT(validate_imports(&layout, &count) != 0);
}

int main(void)
{
	test_classes();
	test_headers_in_file();
	test_segments();
	test_imports();
	if (failures)
		printf("%d failures\n", failures);
	else
		printf("host_loader: all passed\n");
	return failures != 0;
}
