/*
XBDM.C

The Xbox debug monitor (xbdm.dll) calls the game makes. There is no debug
monitor on Linux: module walks report an empty list, which shell_xbox.c's
start-up code already treats as "nothing to protect".
*/

#include "platform.h"

HRESULT __stdcall DmWalkLoadedModules(PDM_WALK_MODULES *walk, PDMN_MODLOAD module)
{
	(void)walk;
	(void)module;
	return XBDM_ENDOFLIST;
}

HRESULT __stdcall DmWalkModuleSections(PDM_WALK_MODSECT *walk, LPCSTR module_name, PDMN_SECTIONLOAD section)
{
	(void)walk;
	(void)module_name;
	(void)section;
	return XBDM_ENDOFLIST;
}

HRESULT __stdcall DmCloseModuleSections(PDM_WALK_MODSECT walk)
{
	(void)walk;
	return XBDM_NOERR;
}
