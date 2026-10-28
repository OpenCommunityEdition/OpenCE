/*
UPDATE.H

What the desktop self-updater (updater.c) needs of the system: a download
over HTTPS, and putting files in place of the running game's and starting
it again. posix_update.c does it on Linux (Mbed TLS, the system's
certificate authorities), win32_update.c on Windows (WinHTTP). The Android
app updates itself in Java (port/android).

Only integers and strings cross here: the two sides are compiled with
different structure layouts.
*/

#ifndef UPDATE_H
#define UPDATE_H

/* bytes received so far, of total (0 when the size is not known) */
typedef void (*update_progress_proc)(void *context, unsigned long long received, unsigned long long total);

/* fetches the https:// url (following redirects) into the file at path,
with the certificate and host name checked, and no more than maximum bytes
of it; 1 on success, else 0 and why in error */
int update_download(const char *url, const char *path, unsigned long long maximum, update_progress_proc progress,
	void *context, char *error, int error_size);

/* one HTTP request (update_https_request's answer): the status (0 if none
came), the response's headers as received ("Name: value" lines, each ended
by "\n"), and its body (malloc'd, NUL terminated: the caller frees it) */
struct update_response
{
	int status;
	char headers[4096];
	char *body;
	unsigned long body_size;
};

/* a request (method "GET", "PUT", "DELETE", "POST") to an https:// url, or
an http:// one on this machine (127.0.0.1, localhost: a test's), with
extra header lines (each ended by "\r\n"; may be NULL) and a body (may be
NULL); no redirect followed, at most maximum bytes of body taken, the
certificate and host name checked; 1 if an answer came (response holds it),
else 0 and why in error. Any thread may make one */
int update_https_request(const char *method, const char *url, const char *headers, const void *body,
	unsigned long body_size, unsigned long maximum, struct update_response *response, char *error, int error_size);

/* the full path of this executable; 1 on success */
int update_executable_path(char *path, int size);

/* puts the file at new_path in place of the one at path, which is renamed
to old_path first (a running executable can be renamed, not overwritten),
keeping its permissions; 1 on success */
int update_replace_file(const char *path, const char *new_path, const char *old_path);

/* deletes the file (or empty folder) at path, if there is one */
void update_delete_file(const char *path);

/* makes the directory at path, if there is none; 1 when it is there */
int update_make_directory(const char *path);

/* starts the executable at path as a process of its own, with no arguments,
this process's environment and none of its files; 1 on success */
int update_launch(const char *path);

#endif
