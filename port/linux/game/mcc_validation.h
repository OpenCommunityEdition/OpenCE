/* MCC content checks owned by host selection, client admission or a download. */
#ifndef MCC_VALIDATION_H
#define MCC_VALIDATION_H
#include <stdint.h>
#include <stddef.h>
#include "mcc_precache.h"

struct mcc_validation_check;
struct mcc_validation_proof {
    char name[64];
    char directory[128];
    struct mcc_precache_snapshot files;
    uint32_t checksum;
    unsigned char digest[32];
    char resources[2][128];
};
/* Independent ownership for menu selection, installed admission and staged
 * downloads. Begin records intent only. Step returns -1/0/1 (failed/pending/
 * complete); end joins the worker and closes every reader before cleanup.
 * A non-NULL directory redirects only this check's opening and snapshots. */
struct mcc_validation_check *mcc_validation_check_begin(char const *name, char const *directory);
int mcc_validation_check_step(struct mcc_validation_check *check,
    struct mcc_validation_proof *proof, char *reason, size_t reason_size);
void mcc_validation_check_end(struct mcc_validation_check *check);
/* Publish only after a fresh full candidate snapshot matches the proof.
 * Staged proofs may move to a new package folder with the same map leaf;
 * selected package resources are remapped without reading their bytes again. */
int mcc_validation_publish(char const *name, struct mcc_validation_proof const *proof);

/* One pending package, four bounded successful results; no session identity. */
/* Request returns 1 when ready, 0 while pending, -1 after failure. Lookup
 * never schedules work. Cancel releases readers but retains completed facts. */
int mcc_validation_request(char const *name, uint32_t *checksum, unsigned char digest[32]);
int mcc_validation_lookup(char const *name, uint32_t *checksum, unsigned char digest[32]);
/* Exact resource selection from a successful check, with the same candidate
 * stamp revalidation as lookup. Empty entries are unused, not missing. */
int mcc_validation_resources(char const *name, char paths[2][128]);
void mcc_validation_update(void);
void mcc_validation_cancel(void);
char const *mcc_validation_error(void);
#endif
