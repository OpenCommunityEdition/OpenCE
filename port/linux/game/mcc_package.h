/* MCC installation paths, independent of Xbox and Custom Edition loaders. */
#ifndef MCC_PACKAGE_H
#define MCC_PACKAGE_H
#include <stdint.h>
/* Canonical identities fit the engine's existing 64-byte map-name field. */
int mcc_package_name(char const *name, char canonical[64]);
int mcc_package_map_path(char const *name, char path[128]);
int mcc_package_resource_path(char const *name, unsigned type, char path[128]);
int mcc_package_checksum(char const *name, uint32_t map_checksum, uint32_t *checksum);
#endif
