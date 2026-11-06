/*
WIN64_MAIN.C

Entry point of the Windows x64 build (port/windows/host,
tools/windows_x64_build.py): the 64-bit x86 Linux build's guest image (the
game as x32 code, tools/linux_guest_build.py), unchanged, in a 64-bit
Windows program, as the Linux host (port/linux/host/host_main.c) runs it in
a 64-bit Linux one. docs/x64_route.md, "Windows x64 host", has the design.

1. The guest's fixed ranges below 4 GB (the Xbox window at 0x80000000, the
   image's at 0x70000000, Custom Edition's tag cache at 0x40440000) are
   reserved before anything else can take them. If something already holds
   one (an overlay or another program's DLL injected into the process), the
   program starts itself again suspended, reserves the ranges in the new
   process before any DLL of its own is loaded there, lets it run, and
   waits for it (HALO_HOST_RESERVED tells the new process to take the
   ranges as its own).
2. The guest image, inside the executable (win64_guest_image.S), is loaded
   (host_loader.c), its imports bound to the thunks of
   tools/windows_host_thunks.py.
3. The guest's main runs on a thread of its own (win64_thread.c), with the
   part of this process's environment it reads: the HALO_ settings, and
   HOME, APPDATA and TMPDIR in the guest's path form (forward slashes). The
   game finds its data, config.toml and its saves (in %APPDATA%\halo, the
   32-bit Windows build's folder) as the Linux build does.

The log: the standard error stream, or halo.log next to halo.exe when there
is none (a release build has no console, as the 32-bit build's).
*/

#include "win64_host.h"

#include <mmsystem.h>

#include <SDL3/SDL.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* win64_guest_image.S */
extern const unsigned char halo_guest_image_start[];
extern const unsigned char halo_guest_image_end[];

/* laptops with a second, faster GPU run a program on the integrated one
unless it exports these (as win32_posix.c does for the 32-bit build) */
__declspec(dllexport) DWORD NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#define RELAUNCHED_VARIABLE L"HALO_HOST_RESERVED"

/* ---------- the log */

static HANDLE log_handle = INVALID_HANDLE_VALUE;
static SRWLOCK log_lock = SRWLOCK_INIT;

static void log_open(void)
{
	HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
	wchar_t path[MAX_PATH + 16];
	DWORD length;
	wchar_t *slash;

	if (handle && handle != INVALID_HANDLE_VALUE && GetFileType(handle) != FILE_TYPE_UNKNOWN)
	{
		log_handle = handle;
		return;
	}
	length = GetModuleFileNameW(NULL, path, MAX_PATH);
	if (!length || length >= MAX_PATH || !(slash = wcsrchr(path, L'\\')))
		return;
	wcscpy(slash + 1, L"halo.log");
	log_handle = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
}

static void log_write(const char *bytes, size_t size)
{
	DWORD written;

	if (log_handle == INVALID_HANDLE_VALUE)
		return;
	WriteFile(log_handle, bytes, (DWORD)size, &written, NULL);
}

/* one write per line, so that the lines of threads do not mix */
static void log_line(int priority, const char *line)
{
	const char *level = priority >= HOST_LOG_FATAL ? "fatal: " : priority >= HOST_LOG_ERROR ? "error: " :
		priority >= HOST_LOG_WARN ? "warning: " : "";
	char text[1200];
	int length = snprintf(text, sizeof(text), "halo-host: %s%s\n", level, line);

	if (length < 0)
		return;
	if ((size_t)length >= sizeof(text))
		length = (int)sizeof(text) - 1;
	AcquireSRWLockExclusive(&log_lock);
	log_write(text, (size_t)length);
	ReleaseSRWLockExclusive(&log_lock);
}

/* the guest's standard output and error (win64_syscall.c) */
void host_console_write(int fd, const char *bytes, size_t size)
{
	(void)fd;
	AcquireSRWLockExclusive(&log_lock);
	log_write(bytes, size);
	ReleaseSRWLockExclusive(&log_lock);
}

/* the C runtime's descriptors 0, 1 and 2 exist, so that the guest's files
never get them (win64_syscall.c sends 1 and 2 to the log) */
static void standard_descriptors(void)
{
	int fd;

	for (fd = 0; fd <= 2; fd++)
	{
		intptr_t handle = _get_osfhandle(fd);

		if (handle == -1)
		{
			int null = _open("NUL", _O_RDWR | _O_BINARY);

			if (null >= 0 && null != fd)
			{
				_dup2(null, fd);
				_close(null);
			}
		}
	}
}

/* ---------- the guest's environment */

/* the guest's path form of a Windows path in the environment */
static void environment_set_path(struct host_environment *environment, const char *name, const wchar_t *variable)
{
	wchar_t value[32768];
	char path[32768];
	DWORD length = GetEnvironmentVariableW(variable, value, (DWORD)(sizeof(value) / sizeof(value[0])));

	if (!length || length >= sizeof(value) / sizeof(value[0]))
		return;
	if (host_path_out(value, path, sizeof(path)) > 0)
		host_environment_set(environment, name, path);
}

/* what the game reads: its settings (HALO_...) and the language; where the
saves and the temporary files go are set from Windows' folders */
static void environment_fill(struct host_environment *environment)
{
	wchar_t *block = GetEnvironmentStringsW();
	const wchar_t *entry;
	char zone[64];

	for (entry = block; entry && *entry; entry += wcslen(entry) + 1)
	{
		char text[32768];

		if (wcsncmp(entry, L"HALO_", 5) && wcsncmp(entry, L"LANG=", 5) && wcsncmp(entry, L"LC_", 3))
			continue;
		if (WideCharToMultiByte(CP_UTF8, 0, entry, -1, text, (int)sizeof(text), NULL, NULL) > 0)
			host_environment_add(environment, text);
	}
	if (block)
		FreeEnvironmentStringsW(block);
	environment_set_path(environment, "HOME", L"USERPROFILE");
	environment_set_path(environment, "APPDATA", L"APPDATA");
	environment_set_path(environment, "TMPDIR", L"TEMP");
	{
		wchar_t user[256];
		char name[512];
		DWORD length = GetEnvironmentVariableW(L"USERNAME", user, (DWORD)(sizeof(user) / sizeof(user[0])));

		if (length && length < sizeof(user) / sizeof(user[0]) &&
			WideCharToMultiByte(CP_UTF8, 0, user, -1, name, (int)sizeof(name), NULL, NULL) > 0)
		{
			host_environment_set(environment, "USER", name);
		}
	}
	host_time_zone(zone, sizeof(zone));
	host_environment_set(environment, "TZ", zone);
}

/* host_desktop.c's host_sdl_get_base_path, in the guest's path form (the
import table names this one) */
void win64_sdl_get_base_path(char *buffer, uint32_t size)
{
	const char *path = SDL_GetBasePath();
	char *cursor;

	SDL_strlcpy(buffer, path ? path : "", size);
	for (cursor = buffer; *cursor; cursor++)
	{
		if (*cursor == '\\')
			*cursor = '/';
	}
}

/* ---------- starting again with the fixed ranges reserved */

static int relaunch(void)
{
	STARTUPINFOW startup;
	PROCESS_INFORMATION process;
	wchar_t path[MAX_PATH];
	DWORD code = 1;
	int index;

	if (!GetModuleFileNameW(NULL, path, MAX_PATH))
		return 1;
	SetEnvironmentVariableW(RELAUNCHED_VARIABLE, L"1");
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	GetStartupInfoW(&startup);
	if (!CreateProcessW(path, GetCommandLineW(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &startup, &process))
	{
		host_logf(HOST_LOG_FATAL, "cannot start the game again (error %lu)", (unsigned long)GetLastError());
		return 1;
	}
	for (index = 0; index < host_memory_fixed_count(); index++)
	{
		uint64_t base, size;

		host_memory_fixed_range(index, &base, &size);
		if (VirtualAllocEx(process.hProcess, (void *)(uintptr_t)base, size, MEM_RESERVE, PAGE_NOACCESS) !=
			(void *)(uintptr_t)base)
		{
			host_logf(HOST_LOG_WARN, "cannot reserve %08llx-%08llx in the new process either (error %lu)",
				(unsigned long long)base, (unsigned long long)(base + size), (unsigned long)GetLastError());
		}
	}
	ResumeThread(process.hThread);
	CloseHandle(process.hThread);
	WaitForSingleObject(process.hProcess, INFINITE);
	GetExitCodeProcess(process.hProcess, &code);
	CloseHandle(process.hProcess);
	return (int)code;
}

/* ---------- main */

#define MAIN_STACK_SIZE (16 * 1024 * 1024)

static void *game_main(void *unused)
{
	struct host_environment environment = { { 0 }, 0 };
	const char *base = SDL_GetBasePath();
	char path[4096];

	(void)unused;
	environment_fill(&environment);
	if (host_load_image(halo_guest_image_start, (size_t)(halo_guest_image_end - halo_guest_image_start)) != 0)
		host_fatal("cannot load the game image");
	snprintf(path, sizeof(path), "%sconfig.toml", base ? base : "");
	host_apply_debug_settings(path, NULL);
	host_run_guest_main(host_make_boot(&environment, "halo"));
	return NULL;
}

int main(int argc, char *argv[])
{
	int relaunched = GetEnvironmentVariableW(RELAUNCHED_VARIABLE, NULL, 0) > 0;

	(void)argc;
	(void)argv;
	/* first, before this process allocates anything more */
	if (host_memory_reserve_fixed(relaunched) != 0)
	{
		log_open();
		host_log_set_sink(log_line);
		if (!relaunched)
		{
			host_logf(HOST_LOG_WARN, "the game's memory below 4 GB is taken; starting again with it reserved");
			return relaunch();
		}
	}
	SetEnvironmentVariableW(RELAUNCHED_VARIABLE, NULL);
	if (log_handle == INVALID_HANDLE_VALUE)
		log_open();
	host_log_set_sink(log_line);
	standard_descriptors();
	/* Sleep() and the waitable timers in 1 ms steps rather than 15.6 ms */
	timeBeginPeriod(1);
	host_threads_initialize();
	host_install_exception_handler();
	if (host_native_thread_create(game_main, NULL, MAIN_STACK_SIZE) != 0)
		host_fatal("cannot start the game thread");
	/* the game ends the process itself (host_exit) */
	for (;;)
		Sleep(INFINITE);
}
