/* Separate resource files belonging to an MCC map installation. */
#ifndef MCC_RESOURCES_H
#define MCC_RESOURCES_H
#include <stdint.h>
struct mcc_runtime;
int mcc_resources_read(struct mcc_runtime *runtime, unsigned type,
    char const *name, uint32_t offset, uint32_t bytes, void *out);
void mcc_resources_dispose(struct mcc_runtime *runtime);
#endif
