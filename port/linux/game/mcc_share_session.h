/* MCC package sharing between a host and a joiner: the request and response
 * messages, the host's serving of one package and the joiner's staged download.
 *
 * This is a message pair of its own for MCC games (distributed messages 102 and
 * 103, mcc_share.c carries them); the Custom Edition map sharing messages 83-86
 * are not used and not changed, and a machine that does not play MCC protocol
 * 65 never sees these. The core here has no engine dependencies: everything it
 * needs from the game (the clock, the connection, the host's admission, the
 * joiner's identity check) comes through struct mcc_share_env, so tests run
 * a host and a joiner against each other with real files. */
#ifndef MCC_SHARE_SESSION_H
#define MCC_SHARE_SESSION_H
#include <stdint.h>
#include "mcc_share_stage.h"
#include "mcc_share_list.h"
#include "mcc_share_rules.h"

#define MCC_SHARE_LEVEL_SIZE 64
#define MCC_SHARE_CHUNK_BYTES 3072
#define MCC_SHARE_WINDOW 16
#define MCC_SHARE_HOST_TRANSFERS 4
#define MCC_SHARE_HOST_MACHINES 128

enum mcc_share_op {
    MCC_SHARE_OP_MANIFEST = 1,   /* request: what files make the level; response: the list */
    MCC_SHARE_OP_CHUNK = 2,      /* request: bytes of one file; response: those bytes */
    MCC_SHARE_OP_DONE = 3        /* request only: the joiner has finished (flags 1) or given up (0) */
};

enum mcc_share_result {
    MCC_SHARE_RESULT_OK,
    MCC_SHARE_RESULT_OFF,            /* the host does not share MCC packages */
    MCC_SHARE_RESULT_WORKSHOP,       /* a Workshop item: never sent */
    MCC_SHARE_RESULT_NOT_SHAREABLE,  /* stock, loose, _shared, unknown or not the game's map */
    MCC_SHARE_RESULT_NO_CREDIT,      /* no maker is credited for it: the joiner is pointed to the maker */
    MCC_SHARE_RESULT_BUSY,
    MCC_SHARE_RESULT_UNAVAILABLE,    /* the host could not read or hash the files */
    MCC_SHARE_RESULT_BAD_REQUEST,
    MCC_SHARE_RESULT_CHANGED         /* the transfer is not (or no longer) the host's */
};

/* What goes on the wire after the distributed message header. Integers are
 * the machines' own (all supported machines are little-endian). */
struct mcc_share_request {
    uint8_t op;
    uint8_t file;                    /* chunk: the manifest's file number */
    uint8_t flags;                   /* done: 1 for a successful install */
    uint8_t reserved;                /* zero */
    uint32_t session;                /* the host's number from its manifest; 0 for the manifest request */
    uint32_t offset;
    uint32_t length;                 /* chunk: at most MCC_SHARE_CHUNK_BYTES */
    char level[MCC_SHARE_LEVEL_SIZE];/* the game's level name, mcc_maps\<package>\<level> */
};
struct mcc_share_response {
    uint8_t op;
    uint8_t result;                  /* enum mcc_share_result */
    uint8_t file;
    uint8_t count;                   /* manifest: entries following */
    uint32_t session;
    uint32_t offset;
    uint32_t length;                 /* chunk: bytes following */
    char level[MCC_SHARE_LEVEL_SIZE];
    char credit[MCC_SHARE_CREDIT_SIZE]; /* manifest: the maker the host credits */
};
struct mcc_share_entry {
    uint8_t kind;                    /* enum mcc_share_file_kind */
    uint8_t reserved[3];
    uint32_t size;
    char leaf[MCC_SHARE_LEAF_SIZE];
    uint8_t sha256[32];
};
typedef char mcc_share_request_size[sizeof(struct mcc_share_request) == 80 ? 1 : -1];
typedef char mcc_share_response_size[sizeof(struct mcc_share_response) == 144 ? 1 : -1];
typedef char mcc_share_entry_size[sizeof(struct mcc_share_entry) == 72 ? 1 : -1];
/* The largest response: a head and either a full manifest or a full chunk. */
#define MCC_SHARE_RESPONSE_MAXIMUM (sizeof(struct mcc_share_response) + MCC_SHARE_CHUNK_BYTES)

/* What the host's admission found for the game's package. */
struct mcc_share_host_package {
    enum mcc_share_result refusal;   /* OK when the package may be offered */
    char credit[MCC_SHARE_CREDIT_SIZE];
    int count;
    struct {
        uint8_t kind;
        char leaf[MCC_SHARE_LEAF_SIZE];
        char path[260];              /* where to read it */
    } files[MCC_SHARE_STAGE_FILES];
};

struct mcc_share_env {
    void *user;
    uint32_t (*now)(void *user);                                 /* milliseconds */
    /* joiner */
    void (*send_request)(void *user, struct mcc_share_request const *request);
    int (*package_taken)(void *user, char const *level);         /* an installed map has this name */
    int (*identity_check)(void *user, char const *stage_dir);    /* the staged files are the host's content */
    void (*finished)(void *user, int ok, char const *message);   /* installed, or why not */
    /* host */
    void (*send_response)(void *user, long machine, void const *message, unsigned bytes);
    /* Admission for a request naming a level: sets package->refusal, and when OK the files.
     * The core still checks the manifest it builds. */
    void (*host_package)(void *user, char const *level, struct mcc_share_host_package *package);
    uint32_t (*upload_limit)(void *user);                        /* bytes a second */
    /* joiner: the bytes free where the package will go, or 0 when that is not known */
    uint64_t (*free_bytes)(void *user);
    struct mcc_share_io io;
    char root[200];                  /* prefix of the staging and mcc_maps folders, "" or ending in / */
};

struct mcc_share_host_transfer {
    int used;
    long machine;
    int hashing;                     /* nonzero until every file has been hashed */
    uint32_t session, heard_at, advanced_at, budget_at;
    int32_t budget;                  /* bytes that may still go; negative after a chunk the limit had not yet paid for */
    char level[MCC_SHARE_LEVEL_SIZE];
    char credit[MCC_SHARE_CREDIT_SIZE];
    struct mcc_share_host_package package;
    struct mcc_share_file files[MCC_SHARE_STAGE_FILES];
    int hash_file;                   /* hashing: the file in hand and how far */
    uint32_t hash_offset;
    struct mcc_sha256 hash;
    FILE *hash_stream;
    struct {
        uint8_t file;
        uint32_t offset, length;
    } queue[MCC_SHARE_WINDOW * 2];
    int queued;
    uint32_t sent_bytes;
};

enum mcc_share_client_state {
    MCC_SHARE_CLIENT_IDLE,
    MCC_SHARE_CLIENT_ASKING,
    MCC_SHARE_CLIENT_TRANSFERRING,
    MCC_SHARE_CLIENT_VERIFYING
};

struct mcc_share {
    struct mcc_share_env env;
    struct mcc_share_host_transfer transfers[MCC_SHARE_HOST_TRANSFERS];
    uint32_t next_session;
    uint32_t manifest_at[MCC_SHARE_HOST_MACHINES];  /* the last manifest request, per machine */
    /* the joiner's download */
    enum mcc_share_client_state state;
    char level[MCC_SHARE_LEVEL_SIZE];
    uint8_t digest[32];
    uint32_t session, asked_at, progress_at, reported_at;
    int request_file;
    uint32_t request_offset;
    int outstanding;
    char credit[MCC_SHARE_CREDIT_SIZE];
    struct mcc_share_stage stage;
};

/* The host's admission for a level: the maintained rules (mcc_share_decide, unchanged)
 * decide eligibility from the facts the game established locally; the host's list
 * (mcc_share_list.h) is the classification: the content is custom only when its exact
 * fingerprint is listed, and it is offered only with a maker to credit. Anything
 * unknown is refused. credit is the maker when the result is OK. */
enum mcc_share_result mcc_share_admit(char const *level, int opt_in, enum mcc_share_sidecar sidecar,
    unsigned char const digest[32], int digest_known, char const *list, unsigned long list_length,
    char credit[MCC_SHARE_CREDIT_SIZE]);

void mcc_share_init(struct mcc_share *share, struct mcc_share_env const *env);
/* Every frame: the host hashes and serves, the joiner checks timeouts and
 * finishes (verify, install, report). */
void mcc_share_update(struct mcc_share *share);

/* ---------- the host */
void mcc_share_host_request(struct mcc_share *share, long machine, void const *message, unsigned bytes);
/* Whether the machine's transfer is going on (it asked lately and its bytes grew lately). */
int mcc_share_host_active(struct mcc_share const *share, long machine);
void mcc_share_host_forget(struct mcc_share *share, long machine);
/* The game ended or this machine stopped hosting: drop every transfer. */
void mcc_share_host_stop(struct mcc_share *share);

/* ---------- the joiner */
/* Ask the host for the level's package. digest is the host's content identity
 * (the game record's). 0 when the name is not a shareable package name or a
 * download is already going. */
int mcc_share_client_begin(struct mcc_share *share, char const *level, unsigned char const digest[32]);
void mcc_share_client_response(struct mcc_share *share, void const *message, unsigned bytes);
void mcc_share_client_cancel(struct mcc_share *share);
int mcc_share_client_busy(struct mcc_share const *share);
int mcc_share_client_level(struct mcc_share const *share, char const *level);
/* 0 to 100, or -1 when no download goes on */
int mcc_share_client_percent(struct mcc_share const *share);
void mcc_share_refusal_text(enum mcc_share_result result, char const *level, char *text, unsigned size);
#endif
