/*
POSIX.H

Boundary between the XDK-facing platform layer and glibc.

Everything except posix_*.c is compiled with the game's MSVC-compatible ABI
(-malign-double, 16-bit wchar_t, XDK headers), under which glibc structures
with 64-bit members, such as struct stat and struct dirent, have the wrong
layout. The posix_*.c files are compiled with the host ABI instead and expose
these helpers, whose parameters are all 32-bit scalars or pointers to structs
made only of 32-bit members so both sides agree on the layout.
*/

#ifndef __HALO_LINUX_POSIX_H
#define __HALO_LINUX_POSIX_H

enum
{
	_posix_file_is_directory = 1 << 0,
	_posix_file_is_read_only = 1 << 1,
};

struct posix_file_information
{
	unsigned long flags;
	unsigned long size_low;
	unsigned long size_high;
	/* seconds and nanoseconds since the Unix epoch */
	unsigned long modification_seconds;
	unsigned long modification_nanoseconds;
	unsigned long access_seconds;
	unsigned long access_nanoseconds;
	unsigned long creation_seconds;
	unsigned long creation_nanoseconds;
};

/* stat()/fstat(); return 0 on success or -1 with errno set */
int posix_stat(const char *path, struct posix_file_information *information);
int posix_fstat(int descriptor, struct posix_file_information *information);

/* set access and modification times; a zero seconds value leaves it alone */
int posix_set_file_times(const char *path,
	unsigned long access_seconds, unsigned long access_nanoseconds,
	unsigned long modification_seconds, unsigned long modification_nanoseconds);

/* 64-bit file positioning on a descriptor */
int posix_seek(int descriptor, long offset_low, long offset_high, int whence,
	unsigned long *position_low, unsigned long *position_high);
int posix_truncate(int descriptor, unsigned long size_low, unsigned long size_high);

/* free and total bytes on the file system holding path */
int posix_disk_space(const char *path,
	unsigned long *free_low, unsigned long *free_high,
	unsigned long *total_low, unsigned long *total_high);

/* permissions and directories */
int posix_set_read_only(const char *path, int read_only);
int posix_make_directory(const char *path);

/* directory enumeration; the handle is opaque */
void *posix_directory_open(const char *path);
/* copies the next entry name (excluding . and ..); returns 0 at the end */
int posix_directory_next(void *directory, char *name, unsigned long name_size);
void posix_directory_close(void *directory);

/* case-insensitive lookup of one path component inside directory;
copies the on-disk spelling into result and returns nonzero if found */
int posix_find_entry_case_insensitive(const char *directory, const char *name,
	char *result, unsigned long result_size);

/* ---------- sockets

Winsock and BSD share the sockaddr_in layout, so addresses pass through as
opaque pointers. Every call returns -1 on failure with the equivalent
Winsock error code available from posix_socket_last_error(). */

int posix_socket_last_error(void);
int posix_socket(int family, int type, int protocol);
int posix_socket_close(int socket);
int posix_socket_bind(int socket, const void *address, int address_length);
int posix_socket_connect(int socket, const void *address, int address_length);
int posix_socket_listen(int socket, int backlog);
int posix_socket_accept(int socket, void *address, int *address_length);
int posix_socket_send(int socket, const void *buffer, int length, int flags);
int posix_socket_sendto(int socket, const void *buffer, int length, int flags,
	const void *address, int address_length);
int posix_socket_recv(int socket, void *buffer, int length, int flags);
int posix_socket_recvfrom(int socket, void *buffer, int length, int flags,
	void *address, int *address_length);
int posix_socket_shutdown(int socket, int how);
int posix_socket_set_nonblocking(int socket, int nonblocking);
int posix_socket_bytes_available(int socket, unsigned long *count);
/* Winsock option levels and names are translated for SOL_SOCKET options */
int posix_socket_setsockopt(int socket, int level, int name, const void *value, int length);
int posix_socket_getsockopt(int socket, int level, int name, void *value, int *length);
int posix_socket_getsockname(int socket, void *address, int *address_length);
int posix_socket_getpeername(int socket, void *address, int *address_length);
/* select over explicit descriptor lists; each list is rewritten in place to
hold only the ready descriptors, and its count updated */
int posix_socket_select(int *read, int *read_count, int *write, int *write_count,
	int *error, int *error_count, long timeout_seconds, long timeout_microseconds, int infinite);
/* the first non-loopback IPv4 address (network byte order), or 0 */
unsigned long posix_local_ipv4_address(void);
/* fills buffer with cryptographically random bytes */
void posix_random_bytes(void *buffer, unsigned long size);

#endif
