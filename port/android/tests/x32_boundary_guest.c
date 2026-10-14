/*
X32_BOUNDARY_GUEST.C

The guest's half of x32_boundary_test.c: compiled as the x32 guest is
(tools/guest_arch.py's X32 flags), it calls the host through the guest's
import stub with pointers made every way that leaves a register's upper
half for the code generator to decide: from a 32-bit integer argument,
from pointer arithmetic, passed on unchanged, and returned by the host.
*/

extern int host_check(void *from_integer, const char *sum, void *passed_on, void *difference, int integer,
	long long wide);
extern char *host_pointer(void);
extern int host_check_returned(char *returned);

int guest_entry(int integer, unsigned int offset, char *pointer)
{
	int result = host_check((void *)integer, pointer + offset, pointer, pointer - 16, integer, (long long)integer);

	return result | host_check_returned(host_pointer() + 1) << 8;
}
