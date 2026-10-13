/*
WIN32_UPDATE.C

The Windows self-updater's system side (port/linux/src/update.h, in place of
posix_update.c): a download over HTTPS with WinHTTP, which checks the
server's certificate against Windows's own certificate store and follows
redirects, and the files of the running game replaced and the game started
again. A running executable and its loaded DLLs can be renamed, not
overwritten: the old files move aside first.

Paths are UTF-8, as SDL gives them.
*/

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <string.h>

#include "update.h"

#define UPDATE_USER_AGENT L"halo-ce-universal-updater"
#define TIMEOUT_MILLISECONDS 20000
/* (posix_update.c's: a release is tens of megabytes, and the largest map of
the index about 760: a body past this fills no disk) */
#define MAXIMUM_DOWNLOAD_SIZE (1024ull * 1024 * 1024)

/* (WinHTTP's TLS 1.3 flag, missing from older SDKs) */
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0x00002000
#endif

static int wide_from_utf8(const char *text, wchar_t *wide, int size)
{
	return MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, size) > 0;
}

static void set_error(char *error, int error_size, const char *what)
{
	DWORD code = GetLastError();

	if (error && error_size > 0)
		snprintf(error, (size_t)error_size, "%s (error %lu)", what, (unsigned long)code);
}

int update_download(const char *url, const char *path, update_progress_proc progress, void *context, char *error,
	int error_size)
{
	return update_request(url, NULL, NULL, NULL, path, progress, context, error, error_size);
}

static int request(const char *url, const char *form, const char *range, const char *agent,
	const volatile int *stop, const char *path, update_progress_proc progress, void *context, char *error,
	int error_size);

int update_request(const char *url, const char *form, const char *agent, const volatile int *stop,
	const char *path, update_progress_proc progress, void *context, char *error, int error_size)
{
	return request(url, form, NULL, agent, stop, path, progress, context, error, error_size);
}

/* (what the body brought, for update_request_range's check) */
static void range_progress(void *context, unsigned long long received, unsigned long long total)
{
	(void)total;
	*(unsigned long long *)context = received;
}

int update_request_range(const char *url, const char *agent, const volatile int *stop, unsigned long long offset,
	unsigned long long length, const char *path, char *error, int error_size)
{
	char range[64];
	unsigned long long received = 0;

	if (!length)
	{
		snprintf(error, (size_t)error_size, "an empty range");
		return 0;
	}
	snprintf(range, sizeof(range), "bytes=%llu-%llu", offset, offset + length - 1);
	if (!request(url, NULL, range, agent, stop, path, range_progress, &received, error, error_size))
		return 0;
	if (received != length)
	{
		wchar_t wide_path[MAX_PATH * 2];

		snprintf(error, (size_t)error_size, "the server sent %llu bytes of the %llu asked for", received, length);
		if (wide_from_utf8(path, wide_path, MAX_PATH * 2))
			DeleteFileW(wide_path);
		return 0;
	}
	return 1;
}

/* one GET (or a POST of form, or a GET of range, "bytes=<first>-<last>",
whose answer must be 206 and the range's length) into the file at path */
static int request(const char *url, const char *form, const char *range, const char *agent,
	const volatile int *stop, const char *path, update_progress_proc progress, void *context, char *error,
	int error_size)
{
	wchar_t wide_url[2048], wide_path[MAX_PATH * 2], host[256], url_path[2048], wide_agent[256];
	wchar_t range_header[96] = { 0 };
	URL_COMPONENTS components;
	HINTERNET session = NULL, connection = NULL, http = NULL;
	HANDLE file = INVALID_HANDLE_VALUE;
	DWORD status = 0, status_size = sizeof(status);
	DWORD length = 0, length_size = sizeof(length);
	DWORD protocols;
	unsigned long long received = 0;
	int succeeded = 0;

	if (!wide_from_utf8(url, wide_url, 2048) || !wide_from_utf8(path, wide_path, MAX_PATH * 2))
	{
		snprintf(error, (size_t)error_size, "a bad address or path");
		return 0;
	}
	memset(&components, 0, sizeof(components));
	components.dwStructSize = sizeof(components);
	components.lpszHostName = host;
	components.dwHostNameLength = 256;
	components.lpszUrlPath = url_path;
	components.dwUrlPathLength = 2048;
	if (!WinHttpCrackUrl(wide_url, 0, 0, &components) || components.nScheme != INTERNET_SCHEME_HTTPS)
	{
		snprintf(error, (size_t)error_size, "not an https:// address: %s", url);
		return 0;
	}
	if (!agent || !wide_from_utf8(agent, wide_agent, 256))
		wcscpy(wide_agent, UPDATE_USER_AGENT);
	session = WinHttpOpen(wide_agent, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session)
	{
		set_error(error, error_size, "could not start WinHTTP");
		goto done;
	}
	/* (TLS 1.2 and 1.3; Windows versions without 1.3 take 1.2) */
	protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
	if (!WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols)))
	{
		protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
		WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
	}
	WinHttpSetTimeouts(session, TIMEOUT_MILLISECONDS, TIMEOUT_MILLISECONDS, TIMEOUT_MILLISECONDS,
		TIMEOUT_MILLISECONDS);
	connection = WinHttpConnect(session, host, components.nPort, 0);
	if (connection)
	{
		http = WinHttpOpenRequest(connection, form ? L"POST" : L"GET", url_path, NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	}
	if (range)
		_snwprintf(range_header, sizeof(range_header) / sizeof(*range_header) - 1, L"Range: %hs\r\n", range);
	/* (a form's answer, redirected, is fetched with a GET: WinHTTP's own
	redirect policy) */
	if (!http ||
		!WinHttpSendRequest(http,
			form ? L"Content-Type: application/x-www-form-urlencoded\r\n" : range ? range_header :
				WINHTTP_NO_ADDITIONAL_HEADERS,
			form || range ? (DWORD)-1L : 0, form ? (LPVOID)form : WINHTTP_NO_REQUEST_DATA,
			form ? (DWORD)strlen(form) : 0, form ? (DWORD)strlen(form) : 0, 0) ||
		!WinHttpReceiveResponse(http, NULL))
	{
		set_error(error, error_size, "could not reach the server");
		goto done;
	}
	if (!WinHttpQueryHeaders(http, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX) ||
		status != (range ? 206u : 200u))
	{
		if (range && status == 200)
			snprintf(error, (size_t)error_size, "the server sends no part of a file, only the whole");
		else
			snprintf(error, (size_t)error_size, "the server answered %lu", (unsigned long)status);
		goto done;
	}
	/* (a range's answer must be that range: "bytes <first>-<last>/<size>") */
	if (range)
	{
		wchar_t content_range[128];
		DWORD content_range_size = sizeof(content_range);
		char expected[96], got[128];

		snprintf(expected, sizeof(expected), "bytes %s/", range + strlen("bytes="));
		if (!WinHttpQueryHeaders(http, WINHTTP_QUERY_CONTENT_RANGE, WINHTTP_HEADER_NAME_BY_INDEX, content_range,
				&content_range_size, WINHTTP_NO_HEADER_INDEX) ||
			WideCharToMultiByte(CP_UTF8, 0, content_range, -1, got, (int)sizeof(got), NULL, NULL) <= 0 ||
			strncmp(got, expected, strlen(expected)) != 0)
		{
			snprintf(error, (size_t)error_size, "the server sent another part of the file");
			goto done;
		}
	}
	if (!WinHttpQueryHeaders(http, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &length, &length_size, WINHTTP_NO_HEADER_INDEX))
	{
		length = 0;
	}
	file = CreateFileW(wide_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		set_error(error, error_size, "could not write the download");
		goto done;
	}
	for (;;)
	{
		char buffer[16384];
		DWORD count = 0, written = 0;

		if (stop && *stop)
		{
			snprintf(error, (size_t)error_size, "stopped");
			goto done;
		}
		if (!WinHttpReadData(http, buffer, sizeof(buffer), &count))
		{
			set_error(error, error_size, "the download broke off");
			goto done;
		}
		if (!count)
			break;
		if (!WriteFile(file, buffer, count, &written, NULL) || written != count)
		{
			set_error(error, error_size, "could not write the download");
			goto done;
		}
		received += count;
		if (received > MAXIMUM_DOWNLOAD_SIZE)
		{
			snprintf(error, (size_t)error_size, "the download is too large");
			goto done;
		}
		if (progress)
			progress(context, received, length);
	}
	if (length && received != length)
	{
		snprintf(error, (size_t)error_size, "the download broke off");
		goto done;
	}
	succeeded = 1;

done:
	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		if (!succeeded)
			DeleteFileW(wide_path);
	}
	if (http)
		WinHttpCloseHandle(http);
	if (connection)
		WinHttpCloseHandle(connection);
	if (session)
		WinHttpCloseHandle(session);
	return succeeded;
}

/* ---------- files and processes */

int update_executable_path(char *path, int size)
{
	wchar_t wide[MAX_PATH * 2];
	DWORD length = GetModuleFileNameW(NULL, wide, MAX_PATH * 2);

	return length > 0 && length < MAX_PATH * 2 &&
		WideCharToMultiByte(CP_UTF8, 0, wide, -1, path, size, NULL, NULL) > 0;
}

int update_replace_file(const char *path, const char *new_path, const char *old_path)
{
	wchar_t wide_path[MAX_PATH * 2], wide_new[MAX_PATH * 2], wide_old[MAX_PATH * 2];
	int existed;

	if (!wide_from_utf8(path, wide_path, MAX_PATH * 2) || !wide_from_utf8(new_path, wide_new, MAX_PATH * 2) ||
		!wide_from_utf8(old_path, wide_old, MAX_PATH * 2))
	{
		return 0;
	}
	DeleteFileW(wide_old);
	existed = GetFileAttributesW(wide_path) != INVALID_FILE_ATTRIBUTES;
	if (existed && !MoveFileExW(wide_path, wide_old, MOVEFILE_REPLACE_EXISTING))
		return 0;
	if (!MoveFileExW(wide_new, wide_path, MOVEFILE_REPLACE_EXISTING))
	{
		if (existed)
			MoveFileExW(wide_old, wide_path, MOVEFILE_REPLACE_EXISTING);
		return 0;
	}
	return 1;
}

void update_delete_file(const char *path)
{
	wchar_t wide[MAX_PATH * 2];

	if (wide_from_utf8(path, wide, MAX_PATH * 2) && !DeleteFileW(wide))
		RemoveDirectoryW(wide);
}

int update_make_directory(const char *path)
{
	wchar_t wide[MAX_PATH * 2];

	return wide_from_utf8(path, wide, MAX_PATH * 2) &&
		(CreateDirectoryW(wide, NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
}

int update_launch(const char *path)
{
	wchar_t wide[MAX_PATH * 2], command[MAX_PATH * 2 + 3], directory[MAX_PATH * 2];
	wchar_t *slash;
	STARTUPINFOW startup;
	PROCESS_INFORMATION process;

	if (!wide_from_utf8(path, wide, MAX_PATH * 2))
		return 0;
	_snwprintf(command, MAX_PATH * 2 + 3, L"\"%ls\"", wide);
	wcscpy(directory, wide);
	slash = wcsrchr(directory, L'\\');
	if (slash)
		*slash = 0;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	/* (none of this process's handles: its sockets hold the game's ports) */
	if (!CreateProcessW(wide, command, NULL, NULL, FALSE, 0, NULL, slash ? directory : NULL, &startup, &process))
		return 0;
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return 1;
}
