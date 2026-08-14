/*
MALLOC.C

symbols in this file:
	_malloc
	__nh_malloc
	__heap_alloc
*/

#include <stddef.h>

/* ---------- private constants */

enum
{
	_SYSTEM_HEAP = 1,
	_BYTES_PER_PARAGRAPH = 16
};

#define HEAP_MAXIMUM_REQUEST ((size_t)-32)

/* ---------- external data */

extern int __active_heap;
extern int _newmode;

/* ---------- external code */

extern int __cdecl _callnewh(size_t size);
extern void * __stdcall GetProcessHeap(void);
extern void * __stdcall RtlAllocateHeap(
	void *heap,
	unsigned long flags,
	size_t size);

/* ---------- public code */

void * __cdecl _heap_alloc(size_t size)
{
	if (size == 0)
		size = 1;

	if (__active_heap != _SYSTEM_HEAP)
	{
		size = (size + _BYTES_PER_PARAGRAPH - 1) &
			~(_BYTES_PER_PARAGRAPH - 1);
	}

	return RtlAllocateHeap(
		GetProcessHeap(),
		0,
		size);
}

void * __cdecl _nh_malloc(
	size_t size,
	int new_handler_enabled)
{
	void *result;

	if (size > HEAP_MAXIMUM_REQUEST)
		return NULL;

	for (;;)
	{
		if (size <= HEAP_MAXIMUM_REQUEST)
			result = _heap_alloc(size);
		else
			result = NULL;

		if (result || new_handler_enabled == 0)
			return result;

		if (!_callnewh(size))
			return NULL;
	}
}

void * __cdecl malloc(size_t size)
{
	return _nh_malloc(size, _newmode);
}
