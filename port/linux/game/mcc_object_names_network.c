/* MCC-only bounded name snapshots. Moving units/items retain native replication;
 * this supplies named scenery/devices that scripts create or destroy. */
#include "cseries.h"
#include "mcc_object_names.h"
#include "mcc_object_names_network.h"
#include "game/game.h"
#include "game/players.h"
#include "objects/objects.h"
#include "scenario/scenario.h"
#include "scenario/scenario_definitions.h"
#include "network_distributed.h"
#include <string.h>

#define MCC_NAME_CHUNK 512
#define MCC_NAME_CHUNKS ((MCC_OBJECT_NAMES_MAXIMUM + MCC_NAME_CHUNK - 1) / MCC_NAME_CHUNK)
struct mcc_name_chunk {
    short bsp, total, first, count;
    byte present[MCC_NAME_CHUNK / 8];
};
typedef char mcc_name_chunk_size[sizeof(struct mcc_name_chunk) == 72 ? 1 : -1];
static byte differing[MCC_NAME_CHUNKS][MCC_NAME_CHUNK / 8];
static long received[MCC_NAME_CHUNKS];
static short received_bsp = NONE;

void mcc_object_names_network_reset(void)
{
    memset(differing, 0, sizeof(differing));
    memset(received, 0xFF, sizeof(received));
    received_bsp = NONE;
}

word mcc_object_names_entry_size(void) { return sizeof(struct mcc_name_chunk); }

void mcc_object_names_host_tick(void)
{
    struct {
        struct distributed_message_header header;
        struct mcc_name_chunk chunk;
    } message;
    long first, i, total = mcc_object_names_count();
    if (!total || game_connection() != _game_connection_network_server || game_time_get() % 15) return;
    for (first = 0; first < total; first += MCC_NAME_CHUNK) {
        memset(&message, 0, sizeof(message));
        message.chunk.bsp = global_structure_bsp_index_get();
        message.chunk.total = (short)total;
        message.chunk.first = (short)first;
        message.chunk.count = (short)MIN(MCC_NAME_CHUNK, total - first);
        for (i = 0; i < message.chunk.count; ++i)
            if (mcc_object_names_get((short)(first + i)) != NONE)
                message.chunk.present[i / 8] |= (byte)(1u << (i % 8));
        distributed_send(&message, _distributed_message_mcc_object_names, 1,
            sizeof(message), _distributed_to_clients);
    }
}

static int mcc_names_holds_player(long object_index)
{
    long child;
    if (player_index_from_unit_index(object_index) != NONE) return 1;
    for (child = object_get(object_index)->object.first_child_object_index; child != NONE;
        child = object_get(child)->object.next_object_index)
        if (mcc_names_holds_player(child)) return 1;
    return 0;
}

void mcc_object_names_receive(void const *entries, short count, long host_time)
{
    struct mcc_name_chunk chunk;
    struct scenario *scenario;
    long i, slot, total = mcc_object_names_count();
    if (!total || game_connection() != _game_connection_network_client || count != 1 || host_time < 0) return;
    memcpy(&chunk, entries, sizeof(chunk));
    if (chunk.total != total || chunk.first < 0 || chunk.first >= total ||
        chunk.first % MCC_NAME_CHUNK || chunk.count != MIN(MCC_NAME_CHUNK, total - chunk.first)) return;
    for (i = chunk.count; i < MCC_NAME_CHUNK; ++i)
        if (chunk.present[i / 8] & (1u << (i % 8))) return;
    if (chunk.bsp != global_structure_bsp_index_get()) {
        mcc_object_names_network_reset();
        return;
    }
    if (received_bsp != chunk.bsp) {
        mcc_object_names_network_reset();
        received_bsp = chunk.bsp;
    }
    slot = chunk.first / MCC_NAME_CHUNK;
    if (host_time <= received[slot]) return;
    received[slot] = host_time;
    scenario = global_scenario_get();
    for (i = 0; i < chunk.count; ++i) {
        short name = (short)(chunk.first + i);
        short type = TAG_BLOCK_GET_ELEMENT(&scenario->object_names, name, struct scenario_object_name)->runtime_object_type;
        long object = object_index_from_name_index(name);
        byte bit = (byte)(1u << (i % 8)), *changed = &differing[slot][i / 8];
        int present = (chunk.present[i / 8] & bit) != 0;
        if (type < 0 || type >= NUMBER_OF_OBJECT_TYPES ||
            TEST_FLAG(_object_mask_biped | _object_mask_vehicle | _object_mask_weapon | _object_mask_equipment, type) ||
            present == (object != NONE)) {
            *changed &= (byte)~bit;
            continue;
        }
        /* Two distinct host snapshots confirm a difference, as in native co-op. */
        if (!(*changed & bit)) { *changed |= bit; continue; }
        *changed &= (byte)~bit;
        if (present) object_new_by_name(name);
        else if (!mcc_names_holds_player(object)) object_delete(object);
    }
}
