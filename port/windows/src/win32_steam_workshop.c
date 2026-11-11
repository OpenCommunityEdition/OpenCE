/*
WIN32_STEAM_WORKSHOP.C

Registered Steam installations for the MCC Workshop catalog. This Windows-facing
unit keeps the Windows SDK separate from the Xbox-facing platform headers.
*/

#include <windows.h>
#include "steam_workshop.h"

static int registry_steam_root(HKEY key, const char *value, DWORD view, char *path)
{
	DWORD size = STEAM_WORKSHOP_PATH_SIZE;

	/* RegGetValue checks the string type, bounds and termination. Use the same
	 * narrow encoding as win32_files.c and the runtime's file functions. */
	if (RegGetValueA(key, "Software\\Valve\\Steam", value, RRF_RT_REG_SZ | view,
		NULL, path, &size) != ERROR_SUCCESS)
		return 0;
	return path[0] != 0;
}

int steam_workshop_windows_roots(char (*paths)[STEAM_WORKSHOP_PATH_SIZE], int maximum)
{
	int count = 0;

	/* The game is 32-bit, but Steam may register its installation in either
	 * machine-wide registry view. Missing registrations leave the fallbacks. */
	if (count < maximum && registry_steam_root(HKEY_CURRENT_USER, "SteamPath", 0, paths[count]))
		count++;
	if (count < maximum && registry_steam_root(HKEY_LOCAL_MACHINE, "InstallPath", RRF_SUBKEY_WOW6432KEY, paths[count]))
		count++;
	if (count < maximum && registry_steam_root(HKEY_LOCAL_MACHINE, "InstallPath", RRF_SUBKEY_WOW6464KEY, paths[count]))
		count++;
	return count;
}
