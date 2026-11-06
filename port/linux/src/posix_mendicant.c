/*
POSIX_MENDICANT.C

MENDICANT AI's platform side (port/linux/game/mendicant): the model bridge's
shared memory, which a trainer or brain.py maps from the same path, and the
clock, yield and sleep that the training wait and the paused episode use.
Desktop Linux only (the web build and Windows leave this file out; Android
skips posix_*). Built with the host ABI.
*/

#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

/* the file at path, created if missing (0600), sized and mapped shared;
NULL when any step fails */
void *posix_mendicant_bridge_map(char const *path, unsigned long size)
{
	void *memory;
	int descriptor = open(path, O_RDWR | O_CREAT, 0600);

	if (descriptor < 0)
		return NULL;
	if (ftruncate(descriptor, (off_t)size))
	{
		close(descriptor);
		return NULL;
	}
	memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
	close(descriptor);
	return memory == MAP_FAILED ? NULL : memory;
}

void posix_mendicant_yield(void)
{
	sched_yield();
}

unsigned long long posix_mendicant_monotonic_ns(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (unsigned long long)now.tv_sec * 1000000000ull + (unsigned long long)now.tv_nsec;
}

void posix_mendicant_sleep_microseconds(unsigned long microseconds)
{
	usleep((useconds_t)microseconds);
}
