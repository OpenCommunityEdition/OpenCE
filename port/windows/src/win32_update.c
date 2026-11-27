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
#include <stdlib.h>
#include <string.h>

#include "update.h"

#define UPDATE_USER_AGENT L"halo-ce-universal-updater"
#define TIMEOUT_MILLISECONDS 20000

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

int update_download(const char *url, const char *path, unsigned long long maximum, update_progress_proc progress,
	void *context, char *error, int error_size)
{
	wchar_t wide_url[2048], wide_path[MAX_PATH * 2], host[256], url_path[2048];
	URL_COMPONENTS components;
	HINTERNET session = NULL, connection = NULL, request = NULL;
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
	session = WinHttpOpen(UPDATE_USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
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
		request = WinHttpOpenRequest(connection, L"GET", url_path, NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	}
	if (!request || !WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
		!WinHttpReceiveResponse(request, NULL))
	{
		set_error(error, error_size, "could not reach the server");
		goto done;
	}
	if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX) || status != 200)
	{
		snprintf(error, (size_t)error_size, "the server answered %lu", (unsigned long)status);
		goto done;
	}
	if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &length, &length_size, WINHTTP_NO_HEADER_INDEX))
	{
		length = 0;
	}
	if (length > maximum)
	{
		snprintf(error, (size_t)error_size, "the download is larger than expected");
		goto done;
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

		if (!WinHttpReadData(request, buffer, sizeof(buffer), &count))
		{
			set_error(error, error_size, "the download broke off");
			goto done;
		}
		if (!count)
			break;
		/* (no more than the caller expects: a body that goes on is refused) */
		if (count > maximum - received)
		{
			snprintf(error, (size_t)error_size, "the download is larger than expected");
			goto done;
		}
		if (!WriteFile(file, buffer, count, &written, NULL) || written != count)
		{
			set_error(error, error_size, "could not write the download");
			goto done;
		}
		received += count;
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
	if (request)
		WinHttpCloseHandle(request);
	if (connection)
		WinHttpCloseHandle(connection);
	if (session)
		WinHttpCloseHandle(session);
	return succeeded;
}

/* ---------- any request */

int update_https_request(const char *method, const char *url, const char *headers, const void *body,
	unsigned long body_size, unsigned long maximum, struct update_response *response, char *error, int error_size)
{
	wchar_t wide_url[2048], host[256], url_path[2048], wide_method[16];
	wchar_t *wide_headers = NULL;
	URL_COMPONENTS components;
	HINTERNET session = NULL, connection = NULL, request = NULL;
	DWORD status = 0, status_size = sizeof(status);
	DWORD protocols, flags = 0;
	char *data = NULL;
	unsigned long received = 0, capacity = 0;
	int succeeded = 0;

	memset(response, 0, sizeof(*response));
	if (!wide_from_utf8(url, wide_url, 2048) || !wide_from_utf8(method, wide_method, 16))
	{
		snprintf(error, (size_t)error_size, "a bad address");
		return 0;
	}
	memset(&components, 0, sizeof(components));
	components.dwStructSize = sizeof(components);
	components.lpszHostName = host;
	components.dwHostNameLength = 256;
	components.lpszUrlPath = url_path;
	components.dwUrlPathLength = 2048;
	if (!WinHttpCrackUrl(wide_url, 0, 0, &components) ||
		(components.nScheme != INTERNET_SCHEME_HTTPS &&
			!(components.nScheme == INTERNET_SCHEME_HTTP && (!wcscmp(host, L"127.0.0.1") || !_wcsicmp(host, L"localhost")))))
	{
		snprintf(error, (size_t)error_size, "not an https:// address: %s", url);
		return 0;
	}
	if (components.nScheme == INTERNET_SCHEME_HTTPS)
		flags = WINHTTP_FLAG_SECURE;
	if (headers && headers[0])
	{
		int length = MultiByteToWideChar(CP_UTF8, 0, headers, -1, NULL, 0);

		wide_headers = length > 0 ? malloc(sizeof(wchar_t) * (size_t)length) : NULL;
		if (!wide_headers || !MultiByteToWideChar(CP_UTF8, 0, headers, -1, wide_headers, length))
		{
			free(wide_headers);
			snprintf(error, (size_t)error_size, "bad headers");
			return 0;
		}
	}
	session = WinHttpOpen(L"opence", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session)
	{
		set_error(error, error_size, "could not start WinHTTP");
		goto done;
	}
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
		request = WinHttpOpenRequest(connection, wide_method, url_path, NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (request)
	{
		/* (no redirect followed: a master's answer is its own) */
		DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;

		WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
	}
	if (!request || !WinHttpSendRequest(request, wide_headers ? wide_headers : WINHTTP_NO_ADDITIONAL_HEADERS,
		wide_headers ? (DWORD)-1L : 0, (void *)body, body ? (DWORD)body_size : 0, body ? (DWORD)body_size : 0, 0) ||
		!WinHttpReceiveResponse(request, NULL))
	{
		set_error(error, error_size, "could not reach the server");
		goto done;
	}
	if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX))
	{
		set_error(error, error_size, "no answer from the server");
		goto done;
	}
	response->status = (int)status;
	{
		wchar_t raw[4096];
		DWORD raw_size = sizeof(raw);

		/* (the headers after the status line, one a line) */
		if (WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, raw, &raw_size,
			WINHTTP_NO_HEADER_INDEX))
		{
			char text[4096];
			int read, written = 0;
			const char *first;

			WideCharToMultiByte(CP_UTF8, 0, raw, -1, text, sizeof(text), NULL, NULL);
			text[sizeof(text) - 1] = 0;
			first = strstr(text, "\r\n");
			for (read = first ? (int)(first - text) + 2 : 0; text[read] && written < (int)sizeof(response->headers) - 1;
				read++)
			{
				if (text[read] != '\r')
					response->headers[written++] = text[read];
			}
			response->headers[written] = 0;
		}
	}
	for (;;)
	{
		char buffer[16384];
		DWORD count = 0;

		if (!WinHttpReadData(request, buffer, sizeof(buffer), &count))
		{
			set_error(error, error_size, "the answer broke off");
			goto done;
		}
		if (!count)
			break;
		if (received + count > maximum)
		{
			snprintf(error, (size_t)error_size, "the answer is larger than %lu bytes", maximum);
			goto done;
		}
		if (received + count + 1 > capacity)
		{
			unsigned long grown_capacity = capacity ? capacity * 2 : 16384;
			char *grown;

			while (grown_capacity < received + count + 1)
				grown_capacity *= 2;
			grown = realloc(data, grown_capacity);
			if (!grown)
				goto done;
			data = grown;
			capacity = grown_capacity;
		}
		memcpy(data + received, buffer, count);
		received += count;
		data[received] = 0;
	}
	response->body = data ? data : calloc(1, 1);
	response->body_size = received;
	data = NULL;
	succeeded = 1;

done:
	free(data);
	free(wide_headers);
	if (request)
		WinHttpCloseHandle(request);
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
