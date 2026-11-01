#include "cseries.h"
#include "third_person_protocol.h"
#include "mcc_protocol.h"
#include "third_person.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"

char const *config_string(char const *name);
int config_write(char const *name, char const *value);
struct network_game *network_game_server_get_game(struct network_game_server *server);
boolean network_game_server_send_game_data_pregame(struct network_game_server *server);

/* The server owns the mode for its whole lifetime. Client capability belongs
 * only to the selected connection; settings and local preferences cannot grant it. */
static int camera_host, camera_host_enabled, camera_locked, camera_client;

void third_person_session_host_begin(void)
{
    char const *mode = config_string("network.session_compatibility");
    camera_host = 1;
    camera_host_enabled = mode && !strcmp(mode, "gulchce_third_person");
    camera_locked = 0;
    camera_client = 0;
    third_person_reset();
}

void third_person_session_host_end(void)
{
    camera_host = camera_host_enabled = camera_locked = camera_client = 0;
    third_person_reset();
}

void third_person_session_lock(void) { if (camera_host) camera_locked = 1; }
int third_person_session_locked(void) { return camera_locked; }
int third_person_session_active(void) { return camera_host ? camera_host_enabled : camera_client; }

int third_person_session_set(int enabled)
{
    struct network_game_server *server = global_network_game_server_get();
    struct network_game *game;
    if (!server || !camera_host || camera_locked || (enabled != 0 && enabled != 1)) return 0;
    game = network_game_server_get_game(server);
    if (!game || game->machine_count > 1) return 0;
    camera_host_enabled = enabled;
    game->cooperative_flags &= ~THIRD_PERSON_POLICY_MASK;
    if (enabled) game->cooperative_flags |= third_person_host_policy() << THIRD_PERSON_POLICY_SHIFT;
    config_write("network.session_compatibility", enabled ? "gulchce_third_person" : "opence_crossplay");
    third_person_reset();
    network_game_server_send_game_data_pregame(server);
    return 1;
}

void third_person_session_client_begin(unsigned int version)
{
    camera_client = version == THIRD_PERSON_PROTOCOL_VERSION || version == THIRD_PERSON_PROTOCOL_MCC_VERSION;
    third_person_reset();
}

void third_person_session_client_clear(void)
{
    camera_client = 0;
    third_person_reset();
}

unsigned int third_person_session_version(char const *map)
{
    unsigned int base = mcc_protocol_version(map);
    if (!third_person_session_active()) return base;
    return base == MCC_PROTOCOL_VERSION ? THIRD_PERSON_PROTOCOL_MCC_VERSION : THIRD_PERSON_PROTOCOL_VERSION;
}

int third_person_session_compatible(char const *map, unsigned int version, unsigned int flags)
{
    unsigned int base = mcc_protocol_version(map);
    if (!(flags & HALO_PORT_ADVERTISED_DISTRIBUTED_FLAG)) return 0;
    if (base == MCC_PROTOCOL_VERSION && !(flags & MCC_PROTOCOL_ADVERTISED_FLAG)) return 0;
    return version == base || version == (base == MCC_PROTOCOL_VERSION ?
        THIRD_PERSON_PROTOCOL_MCC_VERSION : THIRD_PERSON_PROTOCOL_VERSION);
}
#include "networking/network_messages.h"
#include "bungie_net/common/message_header.h"

/* MCC retains ownership of its optional inner envelope and capability checks.
 * This outer envelope only establishes the camera branch's wire contract. */
static int third_person_protocol_inner_valid(byte const *message, unsigned short bytes)
{
    word header;
    if (bytes <= sizeof(header) || bytes > THIRD_PERSON_PROTOCOL_JOIN_MAX - 8) return 0;
    memcpy(&header, message, sizeof(header));
    if (GET_MESSAGE_FLAGS(header) || GET_MESSAGE_SIZE(header) != bytes) return 0;
    if (GET_MESSAGE_TYPE(header) == _message_type_packet)
        return message[bytes - 1] == _message_client_join_game_request;
    return GET_MESSAGE_TYPE(header) == 2 && bytes > 10 &&
        bytes <= MCC_PROTOCOL_JOIN_MAX && message[2] == MCC_PROTOCOL_JOIN_KIND;
}

void const *third_person_protocol_join(void const *message, unsigned short bytes, void *buffer)
{
    byte *out = buffer;
    if (!third_person_protocol_inner_valid(message, bytes)) return NULL;
    build_message_header((word *)out, bytes + 8, 2, 0);
    out[2] = THIRD_PERSON_PROTOCOL_JOIN_KIND;
    out[3] = THIRD_PERSON_PROTOCOL_JOIN_REVISION;
    out[4] = THIRD_PERSON_PROTOCOL_VERSION & 255;
    out[5] = THIRD_PERSON_PROTOCOL_VERSION >> 8;
    out[6] = out[7] = 0;
    memcpy(out + 8, message, bytes);
    return out;
}

/* Camera sessions require this proof before admission, even Forced Off.
 * OpenCE crossplay does not send or accept a camera envelope. */
int third_person_protocol_unwrap(void const *message, unsigned short bytes,
    void *inner, unsigned short *inner_bytes)
{
    byte const *in = message;
    word header;
    if (bytes <= 10 || bytes > THIRD_PERSON_PROTOCOL_JOIN_MAX ||
        in[2] != THIRD_PERSON_PROTOCOL_JOIN_KIND || in[3] != THIRD_PERSON_PROTOCOL_JOIN_REVISION ||
        in[4] != (THIRD_PERSON_PROTOCOL_VERSION & 255) ||
        in[5] != (THIRD_PERSON_PROTOCOL_VERSION >> 8) || in[6] || in[7]) return 0;
    memcpy(&header, in, sizeof(header));
    if (GET_MESSAGE_FLAGS(header) || GET_MESSAGE_TYPE(header) != 2 ||
        GET_MESSAGE_SIZE(header) != bytes || !third_person_protocol_inner_valid(in + 8, bytes - 8)) return 0;
    *inner_bytes = bytes - 8;
    memcpy(inner, in + 8, *inner_bytes);
    return 1;
}
