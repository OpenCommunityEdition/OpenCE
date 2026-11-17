/* Ownership of locally downloaded MCC snapshots, never original installations. */
#ifndef MCC_SHARE_STORAGE_H
#define MCC_SHARE_STORAGE_H
#include "mcc_share_stage.h"
#include "mcc_workshop.h"

#define MCC_SHARE_STORAGE_LIMIT 64
struct mcc_share_storage_item {
    char folder[64];
    char source[64];
    char maker[64];
    struct mcc_workshop_item workshop;
    unsigned files;          /* Managed content plus the local receipt. */
    uint64_t bytes;          /* Recorded installed bytes; listing does not rehash maps. */
};
enum mcc_share_storage_result {
    MCC_SHARE_STORAGE_ERROR,
    MCC_SHARE_STORAGE_REMOVED,
    MCC_SHARE_STORAGE_LEFTOVERS  /* Managed files gone; added unrelated files remain. */
};
/* Open a safe regular native file and wrap the descriptor in this game's FILE
 * ABI. Used by storage and by host/resource-reuse source reads. */
FILE *mcc_share_open_native(char const *path);
/* Fill only native read/stat/delete callbacks for storage operations. Stage
 * creation/rename callbacks remain unset: active staging uses virtual paths. */
void mcc_share_storage_native_io(struct mcc_share_io *io);

/* Only after real metadata/gameplay verification. A valid transferred sidecar
 * wins; otherwise a validated advertised item may generate an absent sidecar.
 * Creates a bounded local receipt exclusively. Failure requires stage_cancel. */
int mcc_share_storage_record(struct mcc_share_stage *stage, char const *source,
    char const *maker, struct mcc_workshop_item const *workshop);
/* Native mcc_maps directory and matching native IO callbacks (including the
 * mandatory open_native_read). Receipts have a
 * fixed format and bounded manifest; neither operation traverses recursively.
 * Listing caps at 64 and sets truncated when more entries may be present. */
int mcc_share_storage_list(char const *maps, struct mcc_share_io const *io,
    struct mcc_share_storage_item *items, int capacity, int *truncated);
int mcc_share_storage_read(char const *maps, struct mcc_share_io const *io,
    char const *folder, struct mcc_share_storage_item *item);
/* Find only bitmap/sound resources with matching receipt kind, size and SHA.
 * The returned native path is safe, regular and readable, with the expected
 * size. Caller copies in bounded slices through stage_write; that verifies the
 * actual hash before publication. No synchronous full-resource hashing here. */
int mcc_share_storage_find_resource(char const *maps, struct mcc_share_io const *io,
    struct mcc_share_file const *expected, char path[1024]);
/* Caller must prohibit deleting the active session's package. Every recorded
 * file is size/hash checked before the first delete. Only recorded files and
 * their empty parent directories are removed; added files/saves are retained.
 * Filesystem failures can interrupt deletion; ERROR never authorizes a broader
 * cleanup. Local processes with the same user's authority are not a trust boundary. */
enum mcc_share_storage_result mcc_share_storage_remove(char const *maps,
    struct mcc_share_io const *io, char const *folder);
#endif
