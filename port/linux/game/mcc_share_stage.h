/* Staged MCC package downloads: the files of a package a host sends are
 * written to a staging folder, checked there, and only then moved into
 * mcc_maps\<package>. Cancelling or failing deletes the staging files and
 * nothing else; no existing package or save is opened for writing.
 *
 * This module has no network and no engine dependencies (standard C file I/O
 * and a three-function folder table), so its tests run the real code. */
#ifndef MCC_SHARE_STAGE_H
#define MCC_SHARE_STAGE_H
#include <stdint.h>
#include <stdio.h>
#include "mcc_sha256.h"

#define MCC_SHARE_STAGE_FILES 8
#define MCC_SHARE_LEAF_SIZE 32
/* One map and its resource maps are large; a credit file is a small text. */
#define MCC_SHARE_MAXIMUM_FILE_BYTES 0x30000000u
#define MCC_SHARE_MAXIMUM_TOTAL_BYTES 0x40000000u
#define MCC_SHARE_MAXIMUM_CREDIT_BYTES 0x10000u
#define MCC_SHARE_MAXIMUM_CREDITS 4

enum mcc_share_file_kind {
    MCC_SHARE_FILE_MAP,          /* <level>.map */
    MCC_SHARE_FILE_BITMAPS,      /* bitmaps.map of the package */
    MCC_SHARE_FILE_SOUNDS,       /* sounds.map of the package */
    MCC_SHARE_FILE_CREDIT,       /* maker and licence text that travels with the package */
    MCC_SHARE_FILE_KINDS
};

struct mcc_share_file {
    uint8_t kind;
    char leaf[MCC_SHARE_LEAF_SIZE];   /* file name inside the package, NUL ended */
    uint32_t size;
    uint8_t sha256[32];
};

/* The file and folder operations the platform layers provide in their own way (the game
 * uses its Win32 file calls, since stdio's rename and remove are not available in every
 * build). Each returns nonzero on success. make_directory fails when the folder already
 * exists, move_file when the destination does. File contents go through stdio. */
struct mcc_share_io {
    int (*make_directory)(char const *path);
    int (*remove_directory)(char const *path);
    int (*directory_exists)(char const *path);
    int (*move_file)(char const *from, char const *to);
    int (*delete_file)(char const *path);
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
    MCC_SHARE_STAGE_INSTALL_FAILED
};

struct mcc_share_stage {
    struct mcc_share_io io;
    enum mcc_share_stage_state state;
    char root[96];                    /* prefix of every path, "" or ending in a separator */
    char folder[64];                  /* the package folder name, one component */
    char level[MCC_SHARE_LEAF_SIZE];  /* the level's file name without .map */
    char stage_dir[200];              /* <root>mcc_staging/<folder> */
    char package_dir[200];            /* <root>mcc_maps/<folder> */
    struct mcc_share_file files[MCC_SHARE_STAGE_FILES];
    int count;
    int current;                      /* the file being written */
    uint32_t written;                 /* bytes of it written */
    uint32_t total, done;             /* every file's bytes, and written so far */
    struct mcc_sha256 hash;
    FILE *stream;
};

/* The leaf a credit file may have (ModInfo.json, credits.txt, readme.txt, license.txt),
 * compared without case. Bounded by length; no terminator needed beyond it. */
int mcc_share_credit_allowed(char const *leaf, unsigned length);
/* Whether a manifest is acceptable for the level (its file name, no extension):
 * exactly one map named <level>.map, at most one bitmaps.map and one sounds.map
 * (the names mcc_share_file_allowed passes), at most four credit files, no repeated
 * name, every size within the limits, and every leaf a plain file name. */
int mcc_share_manifest_valid(struct mcc_share_file const *files, int count, char const *level);

/* Prepare a stage: makes <root>mcc_staging/<folder>, clearing leftover files of the
 * same names. Fails when the manifest is invalid or the package folder name
 * already exists. The caller checks the package against the installed maps
 * (a Workshop item or stock map of the same name) first. The io table and
 * strings are copied. */
enum mcc_share_stage_result mcc_share_stage_begin(struct mcc_share_stage *stage,
    struct mcc_share_io const *io, char const *root, char const *folder, char const *level,
    struct mcc_share_file const *files, int count);
/* The next bytes of the current file, in order. FILE_DONE and ALL_DONE mean the
 * file just matched its size and SHA-256 and was renamed in the staging folder. */
enum mcc_share_stage_result mcc_share_stage_write(struct mcc_share_stage *stage,
    void const *data, uint32_t bytes);
/* Bytes still wanted of the current file (0 when staged). */
uint32_t mcc_share_stage_wanted(struct mcc_share_stage const *stage);
/* Run the identity check on the staged files (the callback is given the staging
 * folder, without a trailing separator). Nothing moves out of the staging folder. */
enum mcc_share_stage_result mcc_share_stage_verify(struct mcc_share_stage *stage,
    int (*check)(void *user, char const *stage_dir), void *user);
/* Move the staged files into <root>mcc_maps/<folder>, the map last. The folder
 * must not exist. On any failure what was moved is deleted again. */
enum mcc_share_stage_result mcc_share_stage_install(struct mcc_share_stage *stage);
/* Delete this stage's staging files and folder. Never touches mcc_maps. After an
 * install it only removes the empty staging folder. */
void mcc_share_stage_cancel(struct mcc_share_stage *stage);
#endif
