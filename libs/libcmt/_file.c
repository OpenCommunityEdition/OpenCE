/*
_FILE.C

symbols in this file:
	___iob_func (.text, 0x06)
	___initstdio (.text, 0xA9)
	___endstdio (.text, 0x14)
	__lock_file (.text, 0x2F)
	__lock_file2 (.text, 0x23)
	__unlock_file (.text, 0x2F)
	__unlock_file2 (.text, 0x23)
	_data_002b6aa4 (.CRT$XIC, 0x04)
	_data_002b6ab4 (.CRT$XPX, 0x04)
	__iob (.data, 0x280)
	__cflush (.bss, 0x04)
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

/* ---------- private CRT constants */

enum
{
	minimum_stream_count = 20,
	default_stream_count = 512,
	stdio_initialization_error = 26,
	stream_lock_base = 16,
	ioinfo_array_shift = 5,
	ioinfo_array_mask = 31
};

/* ---------- private CRT types */

struct runtime_critical_section
{
	unsigned long raw_event[4];
	long lock_count;
	long recursion_count;
	void *owning_thread;
};

struct extended_file
{
	FILE stream;
	struct runtime_critical_section lock;
};

struct io_info
{
	long os_file_handle;
	char flags;
	unsigned char pipe_character;
	unsigned short alignment;
	int lock_initialized;
	struct runtime_critical_section lock;
};

typedef int (__cdecl *initializer_proc)(
	void);

typedef void (__cdecl *terminator_proc)(
	void);

typedef char assert_file_size[
	sizeof(FILE) == 0x20 ? 1 : -1];
typedef char assert_runtime_critical_section_size[
	sizeof(struct runtime_critical_section) == 0x1c ? 1 : -1];
typedef char assert_extended_file_lock_offset[
	offsetof(struct extended_file, lock) == 0x20 ? 1 : -1];
typedef char assert_io_info_size[
	sizeof(struct io_info) == 0x28 ? 1 : -1];

/* ---------- external data */

extern int _nstream;
extern void **__piob;
extern struct io_info *__pioinfo[];
extern char _exitflag;

/*
 * csplit gives the linked address immediately after _iob[] its own symbol.
 * January's loop compares against this proven table boundary.
 */
extern FILE data_00319050;

/* ---------- external code */

int __cdecl _flushall(
	void);

int __cdecl _fcloseall(
	void);

void __cdecl _lock(
	int lock_number);

void __cdecl _unlock(
	int lock_number);

__declspec(dllimport) void __stdcall RtlEnterCriticalSection(
	struct runtime_critical_section *critical_section);

__declspec(dllimport) void __stdcall RtlLeaveCriticalSection(
	struct runtime_critical_section *critical_section);

/* ---------- public data */

FILE _iob[minimum_stream_count] =
{
	{ NULL, 0, NULL, _IOREAD, 0, 0, 0, NULL },
	{ NULL, 0, NULL, _IOWRT, 1, 0, 0, NULL },
	{ NULL, 0, NULL, _IOWRT, 2, 0, 0, NULL }
};

int _cflush = 0;

/* ---------- public code */

FILE * __cdecl __iob_func(
	void)
{
	return _iob;
}

int __cdecl __initstdio(
	void)
{
	FILE *stream;
	int stream_index;

	if (_nstream == 0)
	{
		_nstream = default_stream_count;
	}
	else if (_nstream < minimum_stream_count)
	{
		_nstream = minimum_stream_count;
	}

	__piob = calloc(_nstream, sizeof(void *));
	if (!__piob)
	{
		_nstream = minimum_stream_count;
		__piob = calloc(_nstream, sizeof(void *));
		if (!__piob)
		{
			return stdio_initialization_error;
		}
	}

	stream_index = 0;
	do
	{
		stream = &_iob[stream_index];
		__piob[stream_index++] = stream;
	}
	/* The source induction variable is signed; preserve that comparison. */
	while ((long)(stream + 1) < (long)&data_00319050);

	for (stream_index = 0; stream_index < 3; stream_index++)
	{
		long handle = __pioinfo[stream_index >> ioinfo_array_shift]
			[stream_index & ioinfo_array_mask].os_file_handle;

		if (handle == -1 || handle == 0)
		{
			_iob[stream_index]._file = -1;
		}
	}

	return 0;
}

void __cdecl __endstdio(
	void)
{
	_flushall();
	if (_exitflag)
	{
		_fcloseall();
	}
}

void __cdecl _lock_file(
	void *file)
{
	if (file >= (void *)_iob &&
		file <= (void *)&_iob[minimum_stream_count - 1])
	{
		_lock(stream_lock_base + ((FILE *)file - _iob));
	}
	else
	{
		RtlEnterCriticalSection(&((struct extended_file *)file)->lock);
	}
}

void __cdecl _lock_file2(
	int stream_index,
	void *file)
{
	if (stream_index < minimum_stream_count)
	{
		_lock(stream_lock_base + stream_index);
	}
	else
	{
		RtlEnterCriticalSection(&((struct extended_file *)file)->lock);
	}
}

void __cdecl _unlock_file(
	void *file)
{
	if (file >= (void *)_iob &&
		file <= (void *)&_iob[minimum_stream_count - 1])
	{
		_unlock(stream_lock_base + ((FILE *)file - _iob));
	}
	else
	{
		RtlLeaveCriticalSection(&((struct extended_file *)file)->lock);
	}
}

void __cdecl _unlock_file2(
	int stream_index,
	void *file)
{
	if (stream_index < minimum_stream_count)
	{
		_unlock(stream_lock_base + stream_index);
	}
	else
	{
		RtlLeaveCriticalSection(&((struct extended_file *)file)->lock);
	}
}

/* ---------- runtime initializer ownership */

#pragma data_seg(".CRT$XIC")
initializer_proc data_002b6aa4 = __initstdio;

#pragma data_seg(".CRT$XPX")
terminator_proc data_002b6ab4 = __endstdio;

#pragma data_seg()
