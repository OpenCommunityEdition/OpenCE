/*
FILES_WINDOWS.C

symbols in this file:
00189860 0010:
	_file_location_is_valid (0000)
00189870 0020:
	_file_compare_last_modification_dates (0000)
00189890 00d0:
	_find_files_start (0000)
00189960 0090:
	_file_path_add_name (0000)
001899F0 0090:
	_file_path_add_extension (0000)
00189A80 0060:
	_file_path_remove_name (0000)
00189AE0 00e0:
	_file_path_split (0000)
00189BC0 0090:
	_file_location_get_full_path (0000)
00189C50 0050:
	_file_read_only (0000)
00189CA0 0040:
	_code_00189ca0 (0000)
00189CE0 00d0:
	_file_create (0000)
00189DB0 00e0:
	_file_delete (0000)
00189E90 0090:
	_file_exists (0000)
00189F20 00d0:
	_file_rename (0000)
00189FF0 0190:
	_file_open (0000)
0018A180 0070:
	_file_close (0000)
0018A1F0 0060:
	_file_get_position (0000)
0018A250 0070:
	_file_set_position (0000)
0018A2C0 0060:
	_file_get_eof (0000)
0018A320 0080:
	_file_set_eof (0000)
0018A3A0 00b0:
	_file_read (0000)
0018A450 00b0:
	_file_write (0000)
0018A500 0040:
	_file_read_from_position (0000)
0018A540 0040:
	_file_write_to_position (0000)
0018A580 00c0:
	_file_get_last_modification_date (0000)
0018A640 00e0:
	_file_get_size (0000)
0018A720 0200:
	_find_files_next (0000)
002A22C0 002a:
	??_C@_0CK@FOFFLBNA@?$CBTEST_FLAG?$CIinfo?9?$DOflags?0?5has_file@ (0000)
002A22EC 002f:
	??_C@_0CP@NFMDJMHE@VALID_FLAGS?$CIflags?0?5NUMBER_OF_FIN@ (0000)
002A231C 0029:
	??_C@_0CJ@EJLNJHHO@c?3?2halo?2SOURCE?2tag_files?2files_w@ (0000)
002A2348 0035:
	??_C@_0DF@LNJDDPKA@strlen?$CIpath?$CJ?$CL1?$CLstrlen?$CIname?$CJ?$DM?$DNMAX@ (0000)
002A2380 003a:
	??_C@_0DK@KGCFFLPC@strlen?$CIpath?$CJ?$CL1?$CLstrlen?$CIextension?$CJ@ (0000)
002A23BC 0004:
	??_C@_03FFANDCKJ@d?3?2?$AA@ (0000)
002A23C0 0012:
	??_C@_0BC@FIJDNJBC@path?5?$CG?$CG?5full_path?$AA@ (0000)
002A23D4 0016:
	??_C@_0BG@HKKDEIMM@?$CFs?$CI?8?$CFs?8?$CJ?5error?50x?$CF08x?$AA@ (0000)
002A23EC 000c:
	??_C@_0M@FMHLFEDN@file_create?$AA@ (0000)
002A23F8 000c:
	??_C@_0M@FHNKNLG@file_delete?$AA@ (0000)
002A2404 000c:
	??_C@_0M@LNEIIFKD@file_exists?$AA@ (0000)
002A2410 000a:
	??_C@_09FGGBLHDH@file_open?$AA@ (0000)
002A2420 0055:
	??_C@_0FF@PHPDOLGC@TEST_FLAG?$CIflags?0?5_permission_wri@ (0000)
002A2478 0041:
	??_C@_0EB@BLEHOBFN@flags?5?$CG?5?$CIFLAG?$CI_permission_read_b@ (0000)
002A24BC 002f:
	??_C@_0CP@LNPOBAKC@VALID_FLAGS?$CIflags?0?5NUMBER_OF_PER@ (0000)
002A24EC 000b:
	??_C@_0L@JPADBOF@file_close?$AA@ (0000)
002A24F8 0012:
	??_C@_0BC@MBEKKHAG@file_get_position?$AA@ (0000)
002A250C 0012:
	??_C@_0BC@LFPEGLEK@file_set_position?$AA@ (0000)
002A2520 000d:
	??_C@_0N@HOAJDFCL@file_get_eof?$AA@ (0000)
002A2530 000d:
	??_C@_0N@IGPIAKPK@file_set_eof?$AA@ (0000)
002A2540 000a:
	??_C@_09FEDAADPP@file_read?$AA@ (0000)
002A254C 000b:
	??_C@_0L@GOEDBBNC@file_write?$AA@ (0000)
002A2558 0020:
	??_C@_0CA@EAANDIHO@file_get_last_modification_date?$AA@ (0000)
002A2578 000e:
	??_C@_0O@GLLPAIHN@file_get_size?$AA@ (0000)
002A2588 0003:
	??_C@_02DJGKEECL@?4?4?$AA@ (0000)
002A258C 0004:
	??_C@_03EMIMMIHL@?$CK?4?$CK?$AA@ (0000)
00314CE0 0270:
	_data_00314ce0 (0000)
*/

/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "errors.h"

#define BUILDING_FILES_WINDOWS
#include "files.h"
#include "text/international_strings.h"

/* ---------- constants */

/* ---------- macros */

/*
 * The legacy accessor predates const-correct callers. It only validates and
 * returns the reference storage; keep its required conversion at this one
 * boundary and expose only a read-only view to this translation unit.
 */
#define file_reference_get_const_info(reference) \
	((struct file_reference_info const *) \
		file_reference_get_info((struct file_reference *)(reference)))

/* ---------- structures */

/* ---------- prototypes */

static void code_00189ca0(
	struct file_reference const *file,
	const char *function_name);

/* ---------- globals */

/* ---------- public code */

boolean file_location_is_valid(
	short location)
{
	return TRUE;
}

long file_compare_last_modification_dates(
	struct file_last_modification_date const *date0,
	struct file_last_modification_date const *date1)
{
	return csmemcmp(date0, date1, sizeof(*date0));
}

void file_path_add_name(
	char *path,
	char const *name)
{
	if (*name)
	{
		char *end;

		match_assert(
			"c:\\halo\\SOURCE\\tag_files\\files_windows.c",
			672,
			strlen(path)+1+strlen(name)<=MAXIMUM_FILENAME_LENGTH);
		end = path + strlen(path);
		if (end != path)
		{
			*end++ = '\\';
			*end = 0;
		}
		strncpy(end, name, MAXIMUM_FILENAME_LENGTH-strlen(path));
		path[MAXIMUM_FILENAME_LENGTH] = 0;
	}

	return;
}

void file_path_add_extension(
	char *path,
	char const *extension)
{
	if (*extension)
	{
		char *end;

		match_assert(
			"c:\\halo\\SOURCE\\tag_files\\files_windows.c",
			696,
			strlen(path)+1+strlen(extension)<=MAXIMUM_FILENAME_LENGTH);
		end = path + strlen(path);
		if (end != path)
		{
			*end++ = '.';
			*end = 0;
		}
		strncpy(end, extension, MAXIMUM_FILENAME_LENGTH-strlen(path));
		path[MAXIMUM_FILENAME_LENGTH] = 0;
	}

	return;
}

void file_path_remove_name(
	char *path)
{
	char *original_path = path;
	short index = (short)strlen(original_path);

	while (index && get_previous_character((byte *)original_path, &index) != '\\')
		;

	if (get_next_character((byte *)original_path, &index) == '\\')
	{
		index--;
		original_path[index] = 0;
	}
	else
		original_path[index] = 0;

	return;
}

void file_location_get_full_path(
	short location,
	const char *path,
	char *full_path)
{
	match_assert("c:\\halo\\SOURCE\\tag_files\\files_windows.c", 788, path && full_path);

	*full_path = 0;
	if (!path[0] || !path[1] || !path[2] || !(isalpha)(path[0]) ||
		path[1]!=':' || path[2]!='\\')
	{
		csstrcpy(full_path, "d:\\");
	}
	csstrcat(full_path, path);

	return;
}

boolean file_create(
	struct file_reference *file)
{
	struct file_reference_info *info = file_reference_get_info(file);
	char full_path[MAXIMUM_FILENAME_LENGTH+1] = "";

	file_location_get_full_path(info->location, info->path, full_path);

	if (TEST_FLAG(info->flags, _has_filename_bit))
	{
		void *file_handle = CreateFileA(full_path, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file_handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file_handle);
			return TRUE;
		}
	}
	else
	{
		if (CreateDirectoryA(info->path, NULL))
			return TRUE;
	}

	{
		struct file_reference_info *error_info = file_reference_get_info(file);

		error(_error_silent, "%s('%s') error 0x%08x", "file_create",
			error_info->path, GetLastError());
		SetLastError(0);
	}

	return FALSE;
}

boolean file_delete(
	struct file_reference *file)
{
	struct file_reference_info *info = file_reference_get_info(file);
	char full_path[MAXIMUM_FILENAME_LENGTH+1] = "";

	file_location_get_full_path(info->location, info->path, full_path);

	if (TEST_FLAG(info->flags, _has_filename_bit))
	{
		if (SetFileAttributesA(full_path, FILE_ATTRIBUTE_NORMAL) &&
			DeleteFileA(full_path))
		{
			return TRUE;
		}
	}
	else
	{
		if (RemoveDirectoryA(full_path))
			return TRUE;
	}

	{
		struct file_reference_info *error_info = file_reference_get_info(file);

		error(_error_silent, "%s('%s') error 0x%08x", "file_delete",
			error_info->path, GetLastError());
		SetLastError(0);
	}

	return FALSE;
}

boolean file_rename(
	struct file_reference *file,
	const char *new_name)
{
	struct file_reference_info *info = file_reference_get_info(file);
	boolean result = FALSE;
	char old_path[MAXIMUM_FILENAME_LENGTH+1] = "";
	char new_path[MAXIMUM_FILENAME_LENGTH+1] = "";

	file_location_get_full_path(info->location, info->path, old_path);
	csstrcpy(new_path, old_path);
	file_path_remove_name(new_path);
	file_path_add_name(new_path, new_name);

	if (MoveFileA(old_path, new_path))
	{
		file_path_remove_name(info->path);
		file_path_add_name(info->path, new_name);
		result = TRUE;
	}

	return result;
}

boolean file_get_last_modification_date(
	struct file_reference *file,
	struct file_last_modification_date *date)
{
	struct file_reference_info *info = file_reference_get_info(file);
	char full_path[MAXIMUM_FILENAME_LENGTH+1] = "";
	WIN32_FILE_ATTRIBUTE_DATA attribute_data;

	csmemset(date, 0, sizeof(*date));
	file_location_get_full_path(info->location, info->path, full_path);

	if (GetFileAttributesExA(full_path, GetFileExInfoStandard, &attribute_data))
	{
		csmemcpy(date, &attribute_data.ftLastWriteTime, sizeof(*date));
	}
	else
	{
		struct file_reference_info *error_info = file_reference_get_info(file);

		error(_error_silent, "%s('%s') error 0x%08x", "file_get_last_modification_date",
			error_info->path, GetLastError());
		SetLastError(0);
	}

	return TRUE;
}

boolean file_exists(
	const struct file_reference *file)
{
	struct file_reference_info const *info =
		file_reference_get_const_info(file);
	boolean result = FALSE;
	char full_path[MAXIMUM_FILENAME_LENGTH+1] = "";

	file_location_get_full_path(info->location, info->path, full_path);
	if (GetFileAttributesA(full_path) != (unsigned long)NONE)
	{
		result = TRUE;
	}
	else if (GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND)
	{
		code_00189ca0(file, "file_exists");
	}

	return result;
}

boolean file_read_only(
	struct file_reference *file)
{
	char full_path[MAXIMUM_FILENAME_LENGTH+1];
	struct file_reference_info *info = file_reference_get_info(file);
	unsigned long attributes;
	boolean read_only = FALSE;

	file_location_get_full_path(info->location, info->path, full_path);
	attributes = GetFileAttributesA(full_path);
	if (attributes != (unsigned long)-1 && (attributes & FILE_ATTRIBUTE_READONLY) != 0)
		read_only = TRUE;

	return read_only;
}

boolean file_close(
	struct file_reference *file)
{
	struct file_reference_info *info = file_reference_get_info(file);
	boolean result = FALSE;

	if (CloseHandle(info->file_handle))
	{
		info->file_handle = NULL;
		result = TRUE;
	}
	else
	{
		struct file_reference_info *error_info = file_reference_get_info(file);

		error(_error_silent, "%s('%s') error 0x%08x", "file_close",
			error_info->path, GetLastError());
		SetLastError(0);
	}

	return result;
}

unsigned long file_get_position(
	const struct file_reference *file)
{
	unsigned long position;
	void *file_handle = file_reference_get_const_info(file)->file_handle;

	position = SetFilePointer(file_handle, 0, NULL, FILE_CURRENT);
	if (position == (unsigned long)NONE)
	{
		struct file_reference_info const *info =
			file_reference_get_const_info(file);

		error(_error_silent, "%s('%s') error 0x%08x", "file_get_position",
			info->path, GetLastError());
		SetLastError(0);
	}

	return position;
}

boolean file_set_position(
	const struct file_reference *file,
	unsigned long position)
{
	struct file_reference_info const *info =
		file_reference_get_const_info(file);
	boolean result;

	result = SetFilePointer(info->file_handle, position, NULL, FILE_BEGIN) != (unsigned long)NONE;
	if (!result)
	{
		struct file_reference_info const *error_info =
			file_reference_get_const_info(file);

		error(_error_silent, "%s('%s') error 0x%08x", "file_set_position",
			error_info->path, GetLastError());
		SetLastError(0);
	}

	return result;
}

unsigned long file_get_eof(
	const struct file_reference *file)
{
	unsigned long size;
	void *file_handle = file_reference_get_const_info(file)->file_handle;

	size = GetFileSize(file_handle, NULL);
	if (size == (unsigned long)NONE)
	{
		struct file_reference_info const *info =
			file_reference_get_const_info(file);

		error(_error_silent, "%s('%s') error 0x%08x", "file_get_eof",
			info->path, GetLastError());
		SetLastError(0);
	}

	return size;
}

boolean file_set_eof(
	const struct file_reference *file,
	unsigned long position)
{
	struct file_reference_info const *info =
		file_reference_get_const_info(file);
	boolean result;

	if (file_set_position(file, position) && SetEndOfFile(info->file_handle))
	{
		result = TRUE;
	}
	else
	{
		struct file_reference_info const *error_info;

		result = FALSE;
		error_info = file_reference_get_const_info(file);
		error(_error_silent, "%s('%s') error 0x%08x", "file_set_eof",
			error_info->path, GetLastError());
		SetLastError(0);
	}

	return result;
}

boolean file_read_from_position(
	struct file_reference const *file,
	unsigned long position,
	unsigned long count,
	void *buffer)
{
	return file_set_position(file, position) && file_read(file, count, buffer);
}

boolean file_write_to_position(
	struct file_reference const *file,
	unsigned long position,
	unsigned long count,
	void const *buffer)
{
	return file_set_position(file, position) && file_write(file, count, buffer);
}

/* ---------- private code */

static void code_00189ca0(
	struct file_reference const *file,
	const char *function_name)
{
	struct file_reference_info const *info =
		file_reference_get_const_info(file);

	error(_error_silent, "%s('%s') error 0x%08x", function_name,
		info->path, GetLastError());
	SetLastError(0);

	return;
}
