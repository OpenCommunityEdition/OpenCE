/* Map sharing rules for MCC packages: may a host offer this map to a joiner?
 * Pure decisions only; no files, no network. The caller reads the facts (the
 * Workshop sidecar, whether the content fingerprint is known) and passes them in. */
#ifndef MCC_SHARE_RULES_H
#define MCC_SHARE_RULES_H

enum mcc_share_sidecar {
    MCC_SHARE_SIDECAR_ABSENT,      /* the package has no workshop_id.txt */
    MCC_SHARE_SIDECAR_VALID,       /* it holds a valid Workshop item id */
    MCC_SHARE_SIDECAR_UNREADABLE   /* present, but unreadable or not a valid id */
};

enum mcc_share_reason {
    MCC_SHARE_ELIGIBLE,            /* a hand-placed package: may be offered */
    MCC_SHARE_NOT_OPTED_IN,        /* the host has not turned sharing on */
    MCC_SHARE_BAD_NAME,            /* not a safe mcc_maps identity */
    MCC_SHARE_NOT_A_PACKAGE,       /* a loose map: never offered */
    MCC_SHARE_SHARED_RESOURCES,    /* the _shared resource folder: never offered */
    MCC_SHARE_STOCK,               /* a stock map name: never offered */
    MCC_SHARE_WORKSHOP,            /* a Workshop item: the joiner is pointed to Steam */
    MCC_SHARE_UNKNOWN_ORIGIN,      /* the origin cannot be read: treated as unknown, not offered */
    MCC_SHARE_UNKNOWN_CONTENT      /* no content fingerprint: not offered */
};

struct mcc_share_package {
    char const *name;              /* canonical identity, mcc_maps\<package>\<level> */
    enum mcc_share_sidecar sidecar;
    int content_known;             /* nonzero when the SHA-256 content identity is known */
    int host_opt_in;               /* nonzero when the host allows sharing */
};

enum mcc_share_reason mcc_share_decide(struct mcc_share_package const *package);
int mcc_share_eligible(struct mcc_share_package const *package);
/* A short stable word for logs and tests, such as "workshop". */
char const *mcc_share_reason_word(enum mcc_share_reason reason);
/* The only files that travel with a package, besides its level: bitmaps.map and
 * sounds.map, by leaf name. Anything else, any path, is refused. */
int mcc_share_file_allowed(char const *leaf);
#endif
