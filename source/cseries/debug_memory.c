/*
DEBUG_MEMORY.C

symbols in this file:
0007CCC0 0030:
	_debug_memory_manager_initialize (0000)
0007CCF0 0050:
	_code_0007ccf0 (0000)
0007CD40 0050:
	_code_0007cd40 (0000)
0007CD90 0030:
	_code_0007cd90 (0000)
0007CDC0 0020:
	_code_0007cdc0 (0000)
0007CDE0 0050:
	_check_memory_status (0000)
0007CE30 0010:
	_local_random (0000)
0007CE40 0110:
	_code_0007ce40 (0000)
0007CF50 0070:
	_code_0007cf50 (0000)
0007CFC0 00a0:
	_code_0007cfc0 (0000)
0007D060 00e0:
	_code_0007d060 (0000)
0007D140 00b0:
	_debug_check_memory (0000)
0007D1F0 00e0:
	_debug_dump_memory_for_file (0000)
0007D2D0 0200:
	_debug_dump_memory_by_file (0000)
0007D4D0 0110:
	_debug_malloc (0000)
0007D5E0 00d0:
	_debug_free (0000)
0007D6B0 01a0:
	_debug_realloc (0000)
0007D850 0010:
	_debug_dump_memory (0000)
00257820 003c:
	??_C@_0DM@OPHCCCEJ@Debug?5memory?5manager?5is?5uninitia@ (0000)
0025785C 0026:
	??_C@_0CG@MJFPFHAG@c?3?2halo?2SOURCE?2cseries?2debug_mem@ (0000)
00257888 0052:
	??_C@_0FC@OHFGNKPI@Pointer?5allocated?5at?5?$CFs?0?5?$CFd?5has?5@ (0000)
002578E0 004c:
	??_C@_0EM@MBCHGNP@memory?5check?5failed?5at?5?$CFs?0?5diffe@ (0000)
00257930 0060:
	??_C@_0GA@PLIGMMOI@Invalid?5pointer?3?5header?3?50x?$CFx?5si@ (0000)
00257990 0017:
	??_C@_0BH@JLBIJIH@Checksum?5is?5incorrect?4?$AA@ (0000)
002579A8 0018:
	??_C@_0BI@HDCLMAAI@Signature?5is?5incorrect?4?$AA@ (0000)
002579C0 001b:
	??_C@_0BL@CLGIALFN@Pointer?5has?5been?5disposed?4?$AA@ (0000)
002579E0 0059:
	??_C@_0FJ@FAIAKDFC@Attempted?5an?5operation?5with?5poin@ (0000)
00257A40 005a:
	??_C@_0FK@OHOEBNNG@Attempted?5an?5operation?5with?5poin@ (0000)
00257A9C 0008:
	??_C@_07PFMCDPMA@pointer?$AA@ (0000)
00257AA4 0009:
	??_C@_08KCJKEBBA@previous?$AA@ (0000)
00257AB0 0020:
	??_C@_0CA@HBGNFHPA@?$AN?6Total?5Allocated?3?5?$CFd?5bytes?$AN?6?$AN?6?$AA@ (0000)
00257AD0 0020:
	??_C@_0CA@BMBACHBF@?$CF?540s?5?5?$CF?56d?5?$CF?510d?5?$CF?510d?5bytes?$AN?6?$AA@ (0000)
00257AF0 001a:
	??_C@_0BK@OKIDMCBD@?$CF?540s?5?5?$CF?56s?5?$CF?510s?5?$CF?510s?$AN?6?$AA@ (0000)
00257B0C 0005:
	??_C@_04HJPCFDOP@line?$AA@ (0000)
00257B14 0003:
	??_C@_02EGCJHIOB@id?$AA@ (0000)
00257B18 0005:
	??_C@_04IAGNFIBA@size?$AA@ (0000)
00257B20 0011:
	??_C@_0BB@NOMJNHJF@d?3?2heap_dump?4txt?$AA@ (0000)
00257B34 0004:
	??_C@_03DOPCBAKB@a?$CLb?$AA@ (0000)
00257B38 0049:
	??_C@_0EJ@PBGKLMDM@?$AN?6Total?3?5?$CF40d?5bytes?5in?5?$CF4d?5point@ (0000)
00257B84 003b:
	??_C@_0DL@LDKOLIEH@total_pointer_size?$DN?$DNdebug_memory@ (0000)
00257BC0 0047:
	??_C@_0EH@ELPDFPOK@File?3?5?$CF32s?5?$CF8d?5bytes?5in?5?$CF4d?5poin@ (0000)
00257C08 0027:
	??_C@_0CH@ICOHMPPJ@file_count?$DMMAXIMUM_FILES_WITH_PO@ (0000)
00257C30 0025:
	??_C@_0CF@FHDKMLKL@size?$DO?$DN0?5?$CG?$CG?5size?$DMMAXIMUM_POINTER_@ (0000)
00257C58 0010:
	??_C@_0BA@BEKDBLPJ@pointer?5?$HM?$HM?5size?$AA@ (0000)
002DCD0C 0020:
	_data_002dcd0c (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "errors.h"
#include "memory/crc.h"

/* ---------- constants */

enum
{
	debug_memory_signature = 0x53414654,
	debug_memory_allocated_signature = 0x2D2D2D3E,
	debug_memory_disposed_signature = 0x3C424144,
	debug_memory_trailing_signature = 0x3C2D2D2D,
	MAXIMUM_POINTER_SIZE = 0x10000000,
	MAXIMUM_FILES_WITH_POINTERS = 512
};

/* ---------- macros */

/* ---------- structures */

struct debug_memory_globals
{
	unsigned long signature;
	long total_pointer_size;
	long maximum_pointer_size;
	struct debug_memory_header *first_pointer;
	struct debug_memory_header *minimum_pointer;
	struct debug_memory_header *maximum_pointer;
	long next_allocation_id;
	unsigned long trailing_signature;
};

typedef char debug_memory_globals_size_must_be_0x20[
	sizeof(struct debug_memory_globals) == 0x20 ? 1 : -1];

struct debug_memory_header
{
	unsigned long signature;
	struct debug_memory_header *next;
	struct debug_memory_header *previous;
	unsigned long size;
	const char *file;
	long line;
	long allocation_id;
	unsigned long checksum;
};

typedef char debug_memory_header_size_must_be_0x20[
	sizeof(struct debug_memory_header) == 0x20 ? 1 : -1];
typedef char debug_memory_header_checksum_offset_must_be_0x1c[
	offsetof(struct debug_memory_header, checksum) == 0x1C ? 1 : -1];

struct memory_status
{
	unsigned long minimum_available_memory;
	unsigned long maximum_available_memory;
};

struct file_pointer_totals
{
	const char *file;
	long pointer_count;
	long minimum_size;
	long maximum_size;
	long total_size;
};

/* ---------- prototypes */

unsigned long *get_global_local_random_seed_address(
	void);
unsigned short seed_random(
	unsigned long *seed);
int code_0007cdc0(
	const void *a,
	const void *b);
static void code_0007ce40(
	struct debug_memory_header *header,
	const char *file,
	long line);
static void code_0007cd40(
	void *pointer,
	const char *file,
	long line);
static void code_0007cf50(
	void *pointer,
	unsigned long size);
static void code_0007cfc0(
	struct debug_memory_header *header);
static void code_0007d060(
	struct debug_memory_header *header,
	const char *file,
	long line);

/* ---------- globals */

struct debug_memory_globals data_002dcd0c =
{
	debug_memory_signature,
	0,
	0,
	NULL,
	NULL,
	NULL,
	0,
	debug_memory_signature
};

/* ---------- public code */

void debug_memory_manager_initialize(
	void)
{
	data_002dcd0c.signature = debug_memory_signature;
	data_002dcd0c.total_pointer_size = 0;
	data_002dcd0c.maximum_pointer_size = 0;
	data_002dcd0c.first_pointer = NULL;
	data_002dcd0c.minimum_pointer = NULL;
	data_002dcd0c.maximum_pointer = NULL;
	data_002dcd0c.trailing_signature = debug_memory_signature;

	return;
}

void code_0007ccf0(
	const char *file,
	long line)
{
	match_vassert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		145,
		data_002dcd0c.signature == debug_memory_signature &&
			data_002dcd0c.trailing_signature == debug_memory_signature,
		((char *(__cdecl *)(char *, ...))csprintf)(
			"Debug memory manager is uninitialized or corrupted. (%s:%d)",
			file,
			line));

	return;
}

void debug_dump_memory_for_file(
	const char *file)
{
	struct debug_memory_header *header;
	FILE *dump_file = NULL;
	long total_size = 0;

	header = data_002dcd0c.first_pointer;
	debug_check_memory("c:\\halo\\SOURCE\\cseries\\debug_memory.c", 513);

	for (;
		header != NULL;
		header = header->next)
	{
		if (!file || strstr(header->file, file))
		{
			if (dump_file == NULL)
			{
				dump_file = fopen("d:\\heap_dump.txt", "a+b");
				if (dump_file != NULL)
				{
					fprintf(
						dump_file,
						"% 40s  % 6s % 10s % 10s\r\n",
						"file",
						"line",
						"id",
						"size");
				}
			}
			if (dump_file != NULL)
			{
				fprintf(
					dump_file,
					"% 40s  % 6d % 10d % 10d bytes\r\n",
					header->file,
					header->line,
					header->allocation_id,
					header->size);
			}
			total_size += header->size;
		}
	}

	if (dump_file != NULL)
	{
		fprintf(
			dump_file,
			"\r\nTotal Allocated: %d bytes\r\n\r\n",
			total_size);
		fclose(dump_file);
	}

	return;
}

#define debug_memory_globals data_002dcd0c
#define current_heap_size total_pointer_size

void debug_dump_memory_by_file(
	void)
{
	struct file_pointer_totals files[MAXIMUM_FILES_WITH_POINTERS];
	short file_count = 0;
	long total_pointer_size = 0;
	long pointer_count = 0;
	struct debug_memory_header *header;
	short file_index;
	FILE *dump_file;
	short i;

	header = data_002dcd0c.first_pointer;
	debug_check_memory("c:\\halo\\SOURCE\\cseries\\debug_memory.c", 553);

	for (; header != NULL; header = header->next)
	{
		pointer_count++;
		total_pointer_size += header->size;

		for (file_index = 0; file_index < file_count; file_index++)
		{
			if (header->file == files[file_index].file)
			{
				files[file_index].pointer_count++;
				files[file_index].total_size += header->size;
				if (header->size < files[file_index].minimum_size)
				{
					files[file_index].minimum_size = header->size;
				}
				if (header->size > files[file_index].maximum_size)
				{
					files[file_index].maximum_size = header->size;
				}
				break;
			}
		}

		if (file_index == file_count)
		{
			match_assert(
				"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
				585,
				file_count<MAXIMUM_FILES_WITH_POINTERS);
			files[file_count].file = header->file;
			files[file_count].pointer_count = 1;
			files[file_count].total_size =
				files[file_count].minimum_size =
				files[file_count].maximum_size = header->size;
			file_count++;
		}
	}

	if (file_count != 0 &&
		(dump_file = fopen("d:\\heap_dump.txt", "a+b")) != NULL)
	{
		qsort(
			files,
			file_count,
			sizeof(struct file_pointer_totals),
			code_0007cdc0);
		for (i = 0; i < file_count; i++)
		{
			fprintf(
				dump_file,
				"File: %32s %8d bytes in %4d pointers. (Min: %8d Max: %8d Avg: %5.3f)\r\n",
				files[i].file,
				files[i].total_size,
				files[i].pointer_count,
				files[i].minimum_size,
				files[i].maximum_size,
				(double)files[i].total_size / files[i].pointer_count);
		}
		match_assert(
			"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
			612,
			total_pointer_size==debug_memory_globals.current_heap_size);
		fprintf(
			dump_file,
			"\r\nTotal: %40d bytes in %4d pointers\r\n\r\nLargest Heap Size: %28d bytes\r\n\r\n",
			total_pointer_size,
			pointer_count,
			data_002dcd0c.maximum_pointer_size);
		fclose(dump_file);
	}

	return;
}

#undef current_heap_size
#undef debug_memory_globals

void debug_dump_memory(
	void)
{
	debug_dump_memory_for_file(NULL);

	return;
}

void check_memory_status(
	struct memory_status *memory_status,
	const char *location)
{
	MEMORYSTATUS status;
	unsigned long difference;

	GlobalMemoryStatus(&status);
	memory_status->minimum_available_memory =
		status.dwAvailPhys > memory_status->minimum_available_memory
			? memory_status->minimum_available_memory
			: status.dwAvailPhys;
	memory_status->maximum_available_memory =
		status.dwAvailPhys <= memory_status->maximum_available_memory
			? memory_status->maximum_available_memory
			: status.dwAvailPhys;

	difference = memory_status->maximum_available_memory - memory_status->minimum_available_memory;
	if (difference > 16 * 1024)
	{
		error(
			_error_silent,
			"memory check failed at %s, difference between min and max memory free is %d",
			location,
			difference);
	}

	return;
}

void debug_check_memory(
	const char *file,
	long line)
{
	struct debug_memory_header *header;

	code_0007ccf0(file, line);
	header = data_002dcd0c.first_pointer;
	while (header != NULL)
	{
		code_0007ce40(header, file, line);
		match_vassert(
			"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
			199,
			*(unsigned long *)((byte *)(header + 1) + header->size) ==
				debug_memory_trailing_signature,
			csprintf(
				temporary,
				"Pointer allocated at %s, %d has overrun the end of its buffer. (Size: %d) (%s:%d)",
				header->file,
				header->line,
				header->size,
				file,
				line));
		header = header->next;
	}

	return;
}

void *debug_malloc(
	unsigned int size,
	boolean clear,
	const char *file,
	long line)
{
	unsigned long allocation_size =
		size + sizeof(struct debug_memory_header) + sizeof(unsigned long);
	struct debug_memory_header *header;
	void *pointer = NULL;

	match_assert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		214,
		size>=0 && size<MAXIMUM_POINTER_SIZE);
	code_0007ccf0(file, line);

	header = system_malloc(allocation_size);
	if (header != NULL)
	{
		header->file = file;
		header->signature = debug_memory_allocated_signature;
		header->line = line;
		header->allocation_id = data_002dcd0c.next_allocation_id++;
		header->size = size;
		*(unsigned long *)((byte *)(header + 1) + size) =
			debug_memory_trailing_signature;
		code_0007cfc0(header);

		pointer = header + 1;
		csmemset(pointer, clear ? 0 : 0xCA, size);
	}

	if (pointer != NULL)
	{
		data_002dcd0c.total_pointer_size += size;
		if (data_002dcd0c.total_pointer_size > data_002dcd0c.maximum_pointer_size)
		{
			data_002dcd0c.maximum_pointer_size = data_002dcd0c.total_pointer_size;
		}
	}

	return pointer;
}

void debug_free(
	void *pointer,
	const char *file,
	long line)
{
	struct debug_memory_header *header =
		(struct debug_memory_header *)pointer - 1;

	code_0007ccf0(file, line);
	code_0007ce40(header, file, line);
	code_0007cd40(pointer, file, line);

	data_002dcd0c.total_pointer_size -= header->size;
	code_0007d060(header, file, line);
	header->signature = debug_memory_disposed_signature;
	system_free(header);

	return;
}

void *debug_realloc(
	void *pointer,
	unsigned int size,
	const char *file,
	long line)
{
	void *result = NULL;
	unsigned long old_size = 0;
	unsigned long allocation_size =
		size + sizeof(struct debug_memory_header) + sizeof(unsigned long);
	struct debug_memory_header *header = NULL;
	const char *allocation_file = file;
	long allocation_line = line;

	match_assert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		337,
		pointer || size);
	match_assert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		338,
		size>=0 && size<MAXIMUM_POINTER_SIZE);
	code_0007ccf0(file, line);

	if (pointer != NULL)
	{
		header = (struct debug_memory_header *)pointer - 1;
		code_0007ce40(header, file, line);
		code_0007cd40(pointer, file, line);
		code_0007d060(header, file, line);

		allocation_line = header->line;
		old_size = header->size;
		allocation_file = header->file;
		header->signature = debug_memory_disposed_signature;
	}

	header = system_realloc(
		header,
		pointer != NULL && size == 0 ? 0 : allocation_size);
	if (header != NULL)
	{
		header->signature = debug_memory_allocated_signature;
		header->line = allocation_line;
		header->file = allocation_file;
		header->allocation_id = data_002dcd0c.next_allocation_id++;
		header->size = size;
		*(unsigned long *)((byte *)(header + 1) + size) =
			debug_memory_trailing_signature;
		code_0007cfc0(header);

		result = header + 1;
		if (size > old_size)
		{
			code_0007cf50(
				(byte *)result + old_size,
				size - old_size);
		}
	}

	if (size != 0)
	{
		data_002dcd0c.total_pointer_size += size - old_size;
		if (data_002dcd0c.total_pointer_size > data_002dcd0c.maximum_pointer_size)
		{
			data_002dcd0c.maximum_pointer_size = data_002dcd0c.total_pointer_size;
		}
	}

	return result;
}

/* ---------- private code */

unsigned short local_random(
	void)
{
	return seed_random(get_global_local_random_seed_address());
}

unsigned long code_0007cd90(
	struct debug_memory_header const *header)
{
	unsigned long checksum;

	crc_new(&checksum);
	crc_checksum_buffer(&checksum, header, offsetof(struct debug_memory_header, checksum));

	return checksum;
}

int code_0007cdc0(
	const void *a,
	const void *b)
{
	const struct file_pointer_totals *file_a = a;
	const struct file_pointer_totals *file_b = b;

	return file_b->total_size - file_a->total_size;
}

static void code_0007ce40(
	struct debug_memory_header *header,
	const char *file,
	long line)
{
	const char *reason = NULL;
	unsigned long checksum;

	match_vassert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		160,
		data_002dcd0c.first_pointer != NULL,
		csprintf(
			temporary,
			"Attempted an operation with pointer at 0x%x when no pointers have been allocated. (%s:%d)",
			header + 1,
			file,
			line));
	match_vassert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		165,
		header >= data_002dcd0c.minimum_pointer &&
			header <= data_002dcd0c.maximum_pointer,
		csprintf(
			temporary,
			"Attempted an operation with pointer at 0x%x, outside of the valid pointer range. (%s:%d)",
			header + 1,
			file,
			line));

	if (header->signature == debug_memory_disposed_signature)
	{
		reason = "Pointer has been disposed.";
	}
	else if (header->signature != debug_memory_allocated_signature)
	{
		reason = "Signature is incorrect.";
	}
	else
	{
		crc_new(&checksum);
		crc_checksum_buffer(
			&checksum,
			header,
			offsetof(struct debug_memory_header, checksum));
		if (checksum != header->checksum)
		{
			reason = "Checksum is incorrect.";
		}
	}

	match_vassert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		183,
		reason == NULL,
		csprintf(
			temporary,
			"Invalid pointer: header: 0x%x signature: 0x%x line: %d file: 0x%x size: 0x%x reason: %s (%s:%d)",
			header,
			header->signature,
			header->line,
			header->file,
			header->size,
			reason,
			file,
			line));

	return;
}

static void code_0007cd40(
	void *pointer,
	const char *file,
	long line)
{
	struct debug_memory_header *header = (struct debug_memory_header *)pointer - 1;

	match_vassert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		199,
		*(unsigned long *)((byte *)(header + 1) + header->size) ==
			debug_memory_trailing_signature,
		csprintf(
			temporary,
			"Pointer allocated at %s, %d has overrun the end of its buffer. (Size: %d) (%s:%d)",
			header->file,
			header->line,
			header->size,
			file,
			line));

	return;
}

static void code_0007cf50(
	void *pointer,
	unsigned long size)
{
	byte *current = pointer;
	byte *end;

	match_assert(
		"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
		269,
		pointer);

	end = current + size - 1;
	while (current < end)
	{
		*(unsigned short *)current = local_random();
		current += sizeof(unsigned short);
	}
	if (size & 1)
	{
		*end = (byte)local_random();
	}

	return;
}

static void code_0007cfc0(
	struct debug_memory_header *header)
{
	if (data_002dcd0c.first_pointer == NULL ||
		header < data_002dcd0c.minimum_pointer)
	{
		data_002dcd0c.minimum_pointer = header;
	}
	if (data_002dcd0c.first_pointer == NULL ||
		header > data_002dcd0c.maximum_pointer)
	{
		data_002dcd0c.maximum_pointer = header;
	}

	header->next = data_002dcd0c.first_pointer;
	if (data_002dcd0c.first_pointer != NULL)
	{
		data_002dcd0c.first_pointer->previous = header;
		header->next->checksum = code_0007cd90(header->next);
	}
	header->previous = NULL;
	data_002dcd0c.first_pointer = header;
	header->checksum = code_0007cd90(header);

	return;
}

static void code_0007d060(
	struct debug_memory_header *header,
	const char *file,
	long line)
{
	struct debug_memory_header *previous;
	struct debug_memory_header *next;

	if (header == data_002dcd0c.first_pointer)
	{
		data_002dcd0c.first_pointer = header->next;
		if (data_002dcd0c.first_pointer != NULL)
		{
			data_002dcd0c.first_pointer->previous = NULL;
			data_002dcd0c.first_pointer->checksum =
				code_0007cd90(data_002dcd0c.first_pointer);
		}
	}
	else
	{
		previous = header->previous;
		match_assert(
			"c:\\halo\\SOURCE\\cseries\\debug_memory.c",
			446,
			previous);
		next = header->next;
		previous->next = next;
		previous->checksum = code_0007cd90(previous);
		if (next != NULL)
		{
			next->previous = previous;
			next->checksum = code_0007cd90(next);
		}
	}

	return;
}
