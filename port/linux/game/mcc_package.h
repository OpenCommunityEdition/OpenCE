/* MCC installation paths, independent of Xbox and Custom Edition loaders. */
#ifndef MCC_PACKAGE_H
#define MCC_PACKAGE_H
#include <stdint.h>
#include <stdio.h>
/* Canonical identities fit the engine's existing 64-byte map-name field. */
int mcc_package_name(char const *name, char canonical[64]);
/* While set, the package `name` reads its own map and resource maps from the folder
 * `folder` (forward slashes, relative to the data root: mcc_staging/<package>) so a downloaded
 * package can be fingerprinted before it is installed. NULL clears it. */
void mcc_package_stage_redirect(char const *name, char const *folder);
int mcc_package_map_path(char const *name, char path[128]);
int mcc_package_resource_path(char const *name, unsigned type, char path[128]);
/* Package first, MCC root only on ENOENT; path records the selected/failed file. */
FILE *mcc_package_resource_open(char const *name, unsigned type, char path[128]);
/* Base candidates in loader order: legacy root, singleplayer, multiplayer. */
#define MCC_PACKAGE_BASE_SETS 3
int mcc_package_base_resource_path(unsigned type,unsigned set,char path[128]);
/* Retain a present package override, otherwise use one fixed local base set. */
int mcc_package_resource_candidate(char const *name,unsigned type,unsigned set,char path[128]);
int mcc_package_shared_resource(char const *path, unsigned type);
#endif
