/* MCC session identity is independent of a client's installation folder. */
#ifndef MCC_IDENTITY_H
#define MCC_IDENTITY_H
struct network_game;
struct mcc_runtime;
struct mcc_cache_source;
void const *mcc_identity_host_settings(void const *game, long bytes, struct network_game *copy);
int mcc_identity_accept(struct network_game const *game);
void mcc_identity_disconnect(void);
char const *mcc_identity_local_name(char const *name);
int mcc_identity_check(char const *name);
int mcc_identity_loaded(char const *name, struct mcc_runtime *runtime);
int mcc_content_probe(char const *name, unsigned char digest[32]);
int mcc_resources_fingerprint(struct mcc_runtime *runtime, unsigned char digest[32]);
int mcc_resources_fingerprint_probe(char const *name, struct mcc_cache_source const *source,
    unsigned char digest[32]);
#endif
