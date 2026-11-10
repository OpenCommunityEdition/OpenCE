/* MCC admission surrounds OpenCE's unchanged join request. It grants no authority. */
#ifndef MCC_PROTOCOL_H
#define MCC_PROTOCOL_H

/* OpenCE 41 owns messages through 95. MCC 65 agrees on camera-relative
 * projectile rays; the MCC 64 Workshop metadata framing is unchanged. */
#define MCC_PROTOCOL_VERSION 65
#define MCC_PROTOCOL_ADVERTISED_FLAG 0x08
#define MCC_PROTOCOL_ADVERTISED_VERSION_OFFSET 4
#define MCC_PROTOCOL_JOIN_KIND 100
/* The upstream 41 join carries a 170-byte pass and a 64-byte proof. */
#define MCC_PROTOCOL_JOIN_MAX 512

struct network_game;
struct network_game_server;
struct network_game_server_client_machine;
unsigned int mcc_protocol_version(char const *map);
long mcc_protocol_settings_size(struct network_game const *game);
int mcc_protocol_settings_valid(struct network_game const *game, long bytes);
void mcc_protocol_client_begin(unsigned int flags);
void mcc_protocol_client_clear(void);
void const *mcc_protocol_join(void const *message, unsigned short bytes, void *buffer);
int mcc_protocol_unwrap(void const *message, unsigned short bytes,
    void *inner, unsigned short *inner_bytes);
int mcc_protocol_peer(struct network_game_server_client_machine *machine, int capable);
void mcc_protocol_forget(struct network_game_server_client_machine *machine);
void mcc_protocol_reset(void);
int mcc_protocol_allows(struct network_game const *game,
    struct network_game_server_client_machine *machine);
void mcc_protocol_prune(struct network_game_server *server, struct network_game const *game);
#endif
