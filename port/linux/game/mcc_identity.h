/* MCC session identity is independent of a client's installation folder. */
#ifndef MCC_IDENTITY_H
#define MCC_IDENTITY_H
struct network_game;
struct mcc_runtime;
struct mcc_cache_source;
int mcc_identity_prepare_host(char const *name);
/* Changes when preparation selects different content or metadata; zero when invalid. */
unsigned long mcc_identity_host_revision(void);
void const *mcc_identity_host_settings(void const *game, long bytes, struct network_game *copy);
int mcc_identity_accept(struct network_game const *game);
/* The same judgement for a name and content identity (after a download has installed the package):
 * it never offers a download. */
int mcc_identity_accept_installed(char const name[128], unsigned char const digest[32],
    unsigned char const metadata[32],char const *installed);
/* A handler that may take over a join whose package matches nothing installed (it returns
 * nonzero once it has begun fetching it). */
void mcc_identity_set_missing(int (*handler)(char const name[128], unsigned char const digest[32],unsigned char const metadata[32]));
/* The content identity of the package this machine hosts, when level is that package. */
int mcc_identity_host_package(char const *level, unsigned char digest[32]);
void mcc_identity_disconnect(void);
char const *mcc_identity_local_name(char const *name);
int mcc_identity_check(char const *name);
int mcc_identity_loaded(char const *name, struct mcc_runtime *runtime);
int mcc_content_probe(char const *name, unsigned char digest[32]);
int mcc_resources_fingerprint(struct mcc_runtime *runtime, unsigned char digest[32]);
int mcc_resources_fingerprint_probe(char const *name, struct mcc_cache_source const *source,
    unsigned char digest[32]);
#endif
