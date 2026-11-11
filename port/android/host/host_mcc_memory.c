/* Keep MCC's linked tags out of Android's later allocations. This is an
optional, process-lifetime reservation, independent of the Xbox guest arena.
Never reclaim foreign mappings, including ART's reservations. */
#include "host.h"
#include "host_mcc_memory.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

#define MCC_TAG_BASE 0x50000000u
#define MCC_TAG_SIZE 0x04000000u

static pthread_mutex_t mcc_lock = PTHREAD_MUTEX_INITIALIZER;
static void *mcc_window;
static int mcc_active;

static int mcc_hold(unsigned int address)
{
	void *wanted = (void *)(uintptr_t)address;
	void *result = mmap(wanted, MCC_TAG_SIZE, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED_NOREPLACE, -1, 0);

	if (result == wanted)
	{
		mcc_window = result;
		return 0;
	}
	if (result != MAP_FAILED)
	{
		/* Older kernels may treat NOREPLACE as a hint. Do not accept a
		different address: map tag pointers are linked to the requested base. */
		munmap(result, MCC_TAG_SIZE);
		return -EEXIST;
	}
	return -errno;
}

static void mcc_report_overlap(unsigned int address, int error)
{
	FILE *maps;
	char line[512];
	unsigned long long lo, hi;

	host_logf(HOST_LOG_WARN, "mcc memory: cannot reserve %08x-%08x (%s)",
		address, address + MCC_TAG_SIZE, strerror(-error));
	maps = fopen("/proc/self/maps", "r");
	if (!maps) return;
	while (fgets(line, sizeof(line), maps))
	{
		if (sscanf(line, "%llx-%llx", &lo, &hi) == 2 &&
			lo < (unsigned long long)address + MCC_TAG_SIZE && hi > address)
		{
			line[strcspn(line, "\n")] = '\0';
			host_logf(HOST_LOG_WARN, "mcc memory: occupied by %s", line);
		}
	}
	fclose(maps);
}

void host_mcc_memory_initialize(void)
{
	int error = 0;

	pthread_mutex_lock(&mcc_lock);
	if (!mcc_window) error = mcc_hold(MCC_TAG_BASE);
	if (error) mcc_report_overlap(MCC_TAG_BASE, error);
	pthread_mutex_unlock(&mcc_lock);
}

long long host_mcc_memory_reserve(unsigned int address, unsigned int size)
{
	void *wanted = (void *)(uintptr_t)address;
	int error = 0;

	/* Match the MCC reader's allowed windows; exclude legacy tag arenas. */
	if ((address & 0xffffu) || size != MCC_TAG_SIZE ||
		address < 0x42000000u || address > 0x70000000u)
		return -EINVAL;
	pthread_mutex_lock(&mcc_lock);
	if (mcc_active) error = -EBUSY;
	else
	{
		if (mcc_window && mcc_window != wanted)
		{
			if (munmap(mcc_window, MCC_TAG_SIZE)) error = -errno;
			else mcc_window = NULL;
		}
		if (!error && !mcc_window)
		{
			error = mcc_hold(address);
			if (error) mcc_report_overlap(address, error);
		}
		if (!error)
		{
			/* Only replace the exact inactive reservation owned by this module. */
			if (mmap(mcc_window, MCC_TAG_SIZE, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0) == MAP_FAILED)
				error = -errno;
			else mcc_active = 1;
		}
	}
	pthread_mutex_unlock(&mcc_lock);
	return error ? (long long)error : (long long)address;
}

int host_mcc_memory_release(unsigned int address, unsigned int size)
{
	int error = 0;

	pthread_mutex_lock(&mcc_lock);
	if (!mcc_active || mcc_window != (void *)(uintptr_t)address || size != MCC_TAG_SIZE)
		error = -EINVAL;
	else if (mmap(mcc_window, MCC_TAG_SIZE, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED, -1, 0) == MAP_FAILED)
		error = -errno;
	else mcc_active = 0;
	pthread_mutex_unlock(&mcc_lock);
	return error;
}
