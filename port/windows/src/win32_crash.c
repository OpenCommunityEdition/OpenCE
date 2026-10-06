/*
WIN32_CRASH.C

Crash reports. The game's own __try handler cannot catch a crash
(port/windows/include/halo_windows_prefix.h); an unhandled exception filter
does instead. It writes a backtrace of the EBP frame chain into debug.txt and
the log, as before, and then has a second copy of halo.exe, the crash
reporter ("halo.exe --crash-report <process> <thread> <exception pointers>"),
write a minidump of the crashed game. A crashed process cannot be trusted to
dump itself (its stack, heap or locks may be what broke), so the reporter
reads it from outside, as breakpad and crashpad do. The game waits for the
dump and then ends, without Windows's own crash dialog.

The reporter waits for the game to close, then, at the first crash, asks the
player whether to send crash reports. config.toml keeps the answer
(crash_reports.upload): "yes" sends this report and every later one without
asking, "no" sends none and never asks again. A report is the minidump and
a copy of halo.log, sent to the minidump endpoint of the project's Sentry.
Sentry turns the minidump into a backtrace with function names and lines
from halo.pdb, which the workflow uploads for each build of main
(.github/workflows/build.yml). Reports wait in crashes\ beside halo.exe until
they are sent; one that could not be sent (no network) goes the next time
the game starts ("halo.exe --crash-upload", in the background).

Only numbered builds (the workflow's builds of main, whose symbols Sentry
has) report crashes; the HALO_CRASH_REPORTS_ANY_BUILD environment variable
makes any build report (for testing). Abort() and the C runtime's invalid
parameter checks, which end the process without an exception, raise one
here, so they are reported too.
*/

#include <windows.h>
#include <dbghelp.h>
#include <winhttp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "port_config.h"

/* (tools/windows_build.py gives them, as to updater.c) */
#ifndef HALO_BUILD_NUMBER
#define HALO_BUILD_NUMBER 0
#endif
#ifndef HALO_BUILD_FLAVOR
#define HALO_BUILD_FLAVOR "release"
#endif

/* the minidump endpoint of the project's DSN,
https://e656c596e8f90402b0f47a2613b69e50@o4512207906603008.ingest.de.sentry.io/4512207917744208
(a DSN is public: it only lets a client send events) */
#define SENTRY_HOST L"o4512207906603008.ingest.de.sentry.io"
#define SENTRY_MINIDUMP_PATH L"/api/4512207917744208/minidump/?sentry_key=e656c596e8f90402b0f47a2613b69e50"
#define SENTRY_USER_AGENT L"halo-ce-universal-crash-reporter"

#define CRASH_REPORT_OPTION L"--crash-report"
#define CRASH_UPLOAD_OPTION L"--crash-upload"
#define CRASH_FOLDER L"crashes"
#define CRASH_UPLOAD_LOCK L"Local\\halo-crash-upload"

/* the exceptions abort() and the invalid parameter checks raise (bit 29:
an application's own code) */
#define CRASH_ABORT_EXCEPTION 0xE0000001UL
#define CRASH_INVALID_PARAMETER_EXCEPTION 0xE0000002UL

#define MAXIMUM_PENDING_REPORTS 8
#define MAXIMUM_LOG_SIZE (512 * 1024)
#define MAXIMUM_DUMP_SIZE (32 * 1024 * 1024)
#define MAXIMUM_REPORTER_LOG_SIZE (64 * 1024)
#define DUMP_WAIT_MILLISECONDS 60000
#define EXIT_WAIT_MILLISECONDS 10000
#define CONSENT_WAIT_MILLISECONDS (10 * 60 * 1000)
#define UPLOAD_LOCK_WAIT_MILLISECONDS (5 * 60 * 1000)
#define UPLOAD_TIMEOUT_MILLISECONDS 30000
#define PATH_SIZE (MAX_PATH * 2)

/* (WinHTTP's TLS 1.3 flag, missing from older SDKs) */
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0x00002000
#endif

void platform_log(const char *format, ...);
/* errors.c's: debug.txt, whose first lines (its reference address) place
the addresses here in the build */
void write_to_error_file(char *string, unsigned char date);

/* ---------- files */

/* the folder of halo.exe (and of config.toml), with its separator */
static int crash_executable_folder(wchar_t *path, size_t size)
{
	DWORD length = GetModuleFileNameW(NULL, path, (DWORD)size);
	wchar_t *slash;

	if (!length || length >= size || !(slash = wcsrchr(path, L'\\')))
		return 0;
	slash[1] = 0;
	return 1;
}

/* crashes\ beside halo.exe, with its separator */
static int crash_folder(wchar_t *path, size_t size)
{
	if (!crash_executable_folder(path, size) || wcslen(path) + wcslen(CRASH_FOLDER) + 2 > size)
		return 0;
	wcscat(path, CRASH_FOLDER L"\\");
	return 1;
}

static int crash_path(wchar_t *path, size_t size, const wchar_t *folder, const wchar_t *name, const wchar_t *extension)
{
	int length = _snwprintf(path, size, L"%ls%ls%ls", folder, name, extension);

	return length > 0 && (size_t)length < size;
}

/* a whole file, or at most its last maximum bytes; NULL if there is none.
free() it */
static char *crash_read_file(const wchar_t *path, size_t maximum, DWORD *size)
{
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	LARGE_INTEGER length, offset;
	char *data = NULL;
	DWORD read = 0;

	if (file == INVALID_HANDLE_VALUE)
		return NULL;
	if (GetFileSizeEx(file, &length) && length.QuadPart >= 0)
	{
		offset.QuadPart = length.QuadPart > (LONGLONG)maximum ? length.QuadPart - (LONGLONG)maximum : 0;
		length.QuadPart -= offset.QuadPart;
		data = malloc((size_t)length.QuadPart + 1);
		if (data && (!SetFilePointerEx(file, offset, NULL, FILE_BEGIN) ||
			!ReadFile(file, data, (DWORD)length.QuadPart, &read, NULL) || read != (DWORD)length.QuadPart))
		{
			free(data);
			data = NULL;
		}
	}
	CloseHandle(file);
	if (data)
	{
		data[read] = 0;
		*size = read;
	}
	return data;
}

static int crash_write_file(const wchar_t *path, const char *data, DWORD size)
{
	HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD written = 0;
	int succeeded;

	if (file == INVALID_HANDLE_VALUE)
		return 0;
	succeeded = WriteFile(file, data, size, &written, NULL) && written == size;
	CloseHandle(file);
	if (!succeeded)
		DeleteFileW(path);
	return succeeded;
}

/* the reports waiting in the folder: the names of their minidumps without
".dmp", at most count of them; how many there are */
static int crash_pending_reports(const wchar_t *folder, wchar_t names[][MAX_PATH], int count)
{
	wchar_t pattern[PATH_SIZE];
	WIN32_FIND_DATAW found;
	HANDLE search;
	int pending = 0;

	if (!crash_path(pattern, PATH_SIZE, folder, L"*", L".dmp"))
		return 0;
	search = FindFirstFileW(pattern, &found);
	if (search == INVALID_HANDLE_VALUE)
		return 0;
	do
	{
		size_t length = wcslen(found.cFileName);

		if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || length <= 4 || length >= MAX_PATH)
			continue;
		if (names && pending < count)
		{
			wcscpy(names[pending], found.cFileName);
			names[pending][length - 4] = 0;
		}
		pending++;
	}
	while (FindNextFileW(search, &found));
	FindClose(search);
	return pending;
}

static void crash_delete_report(const wchar_t *folder, const wchar_t *name)
{
	wchar_t path[PATH_SIZE];

	if (crash_path(path, PATH_SIZE, folder, name, L".dmp"))
		DeleteFileW(path);
	if (crash_path(path, PATH_SIZE, folder, name, L".log"))
		DeleteFileW(path);
}

/* ---------- the upload */

struct crash_body
{
	char *data;
	size_t length;
	size_t size;
	int failed;
};

#define CRASH_BOUNDARY "halo-crash-report-3f9c1e7a52d8b406"

static void body_append(struct crash_body *body, const void *data, size_t length)
{
	if (body->failed)
		return;
	if (body->length + length > body->size)
	{
		size_t size = (body->length + length) * 2 + 4096;
		char *grown = realloc(body->data, size);

		if (!grown)
		{
			body->failed = 1;
			return;
		}
		body->data = grown;
		body->size = size;
	}
	memcpy(body->data + body->length, data, length);
	body->length += length;
}

static void body_text(struct crash_body *body, const char *text)
{
	body_append(body, text, strlen(text));
}

static void body_field(struct crash_body *body, const char *name, const char *value)
{
	body_text(body, "--" CRASH_BOUNDARY "\r\nContent-Disposition: form-data; name=\"");
	body_text(body, name);
	body_text(body, "\"\r\n\r\n");
	body_text(body, value);
	body_text(body, "\r\n");
}

static void body_file(struct crash_body *body, const char *name, const char *file_name, const char *type,
	const char *data, size_t length)
{
	body_text(body, "--" CRASH_BOUNDARY "\r\nContent-Disposition: form-data; name=\"");
	body_text(body, name);
	body_text(body, "\"; filename=\"");
	body_text(body, file_name);
	body_text(body, "\"\r\nContent-Type: ");
	body_text(body, type);
	body_text(body, "\r\n\r\n");
	body_append(body, data, length);
	body_text(body, "\r\n");
}

/* POSTs the body to Sentry: the HTTP status (0 for no answer), and the
answer's start (the event's id) */
static DWORD crash_post(const struct crash_body *body, char *answer, DWORD answer_size)
{
	HINTERNET session, connection = NULL, request = NULL;
	DWORD status = 0, status_size = sizeof(status), protocols, received = 0;

	answer[0] = 0;
	session = WinHttpOpen(SENTRY_USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session)
		return 0;
	/* (TLS 1.2 and 1.3; Windows versions without 1.3 take 1.2) */
	protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
	if (!WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols)))
	{
		protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
		WinHttpSetOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
	}
	WinHttpSetTimeouts(session, UPLOAD_TIMEOUT_MILLISECONDS, UPLOAD_TIMEOUT_MILLISECONDS,
		UPLOAD_TIMEOUT_MILLISECONDS, UPLOAD_TIMEOUT_MILLISECONDS);
	connection = WinHttpConnect(session, SENTRY_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (connection)
	{
		request = WinHttpOpenRequest(connection, L"POST", SENTRY_MINIDUMP_PATH, NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	}
	if (request &&
		WinHttpSendRequest(request, L"Content-Type: multipart/form-data; boundary=" CRASH_BOUNDARY, (DWORD)-1L,
			body->data, (DWORD)body->length, (DWORD)body->length, 0) &&
		WinHttpReceiveResponse(request, NULL) &&
		WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX))
	{
		DWORD count = 0;

		while (received + 1 < answer_size &&
			WinHttpReadData(request, answer + received, answer_size - 1 - received, &count) && count)
		{
			received += count;
		}
		answer[received] = 0;
		answer[strcspn(answer, "\r\n")] = 0;
	}
	else
	{
		platform_log("crash report: could not reach %ls (error %lu)", SENTRY_HOST, (unsigned long)GetLastError());
	}
	if (request)
		WinHttpCloseHandle(request);
	if (connection)
		WinHttpCloseHandle(connection);
	WinHttpCloseHandle(session);
	return status;
}

/* sends one report; 1 when it is done with (sent, or refused for good) */
static int crash_upload(const wchar_t *folder, const wchar_t *name)
{
	typedef const char *(CDECL *wine_get_version_proc)(void);
	wchar_t path[PATH_SIZE];
	const wchar_t *build_text = wcsstr(name, L"-build");
	char release[32] = "build-0", environment[16] = "release", file_name[MAX_PATH * 3], answer[256];
	struct crash_body body = { 0 };
	char *dump, *log = NULL;
	DWORD dump_size = 0, log_size = 0, status;
	wine_get_version_proc wine_get_version;
	int number = 0;
	wchar_t flavor[16] = L"";

	/* the build that crashed, from the name: <time>-<process>-build<number>-<flavor> */
	if (build_text && swscanf(build_text, L"-build%d-%15ls", &number, flavor) >= 1)
	{
		snprintf(release, sizeof(release), "build-%d", number);
		if (flavor[0])
			snprintf(environment, sizeof(environment), "%ls", flavor);
	}
	if (!crash_path(path, PATH_SIZE, folder, name, L".dmp") || !(dump = crash_read_file(path, MAXIMUM_DUMP_SIZE + 1,
		&dump_size)))
	{
		return 1;
	}
	if (dump_size > MAXIMUM_DUMP_SIZE)
	{
		platform_log("crash report: %ls.dmp is too large to send", name);
		free(dump);
		return 1;
	}
	if (crash_path(path, PATH_SIZE, folder, name, L".log"))
		log = crash_read_file(path, MAXIMUM_LOG_SIZE, &log_size);
	snprintf(file_name, sizeof(file_name), "%ls.dmp", name);

	body_field(&body, "sentry[release]", release);
	body_field(&body, "sentry[environment]", environment);
	/* (Proton and Wine players' crashes are often Wine's) */
	wine_get_version = (wine_get_version_proc)(void *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"),
		"wine_get_version");
	body_field(&body, "sentry[tags][wine]", wine_get_version ? wine_get_version() : "no");
	body_file(&body, "upload_file_minidump", file_name, "application/octet-stream", dump, dump_size);
	/* (another file of the form is an attachment of the event) */
	if (log)
		body_file(&body, "halo.log", "halo.log", "text/plain", log, log_size);
	body_text(&body, "--" CRASH_BOUNDARY "--\r\n");
	free(dump);
	free(log);
	if (body.failed)
	{
		free(body.data);
		return 0;
	}
	status = crash_post(&body, answer, sizeof(answer));
	free(body.data);
	if (status == 200)
	{
		platform_log("crash report: sent %ls (%s, %s): %s", name, release, environment, answer);
		return 1;
	}
	platform_log("crash report: Sentry answered %lu to %ls: %s", (unsigned long)status, name, answer);
	/* (no answer, a server error or too many reports: again later; any other
	refusal would be the same the next time) */
	return status >= 400 && status < 500 && status != 408 && status != 429;
}

/* sends the reports waiting in the folder, one process at a time */
static void crash_upload_pending(const wchar_t *folder)
{
	wchar_t names[MAXIMUM_PENDING_REPORTS * 2][MAX_PATH];
	HANDLE lock = CreateMutexW(NULL, FALSE, CRASH_UPLOAD_LOCK);
	int count, index;

	if (lock)
		WaitForSingleObject(lock, UPLOAD_LOCK_WAIT_MILLISECONDS);
	count = crash_pending_reports(folder, names, MAXIMUM_PENDING_REPORTS * 2);
	if (count > MAXIMUM_PENDING_REPORTS * 2)
		count = MAXIMUM_PENDING_REPORTS * 2;
	for (index = 0; index < count; index++)
	{
		if (crash_upload(folder, names[index]))
			crash_delete_report(folder, names[index]);
	}
	if (lock)
	{
		ReleaseMutex(lock);
		CloseHandle(lock);
	}
}

/* ---------- the reporter */

/* a module's CodeView record (its PDB's id and name), from its image in the
crashed game; its size, or 0 */
static DWORD crash_codeview_record(HANDLE process, ULONG64 base, char *record, DWORD size)
{
	IMAGE_DOS_HEADER dos;
	IMAGE_NT_HEADERS32 headers;
	IMAGE_DATA_DIRECTORY debug;
	DWORD index;

	if (!ReadProcessMemory(process, (void *)(ULONG_PTR)base, &dos, sizeof(dos), NULL) ||
		dos.e_magic != IMAGE_DOS_SIGNATURE ||
		!ReadProcessMemory(process, (void *)(ULONG_PTR)(base + (ULONG64)dos.e_lfanew), &headers, sizeof(headers),
			NULL) ||
		headers.Signature != IMAGE_NT_SIGNATURE || headers.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
		headers.OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_DEBUG)
	{
		return 0;
	}
	debug = headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
	for (index = 0; index < debug.Size / sizeof(IMAGE_DEBUG_DIRECTORY) && index < 16; index++)
	{
		IMAGE_DEBUG_DIRECTORY entry;

		if (!ReadProcessMemory(process, (void *)(ULONG_PTR)(base + debug.VirtualAddress +
			index * sizeof(IMAGE_DEBUG_DIRECTORY)), &entry, sizeof(entry), NULL))
		{
			return 0;
		}
		/* (an "RSDS" record: signature, GUID, age and the PDB's name) */
		if (entry.Type == IMAGE_DEBUG_TYPE_CODEVIEW && entry.AddressOfRawData && entry.SizeOfData >= 25 &&
			entry.SizeOfData <= size &&
			ReadProcessMemory(process, (void *)(ULONG_PTR)(base + entry.AddressOfRawData), record, entry.SizeOfData,
				NULL) &&
			!memcmp(record, "RSDS", 4))
		{
			return entry.SizeOfData;
		}
	}
	return 0;
}

/* Wine's MiniDumpWriteDump leaves out the modules' CodeView records, which
Sentry finds their symbols by (Windows's own writes them): the minidump
gets them from the crashed game's images */
static void crash_add_codeview_records(HANDLE process, const wchar_t *path)
{
	struct crash_body dump = { 0 };
	MINIDUMP_HEADER header;
	DWORD size = 0, stream;
	char *data = crash_read_file(path, MAXIMUM_DUMP_SIZE, &size);
	int added = 0;

	if (!data)
		return;
	body_append(&dump, data, size);
	free(data);
	if (dump.failed || size < sizeof(header))
		goto done;
	memcpy(&header, dump.data, sizeof(header));
	if (header.Signature != MINIDUMP_SIGNATURE || header.NumberOfStreams > 256 ||
		header.StreamDirectoryRva > size || header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY) >
		size - header.StreamDirectoryRva)
	{
		goto done;
	}
	for (stream = 0; stream < header.NumberOfStreams; stream++)
	{
		MINIDUMP_DIRECTORY directory;
		ULONG32 count, module;

		memcpy(&directory, dump.data + header.StreamDirectoryRva + stream * sizeof(directory), sizeof(directory));
		if (directory.StreamType != ModuleListStream || directory.Location.Rva > size - sizeof(count))
			continue;
		memcpy(&count, dump.data + directory.Location.Rva, sizeof(count));
		if (count > (size - directory.Location.Rva - sizeof(count)) / sizeof(MINIDUMP_MODULE))
			continue;
		for (module = 0; module < count; module++)
		{
			size_t offset = directory.Location.Rva + sizeof(count) + module * sizeof(MINIDUMP_MODULE);
			MINIDUMP_MODULE entry;
			char record[1024];
			DWORD length;

			memcpy(&entry, dump.data + offset, sizeof(entry));
			if (entry.CvRecord.DataSize ||
				!(length = crash_codeview_record(process, entry.BaseOfImage, record, sizeof(record))))
			{
				continue;
			}
			entry.CvRecord.Rva = (RVA)dump.length;
			entry.CvRecord.DataSize = length;
			body_append(&dump, record, length);
			body_append(&dump, "\0\0\0", (4 - length % 4) % 4);
			if (dump.failed)
				goto done;
			memcpy(dump.data + offset, &entry, sizeof(entry));
			added++;
		}
	}
	if (added && crash_write_file(path, dump.data, (DWORD)dump.length))
		platform_log("crash report: added the CodeView records of %d modules", added);

done:
	free(dump.data);
}

static const char *crash_consent(void)
{
	return config_string("crash_reports.upload");
}

/* asks the player: 1 yes, 0 no, -1 no answer (the box went unanswered, as
it can behind a fullscreen window under gamescope) */
static int crash_ask(void)
{
	typedef int (WINAPI *message_box_timeout_proc)(HWND, LPCWSTR, LPCWSTR, UINT, WORD, DWORD);
	static const wchar_t text[] =
		L"Halo crashed.\n\n"
		L"Do you want to send crash reports to the developers? They help us find and fix crashes.\n\n"
		L"A report holds the state of the game when it crashed (the call stacks and registers of its threads, "
		L"and the list of its program files) and its log, halo.log. Reports go to Sentry (sentry.io).\n\n"
		L"The answer is kept in config.toml (crash_reports.upload): Yes sends the report of this crash and of "
		L"every later one, No never sends one.";
	/* (user32's MessageBoxTimeoutW, which Windows and Wine have and do not
	document) */
	message_box_timeout_proc message_box_timeout =
		(message_box_timeout_proc)(void *)GetProcAddress(GetModuleHandleW(L"user32.dll"), "MessageBoxTimeoutW");
	UINT style = MB_YESNO | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND;
	int answer = message_box_timeout ? message_box_timeout(NULL, text, L"Halo crashed", style, 0,
		CONSENT_WAIT_MILLISECONDS) : MessageBoxW(NULL, text, L"Halo crashed", style);

	return answer == IDYES ? 1 : answer == IDNO ? 0 : -1;
}

/* the reporter's own log, crashes\reporter.log (the game's goes to halo.log,
which this process must leave as the game wrote it) */
static void crash_reporter_log(const wchar_t *folder)
{
	wchar_t path[PATH_SIZE];
	WIN32_FILE_ATTRIBUTE_DATA attributes;
	int full;

	if (!crash_path(path, PATH_SIZE, folder, L"reporter", L".log"))
		return;
	/* (added to, until it grows past its limit) */
	full = GetFileAttributesExW(path, GetFileExInfoStandard, &attributes) &&
		(attributes.nFileSizeHigh || attributes.nFileSizeLow > MAXIMUM_REPORTER_LOG_SIZE);
	if (_wfreopen(path, full ? L"w" : L"a", stderr))
		setvbuf(stderr, NULL, _IONBF, 0);
}

/* a copy of the crashed game's halo.log beside its minidump, if the game
wrote it (a debug build logs to its console instead, and leaves an older
halo.log) */
static void crash_copy_log(HANDLE process, const wchar_t *report_log)
{
	wchar_t path[PATH_SIZE];
	FILETIME created, exited, kernel, user;
	WIN32_FILE_ATTRIBUTE_DATA attributes;
	DWORD size = 0;
	char *log;

	if (!crash_executable_folder(path, PATH_SIZE) || wcslen(path) + sizeof("halo.log") > PATH_SIZE)
		return;
	wcscat(path, L"halo.log");
	if (!GetProcessTimes(process, &created, &exited, &kernel, &user) ||
		!GetFileAttributesExW(path, GetFileExInfoStandard, &attributes) ||
		CompareFileTime(&attributes.ftLastWriteTime, &created) < 0)
	{
		return;
	}
	log = crash_read_file(path, MAXIMUM_LOG_SIZE, &size);
	if (log)
	{
		crash_write_file(report_log, log, size);
		free(log);
	}
}

static void crash_signal_dumped(DWORD process_id)
{
	wchar_t name[64];
	HANDLE event;

	_snwprintf(name, 64, L"Local\\halo-crash-dumped-%lu", (unsigned long)process_id);
	event = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
	if (event)
	{
		SetEvent(event);
		CloseHandle(event);
	}
}

/* "halo.exe --crash-report": dumps the crashed game, asks the first time,
and sends */
static void crash_reporter(DWORD process_id, DWORD thread_id, ULONG_PTR exception_pointers)
{
	wchar_t folder[PATH_SIZE], name[MAX_PATH], dump[PATH_SIZE], log[PATH_SIZE];
	MINIDUMP_EXCEPTION_INFORMATION exception;
	SYSTEMTIME now;
	HANDLE process, file;
	const char *consent;
	BOOL dumped;
	int answer;

	if (!crash_folder(folder, PATH_SIZE))
		return;
	CreateDirectoryW(folder, NULL);
	crash_reporter_log(folder);
	consent = crash_consent();
	if (!strcmp(consent, "no"))
		return;
	if (crash_pending_reports(folder, NULL, 0) >= MAXIMUM_PENDING_REPORTS)
	{
		platform_log("crash report: %d reports are waiting to be sent already; no new one", MAXIMUM_PENDING_REPORTS);
		return;
	}
	process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE | SYNCHRONIZE, FALSE,
		process_id);
	if (!process)
	{
		platform_log("crash report: cannot open the game's process (error %lu)", (unsigned long)GetLastError());
		return;
	}
	GetSystemTime(&now);
	_snwprintf(name, MAX_PATH, L"%04u%02u%02u-%02u%02u%02u-%lu-build%d-%hs", now.wYear, now.wMonth, now.wDay,
		now.wHour, now.wMinute, now.wSecond, (unsigned long)process_id, HALO_BUILD_NUMBER, HALO_BUILD_FLAVOR);
	name[MAX_PATH - 1] = 0;
	if (!crash_path(dump, PATH_SIZE, folder, name, L".dmp") || !crash_path(log, PATH_SIZE, folder, name, L".log"))
	{
		CloseHandle(process);
		return;
	}
	file = CreateFileW(dump, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		platform_log("crash report: cannot write %ls (error %lu)", dump, (unsigned long)GetLastError());
		CloseHandle(process);
		return;
	}
	exception.ThreadId = thread_id;
	exception.ExceptionPointers = (EXCEPTION_POINTERS *)exception_pointers;
	/* (the pointers are the crashed game's) */
	exception.ClientPointers = TRUE;
	/* the threads' stacks and registers and the modules (with the ids of their
	PDBs, for Sentry to find symbols by), not the game's memory */
	dumped = MiniDumpWriteDump(process, process_id, file,
		MiniDumpNormal | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules, &exception, NULL, NULL);
	CloseHandle(file);
	if (!dumped)
	{
		platform_log("crash report: the minidump failed (error %lu)", (unsigned long)GetLastError());
		DeleteFileW(dump);
		CloseHandle(process);
		return;
	}
	crash_add_codeview_records(process, dump);
	crash_signal_dumped(process_id);
	/* (the game's fullscreen window would cover the question) */
	WaitForSingleObject(process, EXIT_WAIT_MILLISECONDS);
	crash_copy_log(process, log);
	CloseHandle(process);
	platform_log("crash report: wrote %ls", dump);

	if (strcmp(consent, "yes"))
	{
		answer = crash_ask();
		if (answer < 0)
		{
			platform_log("crash report: no answer; nothing sent");
			crash_delete_report(folder, name);
			return;
		}
		config_write("crash_reports.upload", answer ? "yes" : "no");
		if (!answer)
		{
			platform_log("crash report: the player declined; none will be sent");
			crash_delete_report(folder, name);
			return;
		}
	}
	crash_upload_pending(folder);
}

/* "halo.exe --crash-upload": the reports a crash left unsent */
static void crash_uploader(void)
{
	wchar_t folder[PATH_SIZE], names[MAXIMUM_PENDING_REPORTS * 2][MAX_PATH];
	const char *consent;
	int count, index;

	if (!crash_folder(folder, PATH_SIZE))
		return;
	crash_reporter_log(folder);
	consent = crash_consent();
	if (!strcmp(consent, "yes"))
	{
		crash_upload_pending(folder);
	}
	else if (!strcmp(consent, "no"))
	{
		count = crash_pending_reports(folder, names, MAXIMUM_PENDING_REPORTS * 2);
		if (count > MAXIMUM_PENDING_REPORTS * 2)
			count = MAXIMUM_PENDING_REPORTS * 2;
		for (index = 0; index < count; index++)
			crash_delete_report(folder, names[index]);
	}
}

/* ---------- the crashed game */

static int crash_reports_enabled(void)
{
	return HALO_BUILD_NUMBER > 0 || GetEnvironmentVariableW(L"HALO_CRASH_REPORTS_ANY_BUILD", NULL, 0) > 0;
}

/* starts this executable again with the option and its arguments, with no
console and none of this process's handles (its sockets hold the game's
ports) */
static HANDLE crash_start_reporter(const wchar_t *arguments)
{
	wchar_t executable[PATH_SIZE], command[PATH_SIZE + 128];
	STARTUPINFOW startup;
	PROCESS_INFORMATION process;
	DWORD length = GetModuleFileNameW(NULL, executable, PATH_SIZE);

	if (!length || length >= PATH_SIZE)
		return NULL;
	_snwprintf(command, PATH_SIZE + 128, L"\"%ls\" %ls", executable, arguments);
	command[PATH_SIZE + 127] = 0;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	if (!CreateProcessW(executable, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process))
		return NULL;
	CloseHandle(process.hThread);
	return process.hProcess;
}

/* has the reporter dump this process; 1 once the minidump is written */
static int crash_dump(EXCEPTION_POINTERS *exception)
{
	wchar_t name[64], arguments[128];
	HANDLE event, waits[2];
	DWORD result;

	_snwprintf(name, 64, L"Local\\halo-crash-dumped-%lu", (unsigned long)GetCurrentProcessId());
	event = CreateEventW(NULL, TRUE, FALSE, name);
	if (!event)
		return 0;
	_snwprintf(arguments, 128, CRASH_REPORT_OPTION L" %lu %lu %lx", (unsigned long)GetCurrentProcessId(),
		(unsigned long)GetCurrentThreadId(), (unsigned long)(ULONG_PTR)exception);
	waits[0] = event;
	waits[1] = crash_start_reporter(arguments);
	if (!waits[1])
	{
		CloseHandle(event);
		return 0;
	}
	/* (until the dump is written, or the reporter ends without one: the
	player said no, say) */
	result = WaitForMultipleObjects(2, waits, FALSE, DUMP_WAIT_MILLISECONDS);
	CloseHandle(waits[1]);
	CloseHandle(event);
	return result == WAIT_OBJECT_0;
}

/* a line of the report, to the log and to debug.txt (a player sends
debug.txt; the console closes with the game) */
static void crash_line(const char *format, ...)
{
	char line[256];
	va_list arguments;
	size_t length;

	va_start(arguments, format);
	vsnprintf(line, sizeof(line) - 2, format, arguments);
	va_end(arguments);
	platform_log("%s", line);
	length = strlen(line);
	line[length] = '\r';
	line[length + 1] = '\n';
	line[length + 2] = 0;
	write_to_error_file(line, 1);
}

static LONG WINAPI crash_filter(EXCEPTION_POINTERS *exception)
{
	static volatile LONG crashed_thread = 0;
	EXCEPTION_RECORD *record = exception->ExceptionRecord;
	CONTEXT *context = exception->ContextRecord;
	const DWORD *stack = (const DWORD *)context->Esp;
	LONG thread = (LONG)GetCurrentThreadId();
	LONG first = InterlockedCompareExchange(&crashed_thread, thread, 0);
	int dumped;

	/* one report: a crash in this report goes on to Windows, and another
	thread's crash waits for the end the first one brings */
	if (first == thread)
		return EXCEPTION_CONTINUE_SEARCH;
	if (first)
		Sleep(INFINITE);
	/* the minidump first, before anything here can fail */
	dumped = crash_reports_enabled() && crash_dump(exception);
	/* (where halo.exe is: tools/symbolize_crash.py finds the lines of the
	addresses below from it and halo.pdb) */
	crash_line("crash: halo.exe at %p, build %d (%s)", (void *)GetModuleHandleW(NULL), HALO_BUILD_NUMBER,
		HALO_BUILD_FLAVOR);
	crash_line("crash: exception %08lx at %p (accessing %p), eip %08lx ebp %08lx esp %08lx",
		record->ExceptionCode, record->ExceptionAddress,
		record->NumberParameters >= 2 ? (void *)record->ExceptionInformation[1] : NULL,
		context->Eip, context->Ebp, context->Esp);
	if (!IsBadReadPtr(stack, 6 * sizeof(DWORD)))
	{
		crash_line("crash: stack %08lx %08lx %08lx %08lx %08lx %08lx",
			stack[0], stack[1], stack[2], stack[3], stack[4], stack[5]);
	}
	{
		/* the EBP frame chain (the game keeps frame pointers) */
		const DWORD *frame = (const DWORD *)context->Ebp;
		int depth;

		for (depth = 0; depth < 32 && frame && !IsBadReadPtr(frame, 2 * sizeof(DWORD)); depth++)
		{
			crash_line("crash: called from %08lx", frame[1]);
			if ((const DWORD *)frame[0] <= frame)
				break;
			frame = (const DWORD *)frame[0];
		}
	}
	fflush(stderr);
	/* (the reporter has it: the game ends without Windows's crash dialog) */
	return dumped ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/* abort() and a bad argument to the C runtime end the process at once
(__fastfail), past the filter: an exception of their own first */
static void crash_abort(int signal_number)
{
	(void)signal_number;
	RaiseException(CRASH_ABORT_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

static void crash_invalid_parameter(const wchar_t *expression, const wchar_t *function, const wchar_t *file,
	unsigned int line, uintptr_t reserved)
{
	(void)expression;
	(void)function;
	(void)file;
	(void)line;
	(void)reserved;
	RaiseException(CRASH_INVALID_PARAMETER_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

/* ---------- start-up */

/* the reporter's option on the command line, or NULL */
static const wchar_t *crash_option(const wchar_t *option)
{
	const wchar_t *command = GetCommandLineW();
	const wchar_t *found = command ? wcsstr(command, option) : NULL;

	return found && found > command && found[-1] == L' ' ? found + wcslen(option) : NULL;
}

/* whether this process is a reporter (win32_posix.c then leaves halo.log
to the game) */
int crash_reporter_process(void)
{
	return crash_option(CRASH_REPORT_OPTION) || crash_option(CRASH_UPLOAD_OPTION);
}

__attribute__((constructor))
static void crash_reports_install(void)
{
	const wchar_t *arguments;
	ULONG guarantee = 64 * 1024;
	wchar_t folder[PATH_SIZE];

	if ((arguments = crash_option(CRASH_REPORT_OPTION)) != NULL)
	{
		unsigned long process_id = 0, thread_id = 0, exception_pointers = 0;

		if (swscanf(arguments, L" %lu %lu %lx", &process_id, &thread_id, &exception_pointers) == 3)
			crash_reporter((DWORD)process_id, (DWORD)thread_id, (ULONG_PTR)exception_pointers);
		ExitProcess(0);
	}
	if (crash_option(CRASH_UPLOAD_OPTION))
	{
		crash_uploader();
		ExitProcess(0);
	}

	SetUnhandledExceptionFilter(crash_filter);
	/* (room on the main thread's stack for the filter after a stack overflow) */
	SetThreadStackGuarantee(&guarantee);
	signal(SIGABRT, crash_abort);
	_set_invalid_parameter_handler(crash_invalid_parameter);
	/* reports an earlier crash could not send */
	if (crash_reports_enabled() && crash_folder(folder, PATH_SIZE) && crash_pending_reports(folder, NULL, 0))
	{
		HANDLE uploader = crash_start_reporter(CRASH_UPLOAD_OPTION);

		if (uploader)
			CloseHandle(uploader);
	}
}
