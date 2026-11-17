/* Transport-only MCC loading pulses. Never idle or dispatch the network here:
 * conversion owns incomplete tags and may already be inside message dispatch. */
#include "cseries.h"
#include "bungie_net/common/message_header.h"
#include "networking/network_game_globals.h"
#include "networking/network_game_manager.h"
#include "networking/network_client_manager.h"
#include "network_distributed.h"
#include "mcc_load_network.h"
#include "mcc_loading.h"
#include "mcc_identity.h"
#include "mcc_cache.h"
#include <string.h>
boolean network_distributed_client_send_reliably(void *, word);
boolean network_distributed_server_send_to_machine_reliably(long, void *, word);
unsigned long system_milliseconds(void);
struct network_game_server_client_machine *
network_game_server_get_client_machine_at_index(struct network_game_server *, long);
boolean
network_game_server_client_machine_is_joined_to_game(struct network_game_server *,
                                                     struct network_game_server_client_machine *);
static struct mcc_load_network loading;
/* (kept across rounds: a disconnect ends one round's wait, not the allowance) */
static struct mcc_load_allowance allowances[MCC_ADMISSION_MACHINES];
static uint32_t local_sequence, local_sent, host_revision;
static int local_active, local_stage;
static unsigned long local_done, local_total;

static void identity(struct network_game const *game, struct mcc_admission_identity *out)
{
    memcpy(out->name, game->map.name, 128);
    memcpy(out->content, game->mcc_content_sha256, 32);
    memcpy(out->metadata, game->mcc_metadata_sha256, 32);
    out->checksum = (uint32_t)game->map.version;
}
static int current(void)
{
    struct network_game *game = network_game_get_game();
    struct mcc_admission_identity value;
    if (!loading.active || !game || !mcc_level_name(game->map.name) ||
        (uint32_t)game->number_of_games_played != loading.round ||
        strcmp(game->map.name, loading.identity.name) ||
        (uint32_t)game->map.version != loading.identity.checksum)
        return 0;
    if (global_network_game_server_get())
        return host_revision && host_revision == (uint32_t)mcc_identity_host_revision();
    identity(game, &value);
    return !memcmp(&value, &loading.identity, sizeof(value));
}
static int peer_valid(long index)
{
    struct network_game_server *server = global_network_game_server_get();
    struct network_game_server_client_machine *machine;
    if (!server || !current() || index < 0 || index >= MCC_ADMISSION_MACHINES)
        return 0;
    machine = network_game_server_get_client_machine_at_index(server, index);
    return network_game_server_client_machine_is_joined_to_game(server, machine);
}
void mcc_load_network_start(struct network_game const *game)
{
    struct network_game copy;
    struct mcc_admission_identity value;
    /* Capture the already admitted identity before entering conversion. Do not
     * re-open metadata or restamp files from a loading pulse. */
    if (!game || !mcc_level_name(game->map.name)) {
        mcc_load_network_disconnect();
        return;
    }
    if (global_network_game_server_get()) {
        if (!mcc_identity_host_settings(game, sizeof(*game), &copy)) {
            mcc_load_network_disconnect();
            return;
        }
        game = &copy;
    }
    host_revision = global_network_game_server_get() ? (uint32_t)mcc_identity_host_revision() : 0;
    identity(game, &value);
    mcc_load_network_bind(&loading, &value, (uint32_t)game->number_of_games_played,
                          (uint32_t)system_milliseconds());
}
void mcc_load_network_begin(char const *name)
{
    if (local_active || !name || !mcc_level_name(name))
        return;
    if (!global_network_game_server_get())
        mcc_load_network_start(network_game_get_game());
    if (!current())
        return;
    local_active = 1;
    local_stage = -1;
    local_done = local_total = 0;
    local_sequence = 0;
    local_sent = (uint32_t)system_milliseconds() - MCC_LOAD_NETWORK_PULSE;
    mcc_load_network_pulse();
}
void mcc_load_network_pulse(void)
{
    struct {
        struct distributed_message_header header;
        struct mcc_load_message body;
    } message;
    uint32_t now = (uint32_t)system_milliseconds();
    long index;
    if (!local_active || !current() || (uint32_t)(now - loading.started) >= MCC_LOAD_NETWORK_LIMIT)
        return;
    /* Calls come from completed conversion work, not an idle frame timer. */
    if (local_sequence != UINT32_MAX)
        local_sequence++;
    if ((uint32_t)(now - local_sent) < MCC_LOAD_NETWORK_PULSE)
        return;
    local_sent = now;
    memset(&message, 0, sizeof(message));
    build_message_header(&message.header.header, sizeof(message), 2, 0);
    message.header.type = _distributed_message_mcc_load_progress;
    message.header.count = 1;
    message.body.round = loading.round;
    message.body.sequence = local_sequence;
    message.body.identity = loading.identity;
    if (global_network_game_server_get()) {
        for (index = 0; index < MCC_ADMISSION_MACHINES; index++)
            if (peer_valid(index))
                network_distributed_server_send_to_machine_reliably(index, &message,
                                                                    sizeof(message));
    } else if (global_network_game_client_get())
        network_distributed_client_send_reliably(&message, sizeof(message));
}
/* Stage/item reports remain meaningful when loading presentation is disabled.
 * Repeated render updates for the same item are not conversion progress. */
void mcc_load_network_work(int stage, unsigned long done, unsigned long total)
{
    if (!local_active || stage < 0 || stage >= MCC_LOADING_STAGES || stage < local_stage ||
        (total && done > total))
        return;
    if (stage > local_stage) {
        local_stage = stage;
        local_done = done;
        local_total = total;
    } else {
        if (!total || done <= local_done || (local_total && total != local_total))
            return;
        local_done = done;
        local_total = total;
    }
    mcc_load_network_pulse();
}
void mcc_load_network_end(void)
{
    local_active = 0;
}
void mcc_load_network_disconnect(void)
{
    memset(&loading, 0, sizeof(loading));
    local_active = 0;
    local_sequence = local_sent = host_revision = 0;
}
void mcc_load_network_peer_left(long index)
{
    mcc_load_network_forget(&loading, index);
}
void mcc_load_network_machine_left(long index)
{
    mcc_load_network_forget(&loading, index);
    if (index >= 0 && index < MCC_ADMISSION_MACHINES)
        mcc_load_allowance_forget(&allowances[index]);
}
int mcc_load_network_host_waiting(long index)
{
    uint32_t now = (uint32_t)system_milliseconds();
    if (!peer_valid(index) || !mcc_load_network_active(&loading, index, now))
        return 0;
    /* (the server asks only once the ordinary wait is over, every update) */
    if (!mcc_load_allowance_hold(&allowances[index], now)) {
        mcc_load_network_forget(&loading, index);
        return 0;
    }
    return 1;
}
int mcc_load_network_client_expired(void)
{
    struct network_game_client *client = global_network_game_client_get();
    if (global_network_game_server_get() || !client || !current())
        return 0;
    if (network_game_client_server_has_started_game(client)) {
        mcc_load_network_disconnect();
        return 0;
    }
    return (uint32_t)((uint32_t)system_milliseconds() - loading.started) >= MCC_LOAD_NETWORK_LIMIT;
}
int mcc_load_network_handles_message(void const *data, unsigned size)
{
    struct distributed_message_header header;
    if (!data || size < sizeof(header))
        return 0;
    memcpy(&header, data, sizeof(header));
    return header.type == _distributed_message_mcc_load_progress;
}
void mcc_load_network_handle_message(long index, void const *data, unsigned size, int reliable)
{
    struct distributed_message_header header;
    struct mcc_load_message message;
    struct network_game_client *client;
    struct mcc_admission_identity value;
    if (!reliable || !mcc_load_network_handles_message(data, size) ||
        size != sizeof(header) + sizeof(message) || !current())
        return;
    memcpy(&header, data, sizeof(header));
    memcpy(&message, (unsigned char const *)data + sizeof(header), sizeof(message));
    if (header.count != 1 || GET_MESSAGE_FLAGS(header.header) ||
        GET_MESSAGE_TYPE(header.header) != 2 || GET_MESSAGE_SIZE(header.header) != size)
        return;
    if (index == NONE) {
        client = global_network_game_client_get();
        if (global_network_game_server_get() || !client ||
            network_game_client_server_has_started_game(client))
            return;
        identity(network_game_client_get_game(client), &value);
        if (memcmp(&value, &loading.identity, sizeof(value)))
            return;
        index = 0;
    } else if (!peer_valid(index))
        return;
    mcc_load_network_receive(&loading, index, &message, (uint32_t)system_milliseconds());
}
