/* Staged MCC package downloads: the files of a package a host sends are
 * written to an owned folder and checked before publication. Native builds
 * move the directory into mcc_maps; OPFS uses an invisible final directory
 * and a durable commit record. Cancelling or failing deletes owned files and
 * nothing else; no existing package or save is opened for writing.
 *
 * This module has no network or engine dependencies; tests use the same
 * staging logic and the platform's safe filesystem operations. */
#ifndef MCC_SHARE_STAGE_H
#define MCC_SHARE_STAGE_H
#include <stdint.h>
#include <stdio.h>
#include "mcc_sha256.h"

#define MCC_SHARE_STAGE_FILES 168
#define MCC_SHARE_LEAF_SIZE 128
/* One map and its resource maps are large; a credit file is a small text. */
#define MCC_SHARE_MAXIMUM_FILE_BYTES 0x30000000u
#define MCC_SHARE_MAXIMUM_TOTAL_BYTES 0x40000000u
#define MCC_SHARE_MAXIMUM_CREDIT_BYTES 0x10000u
#define MCC_SHARE_MAXIMUM_CREDITS 4
#define MCC_SHARE_MAXIMUM_JSON_BYTES (256u * 1024u)
#define MCC_SHARE_MAXIMUM_PNG_BYTES (8u * 1024u * 1024u)
#define MCC_SHARE_MAXIMUM_METADATA_BYTES (32u * 1024u * 1024u)
#define MCC_SHARE_STAGE_RECEIPT "download.receipt"
#define MCC_SHARE_STAGE_COMPANION_BYTES 32768u
enum mcc_share_stage_companion_kind {
    MCC_SHARE_STAGE_WORKSHOP,
    MCC_SHARE_STAGE_OWNERSHIP,
    MCC_SHARE_STAGE_COMPANIONS
};

enum mcc_share_file_kind {
    MCC_SHARE_FILE_MAP,          /* <level>.map */
    MCC_SHARE_FILE_BITMAPS,      /* bitmaps.map of the package */
    MCC_SHARE_FILE_SOUNDS,       /* sounds.map of the package */
    MCC_SHARE_FILE_CREDIT,       /* maker and licence text that travels with the package */
    MCC_SHARE_FILE_JSON,
    MCC_SHARE_FILE_PNG,
    MCC_SHARE_FILE_SIDECAR,
    MCC_SHARE_FILE_KINDS
};

struct mcc_share_file {
    uint8_t kind;
    char leaf[MCC_SHARE_LEAF_SIZE];   /* package-relative path, NUL ended */
    uint32_t size;
    uint8_t sha256[32];
};

/* OPFS cannot move directories. This optional strategy creates a fresh,
 * undiscoverable destination and publishes it through a durable commit record.
 * begin owns the directory on success; finish always releases its mutation
 * lease on cancellation, and releases it on successful publication. A negative
 * begin/finish result means the browser fenced further operations but cannot
 * establish publication state; recovery, not live cancellation, owns the files. */
struct mcc_share_direct_io {
    int (*begin)(char const *directory, struct mcc_share_file const *files, int count);
    int (*finish)(char const *directory, int publish);
    int (*flush)(FILE *file);
};

/* Each operation returns nonzero on success. make_directory and move_file
 * fail when their destination exists. File contents use exclusive stdio
 * creation inside the owned stage. No callback may follow links. */
struct mcc_share_io {
    int (*make_directory)(char const *path);
    int (*remove_directory)(char const *path);
    int (*directory_exists)(char const *path);
    int (*move_file)(char const *from, char const *to);
    int (*delete_file)(char const *path);
    /* All existing ancestors must be directories without links/reparse points;
     * an existing final component must be a regular file or directory. */
    int (*path_safe)(char const *path);
    /* Publish the complete directory atomically, failing if the destination exists. */
    int (*publish_directory)(char const *from, char const *to);
    /* Native host paths bypass Xbox drive translation. Returned FILE belongs
     * to the game/guest ABI, not the host side of the Android boundary. */
    FILE *(*open_native_read)(char const *path);
    struct mcc_share_direct_io const *direct;
};

enum mcc_share_stage_state {
    MCC_SHARE_STAGE_IDLE,
    MCC_SHARE_STAGE_WRITING,
    MCC_SHARE_STAGE_STAGED,      /* every file complete and checked, not installed */
    MCC_SHARE_STAGE_INSTALLED,
    MCC_SHARE_STAGE_FAILED
};

enum mcc_share_stage_result {
    MCC_SHARE_STAGE_OK,
    MCC_SHARE_STAGE_FILE_DONE,
    MCC_SHARE_STAGE_ALL_DONE,
    MCC_SHARE_STAGE_BAD_MANIFEST,
    MCC_SHARE_STAGE_BAD_STATE,
    MCC_SHARE_STAGE_TOO_MUCH,      /* more bytes than the manifest's size */
    MCC_SHARE_STAGE_BAD_HASH,
    MCC_SHARE_STAGE_WRITE_FAILED,
    MCC_SHARE_STAGE_REFUSED,       /* the identity check failed */
    MCC_SHARE_STAGE_EXISTS,        /* the package folder or its name is already taken */
    MCC_SHARE_STAGE_INSTALL_FAILED,
    MCC_SHARE_STAGE_STORAGE_UNCERTAIN,
    MCC_SHARE_STAGE_PENDING
};

enum mcc_share_check_result {
    MCC_SHARE_CHECK_FAILED,
    MCC_SHARE_CHECK_READY,
    MCC_SHARE_CHECK_PENDING
};

struct mcc_share_stage {
    struct mcc_share_io io;
    enum mcc_share_stage_state state;
    char root[200];                   /* prefix of every path, "" or ending in a separator */
    char folder[64];                  /* the package folder name, one component */
    char level[MCC_SHARE_LEAF_SIZE];  /* the level's file name without .map */
    char stage_dir[260];              /* native staging, or the hidden direct destination */
    char package_dir[260];            /* <root>mcc_maps/<folder> */
    struct mcc_share_file files[MCC_SHARE_STAGE_FILES];
    int count;
    int current;                      /* the file being written */
    uint32_t written;                 /* bytes of it written */
    uint32_t total, done;             /* every file's bytes, and written so far */
    struct mcc_sha256 hash;
    FILE *stream;
    /* Optional private identity job; release before any owned file cleanup.
     * The session binds this after begin and retains it through publication. */
    void (*cancel_check)(void *user);
    void *check_user;
    int owns_directory, owns_partial, verified, direct_active;
    unsigned char owned[MCC_SHARE_STAGE_FILES];
    /* Locally generated only, after verification; never network manifest entries. */
    unsigned char companion_owned[MCC_SHARE_STAGE_COMPANIONS];
    uint32_t companion_size[MCC_SHARE_STAGE_COMPANIONS];
    unsigned char companion_sha256[MCC_SHARE_STAGE_COMPANIONS][32];
};

/* The leaf a credit file may have (credits.txt, readme.txt, license.txt),
 * compared without case. Bounded by length; no terminator needed beyond it. */
int mcc_share_credit_allowed(char const *leaf, unsigned length);
/* Whether a manifest is acceptable for the level (its file name, no extension):
 * exactly one map named <level>.map, at most one bitmaps.map and one sounds.map
 * (the names mcc_share_file_allowed passes), at most four credit files, no repeated
 * name, bounded metadata/images and no file/directory path collisions.
 * JSON/PNG references and contents are validated separately before install. */
int mcc_share_manifest_valid(struct mcc_share_file const *files, int count, char const *level);

/* Exclusively create <root>mcc_staging/<folder>, or an undiscoverable final
 * destination through io.direct. A staging folder of that name left by a
 * download cut off by a crash is cleared of the files the manifest names and
 * created again; one holding anything else is left, and the stage fails.
 * Existing destinations are never reused or cleaned. The caller supplies a managed snapshot destination
 * distinct from original installations. The IO table and strings are copied. */
enum mcc_share_stage_result mcc_share_stage_begin(struct mcc_share_stage *stage,
    struct mcc_share_io const *io, char const *root, char const *folder, char const *level,
    struct mcc_share_file const *files, int count);
/* The next bytes of the current file, in order. FILE_DONE and ALL_DONE mean the
 * file just matched its size and SHA-256 and was closed/flushed; native staging
 * also renames the completed .part file inside its owned directory. */
enum mcc_share_stage_result mcc_share_stage_write(struct mcc_share_stage *stage,
    void const *data, uint32_t bytes);
/* Bytes still wanted of the current file (0 when staged). */
uint32_t mcc_share_stage_wanted(struct mcc_share_stage const *stage);
/* Poll the identity check on the staged files. The callback returns FAILED,
 * READY or PENDING; pending does not grant permission to publish. */
enum mcc_share_stage_result mcc_share_stage_verify(struct mcc_share_stage *stage,
    int (*check)(void *user, char const *stage_dir), void *user);
/* After successful verification, publish the complete directory through the
 * platform strategy. Existing installations are never replaced. */
enum mcc_share_stage_result mcc_share_stage_install(struct mcc_share_stage *stage);
/* Delete only this transfer's owned files/directories. */
void mcc_share_stage_cancel(struct mcc_share_stage *stage);
/* The metadata validator may discard an unsupported optional preview before
 * publication. Required content cannot be discarded through this interface. */
int mcc_share_stage_discard_preview(struct mcc_share_stage *stage, int index);
/* Exclusively create workshop_id.txt or download.receipt in the verified,
 * owned stage. Generated files follow the same failed-install cleanup. */
int mcc_share_stage_companion(struct mcc_share_stage *stage, char const *leaf,
    void const *data, unsigned bytes);
#endif
