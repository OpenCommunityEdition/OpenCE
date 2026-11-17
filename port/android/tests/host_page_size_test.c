/*
HOST_PAGE_SIZE_TEST.C

Tests the host's runtime page size (host_page_size and
host_memory_page_size_init in host_memory.c). host_memory.c is linked as it
is; the one symbol it needs from other host files that is not in
host_watch_hash.c is stubbed here.
*/

#include "host.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

void host_logf(int priority, const char *format, ...)
{
	(void)priority;
	(void)format;
}

struct host_guest_image host_image;

static int failures;

#define CHECK(condition) \
	do \
	{ \
		if (!(condition)) \
		{ \
			fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
			failures++; \
		} \
	} while (0)

int main(void)
{
	uint64_t size;
	long reported = sysconf(_SC_PAGESIZE);

	/* before init, the default */
	CHECK(host_page_size() == 4096);

	/* after init, the kernel's page size */
	host_memory_page_size_init();
	CHECK(reported > 0);
	CHECK(host_page_size() == (uint64_t)reported);

	/* a second init keeps it */
	host_memory_page_size_init();
	CHECK(host_page_size() == (uint64_t)reported);

	/* usable as a mask: a power of two, at least 4096 */
	size = host_page_size();
	CHECK(size >= 4096);
	CHECK((size & (size - 1)) == 0);

	if (failures)
		return 1;
	printf("host page size tests passed\n");
	return 0;
}
