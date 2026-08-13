/*
CALLOC.C

symbols in this file:
	_calloc
*/

#include <stddef.h>

/* ---------- private constants */

enum
{
	_HEAP_ZERO_MEMORY = 0x00000008,
	_V6_HEAP = 3,
	_BYTES_PER_PARAGRAPH = 16
};

#define HEAP_MAXIMUM_REQUEST ((size_t)-32)

/* ---------- external interfaces */

extern int __active_heap;
extern int _newmode;

extern int __cdecl _callnewh(size_t size);
extern void * __stdcall GetProcessHeap(void);
extern void * __stdcall RtlAllocateHeap(
	void *heap,
	unsigned long flags,
	size_t size);

/* ---------- public code */

void * __cdecl calloc(size_t number, size_t size)
{
	void *result;

	size *= number;
	if (size == 0)
		size = 1;

	for (;;)
	{
		result = NULL;

		if (size <= HEAP_MAXIMUM_REQUEST)
		{
			if (__active_heap == _V6_HEAP)
				size = (size + _BYTES_PER_PARAGRAPH - 1) &
					~(_BYTES_PER_PARAGRAPH - 1);

			result = RtlAllocateHeap(
				GetProcessHeap(),
				_HEAP_ZERO_MEMORY,
				size);
		}

		if (result || _newmode == 0)
			return result;

		if (!_callnewh(size))
			return NULL;
	}
}
