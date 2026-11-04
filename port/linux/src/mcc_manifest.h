/* Explicit Halo 1 campaign metadata, shared by local and Steam packages. */
#ifndef MCC_MANIFEST_H
#define MCC_MANIFEST_H

#include "steam_workshop.h"

struct mcc_manifest
{
	char title[STEAM_WORKSHOP_TITLE_SIZE];
	int count;
	struct steam_workshop_map_text missions[STEAM_WORKSHOP_MAXIMUM_ITEM_MAPS];
};

/* Host paths only. Returns 1 for a complete manifest, 0 when absent, -1
 * when invalid. Missing map binaries remain explicit entries: never skip a
 * mission to advance to a later one. reason is always NUL terminated. */
int mcc_manifest_load(const char *folder, struct mcc_manifest *manifest, char reason[128]);
/* Resolve the owning metadata folder through the existing MCC path mapping.
 * A local override never borrows its campaign from the Steam installation. */
int mcc_manifest_for_level(const char *level, struct mcc_manifest *manifest, char reason[128]);

#endif
