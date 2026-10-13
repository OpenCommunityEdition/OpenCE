#ifndef MCC_PRECACHE_H
#define MCC_PRECACHE_H
#include <stdint.h>
#define MCC_PRECACHE_FILES 9
struct mcc_precache_stamp {
    uint32_t present, attributes, size_low, size_high;
    uint32_t created_low, created_high, written_low, written_high;
};
struct mcc_precache_snapshot {
    struct mcc_precache_stamp files[MCC_PRECACHE_FILES];
};
int mcc_precache_snapshot(char const *name, struct mcc_precache_snapshot *snapshot);
#endif
