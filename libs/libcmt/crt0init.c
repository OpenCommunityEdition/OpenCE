/*
CRT0INIT.C

symbols in this file:
	___xp_a (.CRT$XPA, 0x4)
	___xp_z (.CRT$XPZ, 0x4)
	___xt_a (.CRT$XTA, 0x4)
	___xt_z (.CRT$XTZ, 0x4)
*/

/* ---------- types */

typedef void (__cdecl *_PVFV)(
	void);

/* ---------- runtime section ownership */

#pragma data_seg(".CRT$XPA")
_PVFV __xp_a[] = { 0 };

#pragma data_seg(".CRT$XPZ")
_PVFV __xp_z[] = { 0 };

#pragma data_seg(".CRT$XTA")
_PVFV __xt_a[] = { 0 };

#pragma data_seg(".CRT$XTZ")
_PVFV __xt_z[] = { 0 };

#pragma data_seg()
