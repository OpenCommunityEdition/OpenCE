/*
WIN32_P2P.C

The process and desktop half of port/linux/src/posix.h for Windows, which
internet play uses (p2p.c; the Linux versions are in posix_net.c): the
command line, the registry entry that makes this executable open halo://
links, the user's secret, and Discord's local pipe.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
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

int posix_user_secret(unsigned char *secret, int size)
{
	/* in the user's local application data, which only they (and the
	system's administrators) can read */
	char directory[MAX_PATH], path[MAX_PATH + 64];
	DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", directory, sizeof(directory));
	int attempt;

	if (!length || length >= sizeof(directory))
		return 0;
	snprintf(path, sizeof(path), "%s\\halo-ce-universal.key", directory);
	for (attempt = 0; attempt < 3; attempt++)
	{
		DWORD done = 0;
		BOOL ok;
		HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);

		if (file != INVALID_HANDLE_VALUE)
		{
			ok = ReadFile(file, secret, (DWORD)size, &done, NULL) && done == (DWORD)size;
			CloseHandle(file);
			/* a key cut short (a write that failed, or was stopped) is made
			again: the delete fails while another copy still writes it */
			if (!ok && attempt == 0 && DeleteFileA(path))
				continue;
			return ok ? 1 : 0;
		}
		if (GetLastError() == ERROR_SHARING_VIOLATION)
			return 0;
		file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		/* (another copy of the game made it first: read that one) */
		if (file == INVALID_HANDLE_VALUE)
			continue;
		posix_random_bytes(secret, (posix_ulong)size);
		ok = WriteFile(file, secret, (DWORD)size, &done, NULL) && done == (DWORD)size;
		CloseHandle(file);
		if (!ok)
			DeleteFileA(path);
		return ok ? 1 : 0;
	}
	return 0;
}

/* ---------- Discord's local pipe */

enum
{
	MAXIMUM_DISCORD_PIPES = 4,
};

static HANDLE discord_pipes[MAXIMUM_DISCORD_PIPES];

/* a token's user, into buffer; NULL if not had */
static PSID token_user(HANDLE token, BYTE *buffer, DWORD size)
{
	DWORD length = 0;

	if (!GetTokenInformation(token, TokenUser, buffer, size, &length))
		return NULL;
	return ((TOKEN_USER *)buffer)->User.Sid;
}

/* whether the pipe's server runs as this process's user (pipe names are
the whole machine's: another user may make one, and would be given the
invite) */
static int pipe_server_is_this_user(HANDLE pipe)
{
	BYTE ours[256], theirs[256];
	ULONG process_id = 0;
	HANDLE process, token;
	PSID our_user = NULL, their_user = NULL;
	int result;

	if (!GetNamedPipeServerProcessId(pipe, &process_id))
		return 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
	{
		our_user = token_user(token, ours, sizeof(ours));
		CloseHandle(token);
	}
	process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
	if (!process)
		return 0;
	if (OpenProcessToken(process, TOKEN_QUERY, &token))
	{
		their_user = token_user(token, theirs, sizeof(theirs));
		CloseHandle(token);
	}
	CloseHandle(process);
	result = our_user && their_user && EqualSid(our_user, their_user);
	return result;
}

/* this machine's SMBIOS system UUID (its type 1 structure's), which a
reinstall keeps; else the registry's MachineGuid, which it does not (the
64-bit registry's: this process is 32-bit); as text, 0 if neither
(p2p.c's hardware id) */
int posix_hardware_id_source(char *text, int size)
{
	DWORD table_size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
	HKEY key;

	if (table_size > 8 && table_size < 1024 * 1024)
	{
		BYTE *table = (BYTE *)malloc(table_size);

		if (table && GetSystemFirmwareTable('RSMB', 0, table, table_size) == table_size)
		{
			/* (a RawSMBIOSData: 8 bytes of header, its length, the structures) */
			DWORD length = *(DWORD *)(table + 4);
			BYTE *structure = table + 8;
			BYTE *end = table + 8 + (length < table_size - 8 ? length : table_size - 8);

			while (structure + 4 <= end && structure[1] >= 4)
			{
				BYTE *strings = structure + structure[1];

				if (structure[0] == 1 && structure[1] >= 0x18)
				{
					BYTE *uuid = structure + 8;
					int zeros = 1, ones = 1, index;

					for (index = 0; index < 16; index++)
					{
						zeros &= uuid[index] == 0x00;
						ones &= uuid[index] == 0xFF;
					}
					if (!zeros && !ones && size >= 33)
					{
						for (index = 0; index < 16; index++)
							snprintf(text + 2 * index, 3, "%02x", uuid[index]);
						free(table);
						return 1;
					}
					break;
				}
				if (structure[0] == 127)
					break;
				/* (past its strings, which end with two zeros) */
				while (strings + 1 < end && (strings[0] || strings[1]))
					strings++;
				structure = strings + 2;
			}
		}
		free(table);
	}
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY,
		&key) == ERROR_SUCCESS)
	{
		DWORD type = 0;
		DWORD value_size = (DWORD)size - 1;
		LONG result = RegQueryValueExA(key, "MachineGuid", NULL, &type, (BYTE *)text, &value_size);

		RegCloseKey(key);
		if (result == ERROR_SUCCESS && type == REG_SZ && value_size > 0)
		{
			text[value_size < (DWORD)size ? value_size : (DWORD)size - 1] = 0;
			return text[0] != 0;
		}
	}
	return 0;
}

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
		/* (the pipe's server may only identify this user, not act as them:
		it may be another user's) */
		pipe = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, NULL);
		if (pipe != INVALID_HANDLE_VALUE)
		{
			/* writes never wait (the p2p thread holds its lock): a write takes
			what fits in the pipe */
			DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;

			if (!pipe_server_is_this_user(pipe) || !SetNamedPipeHandleState(pipe, &mode, NULL, NULL))
			{
				CloseHandle(pipe);
				continue;
			}
			discord_pipes[slot] = pipe;
			return slot;
		}
	}
	return -1;
}

int posix_discord_write(int handle, const void *buffer, int length)
{
	DWORD written = 0;

	if (handle < 0 || handle >= MAXIMUM_DISCORD_PIPES || !discord_pipes[handle])
		return -1;
	/* (a pipe that does not wait writes what fits, maybe nothing) */
	if (!WriteFile(discord_pipes[handle], buffer, (DWORD)length, &written, NULL))
		return -1;
	return (int)written;
}

int posix_discord_read(int handle, void *buffer, int length)
{
	DWORD available = 0, read = 0;

	if (handle < 0 || handle >= MAXIMUM_DISCORD_PIPES || !discord_pipes[handle])
		return -1;
	/* only what is already there: a pipe that does not wait fails a read
	of nothing (ERROR_NO_DATA) as if it had closed */
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
