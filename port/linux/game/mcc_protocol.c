#include "cseries.h"
#include "mcc_protocol.h"
#include "mcc_cache.h"
#include "networking/network_game_manager.h"
#include "networking/network_messages.h"
#include "bungie_net/common/message_header.h"
#include <stddef.h>

/* The base record is exactly OpenCE version 32, through local_data. */
typedef char mcc_protocol_base_layout[
    offsetof(struct network_game, mcc_content_sha256) ==
        HALO_PORT_NETWORK_GAME_PLAYERS_END + 0x2A ? 1 : -1];

static struct network_game_server_client_machine *mcc_protocol_peers[HALO_PORT_MAXIMUM_NETWORK_MACHINES];
static int mcc_protocol_client_capable;

struct network_game_server_client_machine *network_game_server_get_client_machine_at_index(
    struct network_game_server *server, long index);
boolean network_game_server_client_machine_is_joined_to_game(struct network_game_server *server,
    struct network_game_server_client_machine *machine);
boolean network_game_server_remove_client_machine_from_game(struct network_game_server *server,
    struct network_game_server_client_machine *machine);

unsigned int mcc_protocol_version(char const *map)
{
    return mcc_level_name(map) ? MCC_PROTOCOL_VERSION : HALO_PORT_NETWORK_VERSION;
}

long mcc_protocol_settings_size(struct network_game const *game)
{
    return mcc_level_name(game->map.name) ? sizeof(*game) :
        offsetof(struct network_game, mcc_content_sha256);
}

int mcc_protocol_settings_valid(struct network_game const *game, long bytes)
{
    if (!memchr(game->map.name, 0, sizeof(game->map.name))) return 0;
    return bytes == mcc_protocol_settings_size(game) &&
        (!mcc_level_name(game->map.name) || mcc_protocol_client_capable);
}

void mcc_protocol_client_begin(unsigned int flags)
{
    mcc_protocol_client_capable = (flags & MCC_PROTOCOL_ADVERTISED_FLAG) != 0;
}

void mcc_protocol_client_clear(void) { mcc_protocol_client_capable = 0; }

void const *mcc_protocol_join(void const *message, unsigned short bytes, void *buffer)
{
    byte *out = buffer;
    if (!mcc_protocol_client_capable) return message;
    if (bytes <= 2 || bytes > MCC_PROTOCOL_JOIN_MAX - 8) return NULL;
    build_message_header((word *)out, bytes + 8, 2, 0);
    out[2] = MCC_PROTOCOL_JOIN_KIND;
    out[3] = 1;
    out[4] = MCC_PROTOCOL_VERSION & 255;
    out[5] = MCC_PROTOCOL_VERSION >> 8;
    out[6] = out[7] = 0;
    memcpy(out + 8, message, bytes);
    return out;
}

/* Only the reliable pre-admission dispatcher calls this. No recursive envelopes,
 * data messages or arbitrary legacy packets can enter the ordinary join handler. */
int mcc_protocol_unwrap(void const *message, unsigned short bytes,
    void *inner, unsigned short *inner_bytes)
{
    byte const *in = message;
    word header;
    if (bytes <= 10 || bytes > MCC_PROTOCOL_JOIN_MAX || in[2] != MCC_PROTOCOL_JOIN_KIND ||
        in[3] != 1 || in[4] != (MCC_PROTOCOL_VERSION & 255) ||
        in[5] != (MCC_PROTOCOL_VERSION >> 8) || in[6] || in[7]) return 0;
    memcpy(&header, in, sizeof(header));
    if (GET_MESSAGE_FLAGS(header) || GET_MESSAGE_TYPE(header) != 2 || GET_MESSAGE_SIZE(header) != bytes) return 0;
    memcpy(&header, in + 8, sizeof(header));
    if (GET_MESSAGE_FLAGS(header) || GET_MESSAGE_TYPE(header) != _message_type_packet ||
        GET_MESSAGE_SIZE(header) != bytes - 8 || in[bytes - 1] != _message_client_join_game_request)
        return 0;
    *inner_bytes = bytes - 8;
    memcpy(inner, in + 8, *inner_bytes);
    return 1;
}

void mcc_protocol_forget(struct network_game_server_client_machine *machine)
{
    unsigned int i;
    for (i = 0; i < NUMBEROF(mcc_protocol_peers); i++)
        if (mcc_protocol_peers[i] == machine) mcc_protocol_peers[i] = NULL;
}

int mcc_protocol_peer(struct network_game_server_client_machine *machine, int capable)
{
    unsigned int i;
    if (!machine) return 0;
    mcc_protocol_forget(machine);
    if (!capable) return 1;
    for (i = 0; i < NUMBEROF(mcc_protocol_peers); i++) {
        if (!mcc_protocol_peers[i]) {
            mcc_protocol_peers[i] = machine;
            return 1;
        }
    }
    return 0;
}

void mcc_protocol_reset(void) { memset(mcc_protocol_peers, 0, sizeof(mcc_protocol_peers)); }

int mcc_protocol_allows(struct network_game const *game,
    struct network_game_server_client_machine *machine)
{
    unsigned int i;
    if (!machine) return 0;
    if (!mcc_level_name(game->map.name)) return 1;
    for (i = 0; i < NUMBEROF(mcc_protocol_peers); i++)
        if (mcc_protocol_peers[i] == machine) return 1;
    network_event("MCC game requires a GulchCE client using MCC protocol %u", MCC_PROTOCOL_VERSION);
    return 0;
}

void mcc_protocol_prune(struct network_game_server *server, struct network_game const *game)
{
    long i;
    if (!mcc_level_name(game->map.name)) return;
    for (i = 0; i < HALO_PORT_MAXIMUM_NETWORK_MACHINES; i++) {
        struct network_game_server_client_machine *machine =
            network_game_server_get_client_machine_at_index(server, i);
        if (network_game_server_client_machine_is_joined_to_game(server, machine) &&
            !mcc_protocol_allows(game, machine))
            network_game_server_remove_client_machine_from_game(server, machine);
    }
}
