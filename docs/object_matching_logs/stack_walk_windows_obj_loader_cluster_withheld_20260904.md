# StackWalk loader cluster: preserved, not admitted (2026-09-04)

## Canonical decision

This is versioned research, not a production implementation or byte-credit
claim. Canonical remains at seven exact StackWalk functions. The complete
source is preserved below so the genuine loader/comparator reconstruction is
not dependent on ignored scratch files.

Root independently read the complete January loader and comparator instruction
streams, the HCEA binary-derived C reconstruction, and the shipped HCEA PDB
function/local records. The target proves the unusual legacy behavior; the
comments record its defects without pretending they are safe on malformed map
files. The final owner census received a separate independent review.

The required public declaration belongs in the existing
`source/cseries/cseries.h` STACK_WALK_WINDOWS.C owner section:

```c
struct debug_symbol_table;
int load_symbol_table(
	char *filename,
	struct debug_symbol_table *symbol_table,
	char *timestamp_str);
```

The initially attempted broad cleanup of inherited StackWalk APIs lost three
exact owners: Decals (400 padded bytes), Game Engine (560), and UI (704).
With that optional cleanup withdrawn, even the minimal new-loader-only API
above lost these four unrelated exact owners:

| Object / function | Padded bytes |
| --- | ---: |
| decals / decals_delete_permanent_from_cluster | 400 |
| game_engine_race / race_touch_flag | 336 |
| game_engine_race / race_engine_player_update | 160 |
| ui_widget_game_data_input_functions / solo_level_select_list_update_displayed_items | 704 |

Both builds also failed the Shell semantic-data identity check because the
expected anonymous `$T18271` owner changed. The broad build also made the
existing Bitmap 3D smoothing park stale. None of those measurements or
allowlists was refreshed to hide the change. No unrelated source was modified.

Snapshots are `scratch/stack-walk-owner-header-only-20260904.json` and
`scratch/stack-walk-loader-minimal-header-20260904.json`. After restoring all
four touched production source/header files, full Ninja passes and
`scratch/stack-walk-header-withheld-restored-20260904.json` matches the
published AI-race baseline: 6,199 strict owners, no gains or regressions.
The protected Bitmap source hashes are unchanged.

No fake export, helper anchor, substitute header, declaration-order search,
or local-public-prototype workaround is accepted. No park/config/target
metadata is changed for this withheld packet. Reopen only with new authentic
owner/compiler context that preserves the inherited exact set.

The final saved artifact has exactly the two intended new code owners and
fifteen ordinary string COMDATs, no COMMON or extra helper, and unchanged
inherited code/data owners. Fourteen strings are target-owned; the remaining
`"r"` literal is exactly compatible with January's selected
`source/bitmaps/tiff_file.obj` owner: external/type 0, bytes `72 00`,
size 2, zero relocations, flags `0x40301040`, select-any, alignment 4.
This duplicate is not the admission blocker.

## Reconstruction evidence

## Scope and admission boundary

This is a scratch-only reconstruction of the genuine caller cluster formed by
`load_symbol_table` (January `_load_symbol_table`, 1568 padded bytes) and its
private retained qsort comparator (January `_code_000806d0`, 48 padded bytes).
No production source, shared header, target metadata, canonical build output,
or park entry is changed by this work.  The private helper must not be emitted
alone: January has a real call relocation to it from the loader, and the
reconstruction retains that same natural caller relationship.

## Authenticated ABI and layout

The existing `source/cseries/stack_walk_windows.c` definitions agree with the
recovered HCEA Release PDB:

- `debug_symbol` is 16 bytes: `address`, `rva_base`,
  `name_string_offset`, and `library_object_string_offset`, all unsigned longs
  at offsets `0x0`, `0x4`, `0x8`, and `0xC`.
- `debug_symbol_table` is 12 bytes: signed-long `number_of_symbols`, then the
  string-storage and symbol pointers at `0x4` and `0x8`.
- the loader has three pointer parameters (filename, table, timestamp string),
  four authenticated local arrays (`line[0x4000]` and three `char[0x100]`
  buffers), a separate `char *` end pointer, and a frame size near `0x4400`
  in the later build.
- `symbol_sort_proc` is a source-private `int (const void *, const void *)`
  qsort callback.  January independently compares the `rva_base` field and
  sorts zero-RVA sentinel records last.

The HCEA repository's binary-derived reconstruction is
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research_cache\halocea\src\blam\bitmaps\load_symbol_table.c`.
It is corroborating reconstruction evidence, **not original source**.
January COFF is the byte and behavioral authority, while the shipped HCEA
Release PDB is authoritative only for its own later-build names/types.  The
January target confirms the parser's file/CRT topology,
`_load_symbol_table` self-name fixup, 4096-record and `0x4000`-byte allocation
increments, consecutive library/object string deduplication, qsort call, and
trailing zero-RVA trimming.

Exact evidence paths and bounded PDB commands:

- January COFF:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\rasterizer-model-wrapper-alias-20260829\build\split\source\cseries\stack_walk_windows.obj`
- HCEA PDB:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX_Release.pdb`
- DIA reader:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe`
- commands: `DIA2Dump.exe -sym load_symbol_table <PDB>` and
  `DIA2Dump.exe -sym symbol_sort_proc <PDB>`.

The loader dump reports length `0x63C`, frame `0x4400`, parameters
`char *filename`, `debug_symbol_table *symbol_table`, `char *timestamp_str`,
and locals `line char[0x4000]`, `last_object_file_name char[0x100]`,
`end_str char *`, `library_object_file_name char[0x100]`, and
`symbol_name char[0x100]`.  The comparator dump spells
`static int symbol_sort_proc(const void *, const void *)`, parameters `elem1`
and `elem2`, length `0x40`, no inline, no inline assembly, and a zero-byte
frame.  January independently supplies the smaller 48-byte comparator body
and its exact field/control behavior.

## Required metadata proposal if later admitted

- Rename target `_code_000806d0` at RVA `0x000806D0` to
  `symbol_sort_proc` and mark it source-private.
- Correct `load_symbol_table`'s owning declaration from `void` to `int` and
  retain the PDB-authenticated `char *`, table pointer, `char *` parameter
  types.  The function returns whether any symbols remain after
  sorting/trimming; the later shipped PDB and HCEA reconstruction both
  spell that return type as `int`.
- No standalone helper anchor, forced linkage, `inline` control, or config
  exception is permitted.

## Bounded measurement

Untouched baseline artifact:
`scratch/stack-walk-loader-baseline-20260904.obj`, SHA-256
`6B9EB8F979E7A09EEFA1AA27EBE0236B40CB7ABE5BD4D18904F5F325068E69AA`.
The baseline is 7 exact / 0 residual / 9 unwritten.

The first complete natural body was 1664 bytes / 101 relocations.  Three real
API/topology errors were then corrected from January evidence, rather than by
instruction or declaration search:

- `<ctype.h>`'s multibyte `isspace` macro had emitted `_mb_cur_max` and
  `_isctype`; January calls the declared CRT `isspace` function twice, so the
  macro is disabled for this TU.
- January deliberately calls `csmemset` for the engine-owned 12-byte table,
  while the 0x4000-byte automatic line buffer is cleared inline after `fopen`
  succeeds.
- the two parse/allocation failure labels now share the ordinary error/free
  cleanup, matching January's single `_error` and `_free_symbol_table` call
  sites.

The earlier scratch draft expressed the large clear as `memset` and disabled
`cseries.h`'s object-like `memset` alias.  That API inference was not justified:
January first stores `line[0] = 0`, then clears the remaining `0x3FFF` bytes
with `rep stosd`, `stosw`, and `stosb`, starting at `line+1`.  A normal
full-buffer `memset` would instead clear `0x4000/4` dwords from the aligned
buffer start.  The final reconstruction therefore uses a late lexical-block
declaration `char line[0x4000] = "";`, removes the `#undef memset`, and exactly
reproduces January's initialization sequence and its placement after the
successful open.  This remains an opcode-supported reconstruction inference;
the HCEA binary-derived reconstruction spells an explicit `memset`, so it is
corroboration for zero initialization but not original-source proof of the
January spelling.

January's first aggregate string-capacity assertion is reachable only on the
allocation/growth path.  The final source places that invariant check inside
the growth block, preserving the ordinary semantic relationship between a
successful reallocation and its postcondition.  This also reproduces the
target's second pair of `strlen` calls only on that path; the strings are not
modified between the capacity test and the assertion.  The superseded draft
placed the assertion unconditionally.

The bounded final source is
`scratch/stack-walk-load-symbol-table-natural-20260904.c`, SHA-256
`E2F59E45ACAB4213BD922F81DDB50A9A397C2CB5C272502FF1990DCC49FB639F`.
Its final diagnostic artifact is
`scratch/stack-walk-load-symbol-table-final-late-initializer-20260904.obj`,
SHA-256
`F70C50F5314D9264F805486EB4397CAB7D7A727AFF899FF6D62D3383F80267FB`.
It measures 8 exact / 1 residual / 7 unwritten and preserves all seven
inherited exact functions.

| Owner | January | Candidate | Result |
| --- | --- | --- | --- |
| `symbol_sort_proc` / `_code_000806d0` | 47 meaningful / 48 padded, 0 relocations, `444999547dd1ee50dcb9bc22d0d3abf3f065cc99c0eb763c77cd3c7ed6955f32` | identical | strict exact |
| `load_symbol_table` | 1556 meaningful / 1568 padded, 93 relocations, `db62270d0aef399714dcd1f59a9d88e06920e5e4679ec062531ff5b2d452f19f` | 1559 meaningful / 1568 padded, 94 relocations, `9c360ea92a4a4b05d13bce4e8b55006c008fb0ae79f7c313db3380b34717b89e` | honest fuzzy, ordinary objdiff 86.23505% |

The one remaining relocation surplus is one duplicated `_fgets` site in the
header-scan loop.  January and the candidate have the same parser behavior,
but VC7 rotates the candidate's natural loop and duplicates its call site;
the rest of the residual includes a three-register allocation cycle and the
two authenticated 256-byte local arrays occupying opposite compiler-selected
slots.  No declaration/lifetime search, fake branch, or register steering was
attempted.  The loader should be parked as `unclassified` if admitted; it
receives no exact credit.  Reopen only for new original source or compiler-
context evidence.

The candidate emits exactly the seven inherited functions plus the real
loader and comparator, with no extra code owner, no COMMON, and no
`_point_from_line3d`.  Its existing 20-byte `_stack_walk_globals` data owner is
unchanged and exact.  January's separate 0x4008-byte unresolved BSS owner is
not fabricated; it remains absent because its genuine symbol-formatter owner
is still unwritten.  The candidate additionally emits the ordinary 2-byte
`"r"` literal COMDAT referenced by `fopen`; January references the same
selected literal owner from outside this csplit object.  Whole-object
admission therefore remains out of scope even though the new comparator is
strict.

## Source-authenticity notes

The unused timestamp `strstr`, unchecked reads in the static-symbol
continuation, pointer-only `end_str` validation followed by a fixed `+5`, the
comparator's two-zero antisymmetry violation, and possible all-zero-RVA trim
underflow look unusual in isolation.  Each is directly present in January
machine control flow; the HCEA reconstruction independently mirrors most of
the parser edges.  They are retained as target-proven trusted-map debug-tool
behavior, not invented to improve matching, and each unsafe case now has an
explicit original-bug comment.  The scratch source has no `volatile`,
`register`, forced inline,
assembly, raw layout access, alias pun, synthetic emission anchor, or
candidate-only helper.  `tools/fake_match_scan.py` reports zero review leads.
The target-proven unsafe legacy edges have explicit `BUG (preserved for exact
matching)` comments with safe corrected behavior.

## Owning-header integration blocker

The production file currently carries a wrong TU-local `void
load_symbol_table(...)` declaration, while `cseries.c` and `errors.c` carry
their own cross-TU declarations for other stack-walk APIs.  Under the house
rules, `source/cseries/cseries.h` is the closest owning header and must own the
correct public `int` declaration; adding another local declaration is not an
admissible workaround.  The table's complete layout should remain private to
`stack_walk_windows.c`, with only `struct debug_symbol_table` forward-declared
for the pointer API.

Root tested both the broad cleanup and the minimal change consisting only of
that forward declaration plus the correct loader prototype in the existing
`STACK_WALK_WINDOWS.C` owner section.  Even the minimal genuine declaration
caused four unrelated inherited exact-function regressions across Decals,
Race, and UI translation units, plus a Shell anonymous data-owner label audit
failure.  Root restored the published header/source and a full Ninja rebuild
returned to the unchanged 6199-exact baseline.  No header ordering or
alternate-owner search is justified.  Consequently the loader/comparator
packet is **withheld research**, not an integration candidate: the exact
private comparator cannot be admitted alone because January retains it through
the genuine loader caller, and the loader cannot be admitted with an incorrect
or caller-local public prototype.

Final reproducible gate command (the alias is diagnostic only):

```text
python tools/campaign/gate.py source/cseries/stack_walk_windows --source scratch\stack-walk-load-symbol-table-natural-20260904.c --alias symbol_sort_proc=code_000806d0 --all --forbid-emitted-symbol _point_from_line3d --out scratch\stack-walk-load-symbol-table-final-late-initializer-20260904.obj
```

The final objdiff report is
`scratch/stack-walk-loader-report-20260904/report-late-initializer.json`.

## Complete withheld source

This standalone scratch translation unit includes inherited context for reproducibility.
Its local loader prototype is a diagnostic convenience, not approved production
ownership; future integration must apply the proper shared-header declaration above.

```c
/*
STACK_WALK_WINDOWS.C

Scratch-only natural reconstruction of load_symbol_table and its genuine
private qsort callback.  Production source remains unchanged.
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"

#include <ctype.h>

/* January calls the CRT function rather than the multibyte ctype macro. */
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

static int symbol_sort_proc(
	const void *elem1,
	const void *elem2);

void stack_walk_with_context(
	boolean disregard_symbol_names,
	short levels_to_ignore,
	void *context);

int load_symbol_table(
	char *filename,
	struct debug_symbol_table *symbol_table,
	char *timestamp_str);

/* ---------- globals */

struct _stack_walk_globals stack_walk_globals =
{
	NONE,
	FALSE
};

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
	long levels_to_ignore)
{
	stack_walk_with_context(
		FALSE,
		(short)(levels_to_ignore + 1),
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

		if (!fgets(line, sizeof(line), map_file))
		{
			goto close_map_file;
		}

		while (TRUE)
		{
			if (!fgets(line, sizeof(line), map_file))
			{
				error(_error_silent, "map file appears corrupt");
				goto close_map_file;
			}

			if (strstr(line, "Lib:Object"))
			{
				break;
			}

			if (strstr(line, "Timestamp"))
			{
				/* BUG (preserved for exact matching): January performs the
				 * timestamp search but ignores whether it succeeds.  A corrected
				 * build should reject a map whose timestamp does not match. */
				strstr(line, timestamp_str);
			}
		}

		string_storage_size = 0;
		string_storage_used = 0;
		symbols_size = 0;
		previous_library_object_offset = NONE;
		strcpy(last_object_file_name, "nothing");

		if (!fgets(line, sizeof(line), map_file))
		{
			goto close_map_file;
		}

		while (TRUE)
		{
			end_str = NULL;
			segment = strtok(line, ":");
			if (!segment || *segment!=' ')
			{
				goto read_next_line;
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
				if (!strstr(line, "entry point at"))
				{
					goto corrupt_map_file;
				}

				/* BUG (preserved for exact matching): January consumes each of
				 * these continuation lines without checking whether fgets failed.
				 * A corrected build should reject EOF before reading line[0] or
				 * tokenizing the buffer. */
				fgets(line, sizeof(line), map_file);
				if (!isspace(line[0]))
				{
					goto corrupt_map_file;
				}

				fgets(line, sizeof(line), map_file);
				if (!strstr(line, "Static symbols"))
				{
					goto corrupt_map_file;
				}

				fgets(line, sizeof(line), map_file);
				if (!isspace(line[0]))
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
			}

			/* BUG (preserved for exact matching): January leaves symbol_name
			 * unchanged if the continuation entry omits its name token, then
			 * continues parsing.  A corrected build should reject that entry. */
			if (token)
			{
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

			/* BUG (preserved for exact matching): January checks only whether
			 * strtoul assigned an end pointer, then advances five bytes without
			 * validating the conversion or remaining field width.  A corrected
			 * build should validate both before advancing. */
			if (!end_str)
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

read_next_line:
			if (!fgets(line, sizeof(line), map_file))
			{
				break;
			}
		}

		goto close_map_file;

allocation_failed:
		token = "could not allocate enough memory for map file";
		goto report_map_file_error;

corrupt_map_file:
		token = "map file appears corrupt";

report_map_file_error:
		error(_error_silent, token);
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

		if (symbol_table->symbols[symbol_table->number_of_symbols-1].rva_base==0)
		{
			/* BUG (preserved for exact matching): January assumes at least one
			 * nonzero RVA while trimming sentinels.  A corrected build should
			 * stop before number_of_symbols reaches zero. */
			do
			{
				symbol_table->number_of_symbols--;
			}
			while (symbol_table->symbols[symbol_table->number_of_symbols-1].rva_base==0);
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

	/* BUG (preserved for exact matching): January returns 1 when both RVAs
	 * are zero, violating comparator antisymmetry.  A corrected build should
	 * return 0 for two equal zero-RVA sentinel records. */
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
```
