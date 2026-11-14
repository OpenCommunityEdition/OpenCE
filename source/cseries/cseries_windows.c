/* ---------- headers */

#include "cseries.h"
#include "cseries_windows.h"
#include "errors.h"

#include <time.h>

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- globals */

/* ---------- public code */

void display_debug_string(
	const char *string)
{
	OutputDebugStringA(string);
	return;
}

void system_exit(
	long code)
{
#ifdef HALO_RELEASE
	/* an assertion a release build skipped (display_assert) carries on; a
	fatal error (errors.c exits with -4998) still stops */
	if (code == -1 && display_assert_skipped)
	{
		display_assert_skipped = FALSE;
		return;
	}
#endif
	halt_and_catch_fire();
	return;
}

void system_unique_identifier_get(
	void *identifier)
{
	display_assert(NULL, "c:\\halo\\SOURCE\\cseries\\cseries_windows.c", 65, TRUE);
	halt_and_catch_fire();
	return;
}

long system_unique_identifiers_equal(
	const void *identifier1,
	const void *identifier2)
{
	byte empty_identifier[16];

	csmemset(empty_identifier, 0, sizeof(empty_identifier));
	if (csmemcmp(identifier1, empty_identifier, sizeof(empty_identifier)) != 0)
	{
		if (csmemcmp(identifier1, identifier2, sizeof(empty_identifier)) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

unsigned long system_milliseconds(
	void)
{
	return GetTickCount();
}

unsigned long system_seconds(
	void)
{
	return time(NULL);
}

void system_get_user_name(
	char *user_name,
	short maximum_length)
{
	csstrncpy(user_name, "xbox", maximum_length);
	return;
}

void *system_calloc(
	long count,
	long size)
{
	return GlobalAlloc(GMEM_ZEROINIT, count * size);
}

void *system_malloc(
	long size)
{
	return GlobalAlloc(0, size);
}

void system_free(
	void *pointer)
{
	LocalFree(pointer);
	return;
}

void *system_realloc(
	void *pointer,
	long size)
{
	if (size < 0)
	{
		display_assert("size>=0", "c:\\halo\\SOURCE\\cseries\\cseries_windows.c", 156, TRUE);
		halt_and_catch_fire();
	}

	if (pointer == NULL)
	{
		if (size == 0)
		{
			display_assert("pointer||size", "c:\\halo\\SOURCE\\cseries\\cseries_windows.c", 157, TRUE);
			halt_and_catch_fire();
		}
		return GlobalAlloc(0, size);
	}

	if (size != 0)
	{
		return GlobalReAlloc(pointer, size, GMEM_MOVEABLE);
	}

	LocalFree(pointer);
	return NULL;
}

unsigned long system_get_used_memory_size(
	void *pointer)
{
	return LocalSize(pointer);
}

void system_memory_information_get(
	struct system_memory_information *information)
{
	MEMORYSTATUS status;

	csmemset(&status, 0, sizeof(status));
	status.dwLength = sizeof(status);
	GlobalMemoryStatus(&status);
	csmemset(information, 0, sizeof(*information));
	information->free = status.dwAvailPhys;
	information->total = status.dwTotalPhys;
	return;
}

void system_show_wait_cursor(
	const char *file,
	long line)
{
	return;
}

void system_alert(
	void)
{
	return;
}

void system_kill_screen_saver(
	void)
{
	return;
}

static const char *exception_code_get_string(
	unsigned long exception_code)
{
	const char *exception_name = NULL;

	switch (exception_code)
	{
	case EXCEPTION_FLT_INVALID_OPERATION:
		exception_name = "EXCEPTION_FLT_INVALID_OPERATION";
		break;
	case EXCEPTION_FLT_OVERFLOW:
		exception_name = "EXCEPTION_FLT_OVERFLOW";
		break;
	case EXCEPTION_FLT_STACK_CHECK:
		exception_name = "EXCEPTION_FLT_STACK_CHECK";
		break;
	case EXCEPTION_FLT_UNDERFLOW:
		exception_name = "EXCEPTION_FLT_UNDERFLOW";
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		exception_name = "EXCEPTION_INT_DIVIDE_BY_ZERO";
		break;
	case EXCEPTION_INT_OVERFLOW:
		exception_name = "EXCEPTION_INT_OVERFLOW";
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		exception_name = "EXCEPTION_PRIV_INSTRUCTION";
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		exception_name = "EXCEPTION_FLT_INEXACT_RESULT";
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		exception_name = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		exception_name = "EXCEPTION_FLT_DENORMAL_OPERAND";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		exception_name = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		exception_name = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
		break;
	case EXCEPTION_ACCESS_VIOLATION:
		exception_name = "EXCEPTION_ACCESS_VIOLATION";
		break;
	case EXCEPTION_SINGLE_STEP:
		exception_name = "EXCEPTION_SINGLE_STEP";
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		exception_name = "EXCEPTION_DATATYPE_MISALIGNMENT";
		break;
	case EXCEPTION_BREAKPOINT:
		exception_name = "EXCEPTION_BREAKPOINT";
		break;
	}

	return exception_name;
}

long generic_exception_filter(
	unsigned long exception_code,
	PEXCEPTION_POINTERS exception_information)
{
	const char *exception_name = exception_code_get_string(exception_code);

	stack_walk_with_context(NULL, 0, exception_information->ContextRecord);
	if (exception_name)
	{
		error(_error_silent, "%s", exception_name);
	}
	else
	{
		error(_error_silent, "unknown exception %08lX", exception_code);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

/* ---------- private code */
