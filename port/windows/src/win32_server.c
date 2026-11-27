/*
WIN32_SERVER.C

A dedicated server's signals, standard input and output, and the process's
use of memory and processor (port/linux/src/posix_server.h), on Windows, in
place of posix_server.c: Ctrl+C, Ctrl+Break, the console's closing and the
system's shutdown ask to quit (a second Ctrl+C ends the process at once);
standard input is read by a thread of its own into a buffer, so that a
console's and a pipe's are read alike without waiting. The release build is
a windowed program, which has no console: a server started from a command
prompt writes to the prompt's (and one started with no console, by a service
manager, to the files or pipes its standard handles are).
*/

#include <windows.h>
#include <aclapi.h>
#include <io.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "posix_server.h"

enum
{
	_posix_signal_quit_ctrl_c = 2,
	INPUT_BUFFER_SIZE = 16384,
};

static volatile LONG server_signals;
static volatile LONG server_quit_signal;
static volatile LONG server_quit_count;
static volatile LONG server_deadline_code;

static CRITICAL_SECTION input_lock;
static char input_buffer[INPUT_BUFFER_SIZE];
static int input_length;
static int input_ended;
static int input_started;

static BOOL WINAPI server_console_handler(DWORD type)
{
	switch (type)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		if (InterlockedIncrement(&server_quit_count) > 1)
			ExitProcess(128 + _posix_signal_quit_ctrl_c);
		InterlockedExchange(&server_quit_signal, _posix_signal_quit_ctrl_c);
		InterlockedOr(&server_signals, _posix_server_signal_quit);
		/* (a closing console ends the process when this returns: the server
		is given the seconds Windows allows to stop) */
		if (type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT || type == CTRL_SHUTDOWN_EVENT)
			Sleep(4500);
		return TRUE;
	}
	return FALSE;
}

void posix_server_signals_install(void)
{
	SetConsoleCtrlHandler(server_console_handler, TRUE);
}

int posix_server_signals_take(int *quit_signal)
{
	int taken = (int)InterlockedExchange(&server_signals, 0);

	if (quit_signal)
		*quit_signal = (int)server_quit_signal;
	return taken;
}

static DWORD WINAPI deadline_thread(LPVOID seconds)
{
	Sleep((DWORD)(UINT_PTR)seconds * 1000);
	ExitProcess(server_quit_signal ? 128 + (UINT)server_quit_signal : (UINT)server_deadline_code);
	return 0;
}

void posix_server_quit_deadline(int seconds, int exit_code)
{
	HANDLE thread;

	server_deadline_code = exit_code;
	thread = CreateThread(NULL, 0, deadline_thread, (LPVOID)(UINT_PTR)(seconds > 0 ? seconds : 1), 0, NULL);
	if (thread)
		CloseHandle(thread);
}

/* a handle that is no console's and no file's, as a windowed program's are
when it is started from a command prompt */
static int handle_missing(DWORD which)
{
	HANDLE handle = GetStdHandle(which);

	return !handle || handle == INVALID_HANDLE_VALUE || GetFileType(handle) == FILE_TYPE_UNKNOWN;
}

void posix_server_stdout_line_buffered(void)
{
	/* (the prompt's console, else none: a service's handles are its own) */
	if (handle_missing(STD_OUTPUT_HANDLE) && AttachConsole(ATTACH_PARENT_PROCESS))
	{
		FILE *ignored;

		freopen_s(&ignored, "CONOUT$", "w", stdout);
		freopen_s(&ignored, "CONOUT$", "w", stderr);
		if (handle_missing(STD_INPUT_HANDLE))
			freopen_s(&ignored, "CONIN$", "r", stdin);
	}
	setvbuf(stdout, NULL, _IOLBF, 1024);
}

int posix_server_stdin_is_terminal(void)
{
	return _isatty(_fileno(stdin)) != 0;
}

static DWORD WINAPI input_thread(LPVOID unused)
{
	HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);

	(void)unused;
	for (;;)
	{
		char buffer[1024];
		DWORD count = 0;

		if (!handle || handle == INVALID_HANDLE_VALUE || !ReadFile(handle, buffer, sizeof(buffer), &count, NULL) ||
			!count)
		{
			EnterCriticalSection(&input_lock);
			input_ended = 1;
			LeaveCriticalSection(&input_lock);
			return 0;
		}
		EnterCriticalSection(&input_lock);
		/* (a reader that does not take what it is given loses the rest) */
		if (input_length + (int)count > INPUT_BUFFER_SIZE)
			count = (DWORD)(INPUT_BUFFER_SIZE - input_length);
		memcpy(input_buffer + input_length, buffer, count);
		input_length += (int)count;
		LeaveCriticalSection(&input_lock);
	}
}

int posix_server_stdin_read(char *buffer, int size)
{
	int count;

	if (!input_started)
	{
		HANDLE thread;

		input_started = 1;
		InitializeCriticalSection(&input_lock);
		thread = CreateThread(NULL, 0, input_thread, NULL, 0, NULL);
		if (!thread)
			return -1;
		CloseHandle(thread);
	}
	EnterCriticalSection(&input_lock);
	count = input_length < size ? input_length : size;
	if (count)
	{
		memcpy(buffer, input_buffer, (size_t)count);
		memmove(input_buffer, input_buffer + count, (size_t)(input_length - count));
		input_length -= count;
	}
	else if (input_ended)
	{
		count = -1;
	}
	LeaveCriticalSection(&input_lock);
	/* (the console's line ends: CR LF) */
	return count;
}

unsigned long posix_server_resident_bytes(void)
{
	PROCESS_MEMORY_COUNTERS counters;

	memset(&counters, 0, sizeof(counters));
	if (!K32GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
		return 0;
	return (unsigned long)counters.WorkingSetSize;
}

unsigned long posix_server_cpu_milliseconds(void)
{
	FILETIME created, exited, kernel, user;
	unsigned long long total;

	if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
		return 0;
	total = ((unsigned long long)kernel.dwHighDateTime << 32 | kernel.dwLowDateTime) +
		((unsigned long long)user.dwHighDateTime << 32 | user.dwLowDateTime);
	return (unsigned long)(total / 10000ULL);
}

static int wide_path(const char *path, wchar_t *wide, int size)
{
	return MultiByteToWideChar(CP_UTF8, 0, path, -1, wide, size) > 0;
}

int posix_server_rename(const char *from, const char *to)
{
	wchar_t wide_from[MAX_PATH * 2], wide_to[MAX_PATH * 2];

	return wide_path(from, wide_from, MAX_PATH * 2) && wide_path(to, wide_to, MAX_PATH * 2) &&
		MoveFileExW(wide_from, wide_to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

int posix_server_write_file(const char *path, const char *text, unsigned long size)
{
	char temporary[4200];
	wchar_t wide[MAX_PATH * 2];
	FILE *file;
	int written;

	if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary) ||
		!wide_path(temporary, wide, MAX_PATH * 2))
	{
		return 0;
	}
	file = _wfopen(wide, L"wb");
	if (!file)
		return 0;
	written = fwrite(text, 1, size, file) == size;
	if (fclose(file) != 0 || !written)
	{
		DeleteFileW(wide);
		return 0;
	}
	return posix_server_rename(temporary, path);
}

/* an access list that lets the process's user alone at a file (as a
mode of 0600 does): in acl, of size bytes; 1 if it was made */
static int owner_only_acl(ACL *acl, DWORD size)
{
	HANDLE token;
	union
	{
		TOKEN_USER user;
		BYTE bytes[256];
	} information;
	DWORD length = 0;
	int made;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
		return 0;
	made = GetTokenInformation(token, TokenUser, &information, sizeof(information), &length) &&
		InitializeAcl(acl, size, ACL_REVISION) &&
		AddAccessAllowedAce(acl, ACL_REVISION, FILE_ALL_ACCESS, information.user.User.Sid);
	CloseHandle(token);
	return made;
}

/* (the access list replaces the folder's, which is not inherited) */
#define OWNER_ONLY_INFORMATION (DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION)

int posix_server_private_file(const char *path)
{
	wchar_t wide[MAX_PATH * 2];
	union
	{
		ACL acl;
		BYTE bytes[512];
	} acl;

	return wide_path(path, wide, MAX_PATH * 2) && owner_only_acl(&acl.acl, sizeof(acl)) &&
		SetNamedSecurityInfoW(wide, SE_FILE_OBJECT, OWNER_ONLY_INFORMATION, NULL, NULL, &acl.acl, NULL) ==
		ERROR_SUCCESS;
}

int posix_server_write_private_file(const char *path, const void *data, unsigned long size)
{
	wchar_t wide[MAX_PATH * 2];
	union
	{
		ACL acl;
		BYTE bytes[512];
	} acl;
	SECURITY_DESCRIPTOR descriptor;
	SECURITY_ATTRIBUTES attributes;
	HANDLE file;
	DWORD written = 0;
	int succeeded;

	if (!wide_path(path, wide, MAX_PATH * 2) || !owner_only_acl(&acl.acl, sizeof(acl)) ||
		!InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION) ||
		!SetSecurityDescriptorDacl(&descriptor, TRUE, &acl.acl, FALSE) ||
		!SetSecurityDescriptorControl(&descriptor, SE_DACL_PROTECTED, SE_DACL_PROTECTED))
	{
		return 0;
	}
	attributes.nLength = sizeof(attributes);
	attributes.lpSecurityDescriptor = &descriptor;
	attributes.bInheritHandle = FALSE;
	/* (created with the access list; a file that was there keeps its own,
	so it is given this one before anything is written) */
	file = CreateFileW(wide, GENERIC_WRITE | WRITE_DAC, 0, &attributes, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return 0;
	if (SetSecurityInfo(file, SE_FILE_OBJECT, OWNER_ONLY_INFORMATION, NULL, NULL, &acl.acl, NULL) != ERROR_SUCCESS)
	{
		CloseHandle(file);
		return 0;
	}
	succeeded = WriteFile(file, data, (DWORD)size, &written, NULL) && written == (DWORD)size;
	return CloseHandle(file) && succeeded;
}

int posix_server_remove(const char *path)
{
	wchar_t wide[MAX_PATH * 2];

	return wide_path(path, wide, MAX_PATH * 2) && DeleteFileW(wide);
}

static unsigned long long unix_hundred_nanoseconds(void)
{
	FILETIME now;

	GetSystemTimeAsFileTime(&now);
	/* (the FILETIME epoch is 1601) */
	return ((unsigned long long)now.dwHighDateTime << 32 | now.dwLowDateTime) - 116444736000000000ULL;
}

unsigned long posix_server_unix_seconds(void)
{
	return (unsigned long)(unix_hundred_nanoseconds() / 10000000ULL);
}

long long posix_server_unix_milliseconds(void)
{
	return (long long)(unix_hundred_nanoseconds() / 10000ULL);
}

void posix_server_utc_text(unsigned long seconds, char *text, int size)
{
	time_t when = (time_t)seconds;
	struct tm utc;

	if (gmtime_s(&utc, &when) || !strftime(text, (size_t)size, "%Y-%m-%dT%H:%M:%SZ", &utc))
		snprintf(text, (size_t)size, "%lu", seconds);
}

void posix_server_local_text(unsigned long seconds, char *text, int size)
{
	time_t when = (time_t)seconds;
	struct tm local;

	if (localtime_s(&local, &when) || !strftime(text, (size_t)size, "%Y-%m-%d %H:%M:%S", &local))
		snprintf(text, (size_t)size, "%lu", seconds);
}
