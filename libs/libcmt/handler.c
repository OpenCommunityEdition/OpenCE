/*
HANDLER.CPP

symbols in this file:
	?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z
	?_query_new_handler@@YAP6AHI@ZXZ
	__callnewh
	?_pnhHeap@@3P6AHI@ZA
*/

#include <new.h>

/* ---------- private constants */

#define _HEAP_LOCK 4

/* ---------- external interfaces */

extern "C" void __cdecl _lock(
	int lock_number);

extern "C" void __cdecl _unlock(
	int lock_number);

/* ---------- public data */

_PNH _pnhHeap;

/* ---------- public code */

_PNH __cdecl _set_new_handler(
	_PNH new_handler)
{
	_PNH old_handler;

	_lock(_HEAP_LOCK);
	old_handler = _pnhHeap;
	_pnhHeap = new_handler;
	_unlock(_HEAP_LOCK);

	return old_handler;
}

_PNH __cdecl _query_new_handler(
	void)
{
	return _pnhHeap;
}

extern "C" int __cdecl _callnewh(
	size_t size)
{
	_PNH new_handler = _pnhHeap;

	if (!new_handler || !(*new_handler)(size))
		return 0;

	return 1;
}
