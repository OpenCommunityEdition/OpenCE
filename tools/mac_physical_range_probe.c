#include <stdio.h>
#include "engine_physical_memory.h"
#include "xtl.h"
#undef fprintf
#undef printf

static unsigned int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); } } while (0)

int main(void)
{
	unsigned char *first, *second;
	CHECK(!mac_engine_physical_memory_contains((void *)0x80000000UL, 1));
	CHECK(mac_engine_physical_memory_start());
	CHECK(!mac_engine_physical_memory_contains((void *)0x80000000UL, 1));
	first = (unsigned char *)XPhysicalAlloc(8192, 0xffffffffUL, 0, PAGE_READWRITE);
	second = (unsigned char *)XPhysicalAlloc(4096, 0xffffffffUL, 0, PAGE_READWRITE);
	CHECK(first != NULL && second != NULL);
	if (first && second)
	{
		CHECK(mac_engine_physical_memory_contains(first, 1));
		CHECK(mac_engine_physical_memory_contains(first + 4096, 4096));
		CHECK(mac_engine_physical_memory_contains(first, 8192));
		CHECK(!mac_engine_physical_memory_contains(first, 8193));
		CHECK(mac_engine_physical_memory_contains(second, 4096));
		XPhysicalFree(first);
		CHECK(!mac_engine_physical_memory_contains(first, 1));
		XPhysicalFree(second);
		CHECK(!mac_engine_physical_memory_contains(second, 1));
	}
	mac_engine_physical_memory_stop();
	CHECK(!mac_engine_physical_memory_contains((void *)0x80000000UL, 1));
	printf("Mac physical ownership range: %u checks, %u failures\n", checks, failures);
	return failures ? 1 : 0;
}
