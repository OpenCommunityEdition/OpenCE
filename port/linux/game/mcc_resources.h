/* Separate resource files belonging to an MCC map installation. */
#ifndef MCC_RESOURCES_H
#define MCC_RESOURCES_H
#include <stdint.h>
struct mcc_runtime;
struct mcc_cache_source;
int mcc_resources_select(struct mcc_runtime *runtime, void *raw);
int mcc_resources_checksum(struct mcc_runtime *runtime, unsigned mask, uint32_t *checksum);
int mcc_resources_probe(char const *name, struct mcc_cache_source const *source, uint32_t *checksum);
int mcc_resources_read(struct mcc_runtime *runtime, unsigned type,
    char const *name, uint32_t offset, uint32_t bytes, void *out, char const *group_name);
void mcc_resources_dispose(struct mcc_runtime *runtime);
#endif
