/* ---------- headers */

#include "cseries.h"
#define NOD3D
#define NODSOUND
#include "cseries_windows.h"
#include "errors.h"

#include <ctype.h>

/* January calls the CRT isspace function rather than the multibyte ctype macro. */
#undef isspace

/* ---------- constants */

enum
{
	MAXIMUM_DEBUG_SYMBOL_NAME_LENGTH = 256,
	MAXIMUM_LIBRARY_OBJECT_FILE_NAME_LENGTH = 256,
	DEBUG_SYMBOL_ALLOCATION_COUNT = 4096,
	DEBUG_SYMBOL_STRING_STORAGE_ALLOCATION_SIZE = 0x4000
};

/* ---------- macros */

/* ---------- structures */

struct debug_symbol_table
{
	long number_of_symbols;
	char *string_storage;
	struct debug_symbol *symbols;
};

struct debug_symbol
{
	unsigned long address;
	unsigned long rva_base;
	unsigned long name_string_offset;
	unsigned long library_object_string_offset;
};

struct _stack_walk_globals
{
	long fixup;
	boolean disregard_symbol_names;
	struct debug_symbol_table symbol_table;
};

/* ---------- prototypes */

static boolean is_valid_ebp(
	void);

static unsigned long walk_up(
	void);

static void initialize_stack_walk(
	CONTEXT *context);

static void walk_stack_context(
	unsigned long *routine_addresses,
	unsigned long number_of_levels,
	unsigned long ignore_levels,
	unsigned long *levels_dumped);

static void walk_stack(
	unsigned long *routine_addresses,
	unsigned long number_of_levels,
	unsigned long ignore_levels,
	unsigned long *levels_dumped);

static int symbol_sort_proc(
	const void *elem1,
	const void *elem2);

/* ---------- globals */

static struct _stack_walk_globals stack_walk_globals =
{
	NONE,
	FALSE
};

static unsigned long *old_ebp;
static unsigned long walk_up_current_frame;

/* ---------- public code */

long stack_walk_global_function_offset(
	void)
{
	return stack_walk_globals.fixup==NONE ? 0 : stack_walk_globals.fixup;
}

void stack_walk_disregard_symbol_names(
	boolean disregard)
{
	stack_walk_globals.disregard_symbol_names = disregard;
	return;
}

void stack_walk(
	short levels_to_ignore)
{
	stack_walk_with_context(
		NULL,
		levels_to_ignore + 1,
		NULL);

	return;
}

void free_symbol_table(
	struct debug_symbol_table *symbol_table)
{
	match_assert("c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c", 549, symbol_table);

	if (symbol_table->string_storage)
	{
		debug_free(symbol_table->string_storage, "c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c", 551);
	}

	if (symbol_table->symbols)
	{
		debug_free(symbol_table->symbols, "c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c", 552);
	}

	symbol_table->number_of_symbols = 0;
	symbol_table->string_storage = NULL;
	symbol_table->symbols = NULL;
	return;
}

char *symbol_name_from_address(
	unsigned long fake_address,
	struct debug_symbol_table *symbol_table)
{
	static char symbol_buffer[0x4000] = { 0 };
	unsigned long address = stack_walk_globals.fixup + fake_address;

	csstrcpy(symbol_buffer, "<unknown>");
	if (symbol_table->number_of_symbols > 0)
	{
		if (address >= symbol_table->symbols[0].rva_base && address < symbol_table->symbols[symbol_table->number_of_symbols - 1].rva_base + 0xFFFF)
		{
			long symbol_index = 1;

			if (symbol_table->number_of_symbols > 1)
			{
				while (symbol_table->symbols[symbol_index - 1].rva_base > address || address >= symbol_table->symbols[symbol_index].rva_base)
				{
					symbol_index++;
					if (symbol_index >= symbol_table->number_of_symbols)
					{
						return symbol_buffer;
					}
				}

				_snprintf(
					symbol_buffer,
					0x3FFF,
					"%s + %04lX : %s",
					symbol_table->string_storage + symbol_table->symbols[symbol_index - 1].name_string_offset,
					address - symbol_table->symbols[symbol_index - 1].rva_base,
					symbol_table->string_storage + symbol_table->symbols[symbol_index - 1].library_object_string_offset);
			}
		}
	}

	return symbol_buffer;
}

long base_address_from_symbol_name(
	char const *name,
	struct debug_symbol_table *symbol_table)
{
	long base_address = NONE;
	long symbol_index;
	char const *symbol_name;

	for (symbol_index = 1; symbol_index < symbol_table->number_of_symbols; symbol_index++)
	{
		symbol_name = symbol_table->string_storage + symbol_table->symbols[symbol_index].name_string_offset;

		if (csstrcmp(name, symbol_name)==0)
		{
			base_address = symbol_table->symbols[symbol_index].rva_base;
		}
	}

	return base_address;
}

void stack_walk_dispose(
	void)
{
	stack_walk_globals.fixup = NONE;
	stack_walk_globals.disregard_symbol_names = FALSE;
	free_symbol_table(&stack_walk_globals.symbol_table);
	return;
}

void stack_walk_with_context(
	FILE *error_stream,
	short levels_to_ignore,
	CONTEXT *context_pointer)
{
	unsigned long routine_addresses[64] = { 0 };
	unsigned long levels_dumped;
	long frame_number;

	if (context_pointer)
	{
		initialize_stack_walk(context_pointer);
		walk_stack_context(
			routine_addresses,
			NUMBEROF(routine_addresses),
			levels_to_ignore,
			&levels_dumped);
	}
	else
	{
		walk_stack(
			routine_addresses,
			NUMBEROF(routine_addresses),
			levels_to_ignore,
			&levels_dumped);
	}

	if (!error_stream)
	{
		error(_error_silent, "Printing stuff for Mat's edification");

		for (frame_number = levels_dumped - 1; frame_number >= levels_to_ignore; frame_number--)
		{
#ifdef HALO_ARM64_GUEST
			/* the call site (the BL before the return address), for
			llvm-symbolizer --obj=build/android/halo_guest.elf */
			unsigned long routine_address = routine_addresses[frame_number] - 4;
#else
			unsigned long routine_address = routine_addresses[frame_number] + *(long *)(routine_addresses[frame_number] - sizeof(long));
#endif
			char const *symbol_name;

			if (stack_walk_globals.symbol_table.number_of_symbols && !stack_walk_globals.disregard_symbol_names)
			{
				symbol_name = symbol_name_from_address(routine_address, &stack_walk_globals.symbol_table);
			}
			else
			{
				symbol_name = "?????";
			}

			error(_error_silent, "%08lX %s", routine_address, symbol_name);
		}
	}

	if (context_pointer)
	{
		unsigned long instruction = *(unsigned long *)context_pointer->Eip;
		unsigned long instruction_byte0 = instruction & 0xFF;
		unsigned long instruction_byte1 = (instruction >> 8) & 0xFF;
		unsigned long instruction_byte2 = (instruction >> 16) & 0xFF;
		unsigned long instruction_byte3 = (instruction >> 24) & 0xFF;
		char const *symbol_name;

		error(_error_silent, "EAX: 0x%08lX", context_pointer->Eax);
		error(_error_silent, "EBX: 0x%08lX", context_pointer->Ebx);
		error(_error_silent, "ECX: 0x%08lX", context_pointer->Ecx);
		error(_error_silent, "EDX: 0x%08lX", context_pointer->Edx);
		error(_error_silent, "EDI: 0x%08lX", context_pointer->Edi);
		error(_error_silent, "ESI: 0x%08lX", context_pointer->Esi);
		error(_error_silent, "EBP: 0x%08lX", context_pointer->Ebp);
		error(_error_silent, "ESP: 0x%08lX", context_pointer->Esp);

		if (stack_walk_globals.symbol_table.number_of_symbols && !stack_walk_globals.disregard_symbol_names)
		{
			symbol_name = symbol_name_from_address(context_pointer->Eip, &stack_walk_globals.symbol_table);
		}
		else
		{
			symbol_name = "?????";
		}

		error(
			_error_silent,
			"EIP: 0x%08lX, %02lX %02lX %02lX %02lX %s",
			context_pointer->Eip,
			instruction_byte0,
			instruction_byte1,
			instruction_byte2,
			instruction_byte3,
			symbol_name);
	}

	for (frame_number = levels_dumped - 1; frame_number >= levels_to_ignore; frame_number--)
	{
		if (!error_stream)
		{
			error(
				_error_silent,
				"%08lX %s",
				routine_addresses[frame_number],
				stack_walk_globals.symbol_table.number_of_symbols && !stack_walk_globals.disregard_symbol_names
					? symbol_name_from_address(routine_addresses[frame_number], &stack_walk_globals.symbol_table)
					: "?????");
		}
		else
		{
			fprintf(
				error_stream,
				"%08lX %s\n",
				routine_addresses[frame_number],
				stack_walk_globals.symbol_table.number_of_symbols && !stack_walk_globals.disregard_symbol_names
					? symbol_name_from_address(routine_addresses[frame_number], &stack_walk_globals.symbol_table)
					: "?????");
		}
	}

	return;
}

int load_symbol_table(
	char *filename,
	struct debug_symbol_table *symbol_table,
	char *timestamp_str)
{
	FILE *map_file;
	unsigned long string_storage_size;
	unsigned long string_storage_used;
	unsigned long symbols_size;
	long previous_library_object_offset;
	unsigned long symbol_address;
	unsigned long rva_base;
	char *segment;
	char *token;
	char *end_str;
	char symbol_name[MAXIMUM_DEBUG_SYMBOL_NAME_LENGTH];
	char library_object_file_name[MAXIMUM_LIBRARY_OBJECT_FILE_NAME_LENGTH];
	char last_object_file_name[MAXIMUM_LIBRARY_OBJECT_FILE_NAME_LENGTH];

	match_assert("c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c", 256, symbol_table);
	csmemset(symbol_table, 0, sizeof(*symbol_table));

	map_file = fopen(filename, "r");
	if (!map_file)
	{
		error(_error_silent, "Couldn't read map file '%s'", filename);
		goto finished;
	}

	{
		char line[DEBUG_SYMBOL_STRING_STORAGE_ALLOCATION_SIZE] = "";
		boolean found_symbols_section = FALSE;

		if (!fgets(line, sizeof(line), map_file))
		{
			goto close_map_file;
		}

		while (!found_symbols_section)
		{
			if (!fgets(line, sizeof(line), map_file))
			{
				error(_error_silent, "map file appears corrupt");
				goto close_map_file;
			}

			if (strstr(line, "Lib:Object"))
			{
				found_symbols_section = TRUE;
			}
		}

		string_storage_size = 0;
		string_storage_used = 0;
		symbols_size = 0;
		strcpy(last_object_file_name, "nothing");
		previous_library_object_offset = NONE;

		while (fgets(line, sizeof(line), map_file))
		{
			end_str = NULL;
			segment = strtok(line, ":");
			if (!segment || *segment!=' ')
			{
				continue;
			}

			token = strtok(NULL, " \t\n\r");
			if (!token)
			{
				goto corrupt_map_file;
			}
			symbol_address = strtoul(token, &end_str, 16);

			token = strtok(NULL, " \t\n\r");
			if (token)
			{
				strncpy(symbol_name, token, sizeof(symbol_name)-1);
				symbol_name[sizeof(symbol_name)-1] = 0;
			}
			else
			{
				if (!strstr(line, "entry point at"))
				{
					goto corrupt_map_file;
				}

				if (!fgets(line, sizeof(line), map_file) || !isspace(line[0]) ||
					!fgets(line, sizeof(line), map_file) || !strstr(line, "Static symbols") ||
					!fgets(line, sizeof(line), map_file) || !isspace(line[0]))
				{
					goto corrupt_map_file;
				}

				fgets(line, sizeof(line), map_file);
				segment = strtok(line, ":");
				if (!segment || *segment!=' ')
				{
					goto corrupt_map_file;
				}

				token = strtok(NULL, " \t\n\r");
				if (!token)
				{
					goto corrupt_map_file;
				}
				symbol_address = strtoul(token, &end_str, 16);

				token = strtok(NULL, " \t\n\r");
				if (!token)
				{
					goto corrupt_map_file;
				}
				strncpy(symbol_name, token, sizeof(symbol_name)-1);
				symbol_name[sizeof(symbol_name)-1] = 0;
			}

			token = strtok(NULL, " \t\n\r");
			if (!token)
			{
				goto corrupt_map_file;
			}
			rva_base = strtoul(token, &end_str, 16);

			if (strcmp(symbol_name, "_load_symbol_table")==0)
			{
				stack_walk_globals.fixup = rva_base - (unsigned long)load_symbol_table;
			}

			/* (the address's five columns, " f   ", before the object name) */
			if (!end_str || strlen(end_str) < 5)
			{
				goto corrupt_map_file;
			}
			end_str += 5;

			token = strtok(end_str, " \t\n\r");
			if (!token)
			{
				goto corrupt_map_file;
			}
			strncpy(library_object_file_name, token, sizeof(library_object_file_name)-1);
			library_object_file_name[sizeof(library_object_file_name)-1] = 0;

			if ((unsigned long)symbol_table->number_of_symbols >= symbols_size)
			{
				struct debug_symbol *new_symbols;

				symbols_size += DEBUG_SYMBOL_ALLOCATION_COUNT;
				new_symbols = match_realloc(
					"c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c",
					454,
					symbol_table->symbols,
					symbols_size * sizeof(*symbol_table->symbols));
				if (!new_symbols)
				{
					goto allocation_failed;
				}
				symbol_table->symbols = new_symbols;
			}

			if (string_storage_used + strlen(symbol_name) + 1 + strlen(library_object_file_name) + 1 >= string_storage_size)
			{
				char *new_string_storage;

				string_storage_size += DEBUG_SYMBOL_STRING_STORAGE_ALLOCATION_SIZE;
				new_string_storage = match_realloc(
					"c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c",
					471,
					symbol_table->string_storage,
					string_storage_size);
				if (!new_string_storage)
				{
					goto allocation_failed;
				}
				symbol_table->string_storage = new_string_storage;

				match_assert(
					"c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c",
					482,
					string_storage_used + strlen(symbol_name) + 1 + strlen(library_object_file_name) + 1 < string_storage_size);
			}

			{
				struct debug_symbol *new_symbol = &symbol_table->symbols[symbol_table->number_of_symbols++];

				new_symbol->address = symbol_address;
				new_symbol->rva_base = rva_base;

				match_assert(
					"c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c",
					489,
					string_storage_used + strlen(symbol_name) + 1 < string_storage_size);
				strcpy(symbol_table->string_storage + string_storage_used, symbol_name);
				new_symbol->name_string_offset = string_storage_used;
				string_storage_used += strlen(symbol_name) + 1;

				if (strcmp(last_object_file_name, library_object_file_name)==0)
				{
					new_symbol->library_object_string_offset = previous_library_object_offset;
				}
				else
				{
					match_assert(
						"c:\\halo\\SOURCE\\cseries\\stack_walk_windows.c",
						501,
						string_storage_used + strlen(library_object_file_name) + 1 < string_storage_size);
					strcpy(symbol_table->string_storage + string_storage_used, library_object_file_name);
					new_symbol->library_object_string_offset = string_storage_used;
					string_storage_used += strlen(library_object_file_name) + 1;
					previous_library_object_offset = new_symbol->library_object_string_offset;
					strcpy(last_object_file_name, library_object_file_name);
				}
			}
		}

		goto close_map_file;

allocation_failed:
		error(_error_silent, "could not allocate enough memory for map file");
		free_symbol_table(symbol_table);
		goto close_map_file;

corrupt_map_file:
		error(_error_silent, "map file appears corrupt");
		free_symbol_table(symbol_table);

close_map_file:
		fclose(map_file);
	}

finished:
	if (symbol_table->number_of_symbols > 0)
	{
		qsort(
			symbol_table->symbols,
			symbol_table->number_of_symbols,
			sizeof(*symbol_table->symbols),
			symbol_sort_proc);

		while (symbol_table->number_of_symbols > 0 &&
			symbol_table->symbols[symbol_table->number_of_symbols-1].rva_base==0)
		{
			symbol_table->number_of_symbols--;
		}
	}

	return symbol_table->number_of_symbols > 0;
}

void stack_walk_initialize(
	void)
{
	load_symbol_table(
		"d:\\cachebeta.map",
		&stack_walk_globals.symbol_table,
		"Mon Dec 17 12:49:36 2001");

	if (stack_walk_globals.fixup==NONE)
	{
		stack_walk_globals.fixup = 0;
	}

	return;
}

/* ---------- private code */

static int symbol_sort_proc(
	const void *elem1,
	const void *elem2)
{
	const struct debug_symbol *symbol1 = elem1;
	const struct debug_symbol *symbol2 = elem2;

	if (symbol1->rva_base == symbol2->rva_base)
	{
		return 0;
	}
	if (symbol1->rva_base==0 || symbol1->rva_base > symbol2->rva_base)
	{
		return 1;
	}

	if (symbol2->rva_base==0 || symbol1->rva_base < symbol2->rva_base)
	{
		return NONE;
	}

	return 0;
}

static boolean is_valid_ebp(
	void)
{
	return 0==(walk_up_current_frame & (sizeof(unsigned long) - 1)) && walk_up_current_frame >= (unsigned long)old_ebp;
}

static unsigned long walk_up(
	void)
{
	unsigned long routine_address = 0;

	if (walk_up_current_frame)
	{
#ifdef HALO_ARM64_GUEST
		/* an AArch64 frame record: the caller's frame pointer, then the
		return address, 8 bytes each (the upper halves are zero) */
		routine_address = ((unsigned long *)walk_up_current_frame)[2];
#else
		routine_address = ((unsigned long *)walk_up_current_frame)[1];
#endif
		walk_up_current_frame = ((unsigned long *)walk_up_current_frame)[0];
		if (!is_valid_ebp())
		{
			walk_up_current_frame = 0;
		}

		old_ebp = (unsigned long *)walk_up_current_frame;
	}

	return routine_address;
}

static void initialize_stack_walk(
	CONTEXT *context)
{
	old_ebp = (unsigned long *)context->Esp;
	walk_up_current_frame = context->Ebp;
	return;
}

static void walk_stack_context(
	unsigned long *routine_addresses,
	unsigned long number_of_levels,
	unsigned long ignore_levels,
	unsigned long *levels_dumped)
{
	unsigned long level;

	if (!is_valid_ebp())
	{
		walk_up_current_frame = 0;
	}

	if (ignore_levels)
	{
		while (--ignore_levels)
		{
			walk_up();
		}
	}

	for (level = 0; level < number_of_levels; level++)
	{
		routine_addresses[level] = walk_up();
		if (!routine_addresses[level])
		{
			break;
		}
	}

	*levels_dumped = level;
	return;
}

static void walk_stack(
	unsigned long *routine_addresses,
	unsigned long number_of_levels,
	unsigned long ignore_levels,
	unsigned long *levels_dumped)
{
	unsigned long level;

	walk_up_current_frame = (unsigned long)__builtin_frame_address(0);
	old_ebp = (unsigned long *)walk_up_current_frame;

	if (!is_valid_ebp())
	{
		walk_up_current_frame = 0;
	}

	if (ignore_levels)
	{
		while (--ignore_levels)
		{
			walk_up();
		}
	}

	for (level = 0; level < number_of_levels; level++)
	{
		routine_addresses[level] = walk_up();
		if (!routine_addresses[level])
		{
			break;
		}
	}

	*levels_dumped = level;
	return;
}
