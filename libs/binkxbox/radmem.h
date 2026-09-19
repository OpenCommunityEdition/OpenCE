#ifndef BINK_RADMEM_H
#define BINK_RADMEM_H

typedef void *(__stdcall *rad_malloc_proc)(
	unsigned long bytes);
typedef void (__stdcall *rad_free_proc)(
	void *allocation);

void __stdcall RADSetMemory(
	rad_malloc_proc allocate,
	rad_free_proc release);
void *__stdcall radmalloc(
	unsigned long bytes);
void __stdcall radfree(
	void *allocation);

#endif
