/* Host-established eligibility for local and Workshop MCC packages.
 * Content origin alone establishes neither permission nor byte safety. */
#ifndef MCC_SHARE_RULES_H
#define MCC_SHARE_RULES_H
#include <stddef.h>

enum mcc_share_sidecar {
    MCC_SHARE_SIDECAR_ABSENT,      /* the package has no workshop_id.txt */
    MCC_SHARE_SIDECAR_VALID,       /* it holds a valid Workshop item id */
    MCC_SHARE_SIDECAR_UNREADABLE   /* present, but unreadable or not a valid id */
};

enum mcc_share_content {
    MCC_SHARE_CONTENT_UNKNOWN,
    MCC_SHARE_CONTENT_CUSTOM,
    MCC_SHARE_CONTENT_STOCK
};

enum mcc_share_reason {
    MCC_SHARE_ELIGIBLE,            /* passes preliminary offer policy */
    MCC_SHARE_NOT_OPTED_IN,        /* the host has not turned sharing on */
    MCC_SHARE_BAD_NAME,            /* not a safe mcc_maps identity */
    MCC_SHARE_NOT_A_PACKAGE,       /* a loose map: never offered */
    MCC_SHARE_SHARED_RESOURCES,    /* the _shared resource folder: never offered */
    MCC_SHARE_STOCK,               /* identified stock content: never offered */
    MCC_SHARE_UNKNOWN_ORIGIN,      /* the origin cannot be read: treated as unknown, not offered */
    MCC_SHARE_UNKNOWN_CONTENT      /* no classification or fingerprint: not offered */
};

struct mcc_share_package {
    char const *name;              /* mcc_maps\<package>\<level>, either slash */
    size_t name_length;            /* readable bytes excluding NUL; no terminator needed */
    enum mcc_share_sidecar sidecar;
    /* Classification of the exact content, established locally by the caller.
     * Names, a missing sidecar and a newly computed hash do not establish CUSTOM.
     * Do not deserialize this struct or accept these facts from a remote host. */
    enum mcc_share_content content;
    int content_known;             /* nonzero when the SHA-256 content identity is known */
    int host_opt_in;               /* nonzero when the host allows sharing */
};

enum mcc_share_reason mcc_share_decide(struct mcc_share_package const *package);
int mcc_share_eligible(struct mcc_share_package const *package);
/* A short stable word for logs and tests, such as "not-opted-in". */
char const *mcc_share_reason_word(enum mcc_share_reason reason);
/* Preliminary resource leaf allowlist, not file validation or path containment.
 * Buffers must contain the declared number of readable bytes; no terminator
 * is required. Only bitmaps.map and sounds.map match, excluding their NUL. */
int mcc_share_file_allowed(char const *leaf, size_t length);
#endif
