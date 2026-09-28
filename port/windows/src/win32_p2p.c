/*
WIN32_P2P.C

The process and desktop half of port/linux/src/posix.h for Windows, which
internet play uses (p2p.c; the Linux versions are in posix_net.c): the
command line, the registry entry that makes this executable open halo://
links, and Discord's local pipe.
*/

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "posix.h"

/* ---------- the process */

int posix_command_line_argument(int index, char *buffer, posix_ulong size)
{
	/* split the command line as the C runtime does: spaces separate
	arguments, double quotes group them (the only case a link or a path
	needs) */
	const char *cursor = GetCommandLineA();
	int current = 0;

	if (!size)
		return 0;
	for (;;)
	{
		posix_ulong length = 0;
		BOOL quoted = FALSE;

		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (!*cursor)
			return 0;
		while (*cursor && (quoted || (*cursor != ' ' && *cursor != '\t')))
		{
			if (*cursor == '"')
				quoted = !quoted;
			else if (current == index && length + 1 < size)
				buffer[length++] = *cursor;
			cursor++;
		}
		if (current++ == index)
		{
			buffer[length] = '\0';
			return 1;
		}
	}
}

posix_ulong posix_process_id(void)
{
	return (posix_ulong)GetCurrentProcessId();
}

int posix_register_url_scheme(const char *scheme, const char *description)
{
	/* HKEY_CURRENT_USER\Software\Classes\<scheme>, as Windows looks links up */
	char executable[MAX_PATH], key_name[256], command[MAX_PATH + 16], label[256];
	HKEY key;
	BOOL ok = TRUE;

	if (!GetModuleFileNameA(NULL, executable, sizeof(executable)))
		return 0;
	snprintf(key_name, sizeof(key_name), "Software\\Classes\\%s", scheme);
	snprintf(label, sizeof(label), "URL:%s", description);
	if (RegCreateKeyExA(HKEY_CURRENT_USER, key_name, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS)
		return 0;
	ok &= RegSetValueExA(key, NULL, 0, REG_SZ, (const BYTE *)label, (DWORD)strlen(label) + 1) == ERROR_SUCCESS;
	ok &= RegSetValueExA(key, "URL Protocol", 0, REG_SZ, (const BYTE *)"", 1) == ERROR_SUCCESS;
	RegCloseKey(key);
	snprintf(key_name, sizeof(key_name), "Software\\Classes\\%s\\shell\\open\\command", scheme);
	snprintf(command, sizeof(command), "\"%s\" \"%%1\"", executable);
	if (RegCreateKeyExA(HKEY_CURRENT_USER, key_name, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS)
		return 0;
	ok &= RegSetValueExA(key, NULL, 0, REG_SZ, (const BYTE *)command, (DWORD)strlen(command) + 1) == ERROR_SUCCESS;
	RegCloseKey(key);
	return ok ? 1 : 0;
}

/* ---------- Discord's local pipe */

enum
{
	MAXIMUM_DISCORD_PIPES = 4,
};

static HANDLE discord_pipes[MAXIMUM_DISCORD_PIPES];

int posix_discord_connect(void)
{
	int slot;
	int number;

	for (slot = 0; slot < MAXIMUM_DISCORD_PIPES && discord_pipes[slot]; slot++)
		;
	if (slot == MAXIMUM_DISCORD_PIPES)
		return -1;
	for (number = 0; number < 10; number++)
	{
		char name[64];
		HANDLE pipe;

		snprintf(name, sizeof(name), "\\\\.\\pipe\\discord-ipc-%d", number);
		pipe = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (pipe != INVALID_HANDLE_VALUE)
		{
			discord_pipes[slot] = pipe;
			return slot;
		}
	}
	return -1;
}

int posix_discord_write(int handle, const void *buffer, int length)
{
	DWORD written;

	if (handle < 0 || handle >= MAXIMUM_DISCORD_PIPES || !discord_pipes[handle])
		return -1;
	if (!WriteFile(discord_pipes[handle], buffer, (DWORD)length, &written, NULL) || written != (DWORD)length)
		return -1;
	return length;
}

int posix_discord_read(int handle, void *buffer, int length)
{
	DWORD available = 0, read = 0;

	if (handle < 0 || handle >= MAXIMUM_DISCORD_PIPES || !discord_pipes[handle])
		return -1;
	/* the pipe is blocking: read only what is already there */
	if (!PeekNamedPipe(discord_pipes[handle], NULL, 0, NULL, &available, NULL))
		return -1;
	if (!available)
		return 0;
	if (available > (DWORD)length)
		available = (DWORD)length;
	if (!ReadFile(discord_pipes[handle], buffer, available, &read, NULL))
		return -1;
	return (int)read;
}

void posix_discord_close(int handle)
{
	if (handle >= 0 && handle < MAXIMUM_DISCORD_PIPES && discord_pipes[handle])
	{
		CloseHandle(discord_pipes[handle]);
		discord_pipes[handle] = NULL;
	}
}
