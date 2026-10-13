/*
UPDATE.H

What the desktop self-updater (updater.c) needs of the system: a download
over HTTPS, and putting files in place of the running game's and starting
it again. The map archives (map_archives.c) download over HTTPS with it
too. posix_update.c does it on Linux (Mbed TLS, the system's
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
with the certificate and host name checked; 1 on success, else 0 and why in
error */
int update_download(const char *url, const char *path, update_progress_proc progress, void *context,
	char *error, int error_size);

/* update_download's request with more of it given: form, when not NULL, is
POSTed (application/x-www-form-urlencoded, a redirect after it followed with
a GET); agent, when not NULL, is the User-Agent; and the download stops,
failing, once *stop (when stop is not NULL) is not 0 */
int update_request(const char *url, const char *form, const char *agent, const volatile int *stop,
	const char *path, update_progress_proc progress, void *context, char *error, int error_size);

/* update_request's GET of length bytes of the file at url from offset (a
Range request) into the file at path: 1 when the server sent those bytes
(206), else 0 and why in error (a server that sends the whole file instead
is refused, unread) */
int update_request_range(const char *url, const char *agent, const volatile int *stop, unsigned long long offset,
	unsigned long long length, const char *path, char *error, int error_size);

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
