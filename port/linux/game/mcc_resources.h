/* Separate resource files belonging to an MCC map installation. */
#ifndef MCC_RESOURCES_H
#define MCC_RESOURCES_H
#include <stdint.h>
#include <stddef.h>
struct mcc_runtime;
struct mcc_cache_source;
struct mcc_resource_check;
struct mcc_resource_check *mcc_resources_check_begin(char const *name,
    struct mcc_cache_source const *source, char *reason, size_t size);
int mcc_resources_check_step(struct mcc_resource_check *check, uint32_t *checksum,
    unsigned char digest[32]);
void mcc_resources_check_end(struct mcc_resource_check *check);
int mcc_resources_select(struct mcc_runtime *runtime, void *raw);
int mcc_resources_checksum(struct mcc_runtime *runtime, unsigned mask, uint32_t *checksum);
int mcc_resources_probe(char const *name, struct mcc_cache_source const *source, uint32_t *checksum);
int mcc_resources_probe_explain(char const *name, struct mcc_cache_source const *source,
    uint32_t *checksum, char *reason, size_t size);
int mcc_resources_read(struct mcc_runtime *runtime, unsigned type,
    char const *name, uint32_t offset, uint32_t bytes, void *out, char const *group_name);
void mcc_resources_dispose(struct mcc_runtime *runtime);
#endif
