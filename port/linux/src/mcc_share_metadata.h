/* Metadata carried with an MCC host snapshot. No network or installation state. */
#ifndef MCC_SHARE_METADATA_H
#define MCC_SHARE_METADATA_H

#define MCC_SHARE_METADATA_FILES 160
#define MCC_SHARE_METADATA_PATH 128
#define MCC_SHARE_METADATA_BYTES (24u * 1024u * 1024u)

enum mcc_share_metadata_kind {
    MCC_SHARE_METADATA_JSON = 1,
    MCC_SHARE_METADATA_PNG,
    MCC_SHARE_METADATA_WORKSHOP
};

struct mcc_share_metadata_entry {
    char path[MCC_SHARE_METADATA_PATH];
    unsigned kind;
    unsigned size;
};

struct mcc_share_metadata_plan {
    int count;
    struct mcc_share_metadata_entry files[MCC_SHARE_METADATA_FILES];
};

/* Read through the contained MCC reader. Campaigns include every mission's
 * JSON but only the selected mission's pictures. A PNG with size zero is an
 * allowed reference whose picture is absent or invalid: do not send it.
 * Map-only folders succeed with no metadata. campaign=-1 selects authored
 * campaign metadata when the level belongs to it, independently of menu settings.
 * reason is always terminated.
 * Plan paths use lowercase ASCII spelling; source reads must remain contained
 * and case-insensitive (posix_mcc_preview_read), not plain root/path fopen. */
int mcc_share_metadata_plan(const char *root, int campaign, const char *level,
    struct mcc_share_metadata_plan *plan, char reason[128]);

/* Required JSON identity, separate from the map/resource gameplay digest.
 * Hashes canonical relative paths and bytes; pictures and advisory Workshop
 * identity do not affect admission. All-zero means no JSON. No PNG is decoded. */
int mcc_share_metadata_digest(const char *root, int campaign, const char *level,
    unsigned char digest[32], char reason[128]);

/* Rebuild the allowlist from staged JSON, require its complete metadata, and
 * reject unreferenced received files. keep[i] is zero for an invalid optional
 * PNG; the caller discards it before installation. No file is changed here.
 * received contains metadata entries only, with sizes already hash-verified. */
int mcc_share_metadata_validate(const char *root, int campaign, const char *level,
    const struct mcc_share_metadata_entry *received, int count,
    unsigned char keep[MCC_SHARE_METADATA_FILES], char reason[128]);

#endif
