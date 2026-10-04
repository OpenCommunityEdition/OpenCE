/* the Linux socket, random and struct stat spellings on Darwin
(posix_files.c, posix_net.c) */
#define st_mtim st_mtimespec
#define st_atim st_atimespec
#define st_ctim st_ctimespec

#include <sys/socket.h>
#include <sys/random.h>

#define SOCK_CLOEXEC 0
#define SOCK_NONBLOCK 0

static inline int accept4(int socket, struct sockaddr *address, socklen_t *address_length, int flags)
{
	(void)flags;
	return accept(socket, address, address_length);
}

static inline ssize_t getrandom(void *buffer, size_t size, unsigned flags)
{
	(void)flags;
	return getentropy(buffer, size) == 0 ? (ssize_t)size : -1;
}
