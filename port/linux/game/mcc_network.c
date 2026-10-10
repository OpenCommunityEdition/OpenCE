#include "cseries.h"
#include "mcc_network.h"
#include "mcc_grenades.h"
#include "mcc_campaign.h"
#include "mcc_cache.h"
#include "game/game.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "network_distributed.h"
#include <math.h>

struct mcc_network_inventory {
    long unit;
    byte counts[2];
    byte reserved[2];
};
typedef char mcc_network_inventory_size[sizeof(struct mcc_network_inventory) == 8 ? 1 : -1];
struct mcc_network_history {
    long unit;
    byte counts[2];
    long changed[2];
    long sent;
};
static struct mcc_network_history mcc_network_sent[MAXIMUM_TRACKED_OBJECTS];
static struct mcc_network_history mcc_network_own[MAXIMUM_TRACKED_OBJECTS];
static long mcc_network_last_tick = NONE;

void mcc_network_new_game(void)
{
    long slot;
    csmemset(mcc_network_sent, 0, sizeof(mcc_network_sent));
    csmemset(mcc_network_own, 0, sizeof(mcc_network_own));
    mcc_network_last_tick = NONE;
    for (slot = 0; slot < MAXIMUM_TRACKED_OBJECTS; ++slot) {
        mcc_network_sent[slot].unit = mcc_network_own[slot].unit = NONE;
        mcc_network_sent[slot].sent = NONE;
        mcc_network_own[slot].changed[0] = mcc_network_own[slot].changed[1] = NONE;
    }
}

static void mcc_network_observe_clock(long now)
{
    /* Checkpoint restores rewind simulation time but not these private caches. */
    if (mcc_network_last_tick != NONE && now < mcc_network_last_tick)
        mcc_network_new_game();
    mcc_network_last_tick = now;
}

word mcc_network_inventory_entry_size(void) { return sizeof(struct mcc_network_inventory); }
word mcc_network_campaign_entry_size(void) { return MCC_CAMPAIGN_SNAPSHOT_BYTES; }

static void mcc_network_campaign_tick(long now)
{
    struct {
        struct distributed_message_header header;
        byte state[MCC_CAMPAIGN_SNAPSHOT_BYTES];
    } message;
    if (!mcc_cache_tags_loaded() || game_connection()!=_game_connection_network_server || now%3) return;
    /* Complete replacement snapshots also repair lost packets and late joins.
     * OpenCE's normal host-only transport and stale-tick checks apply. */
    mcc_campaign_snapshot(message.state);
    distributed_send(&message,_distributed_message_mcc_campaign,1,sizeof(message),_distributed_to_clients);
}

void mcc_network_handle_campaign(void const *entries, short count)
{
    if (mcc_cache_tags_loaded() && game_connection()==_game_connection_network_client && count==1)
        mcc_campaign_restore(entries,MCC_CAMPAIGN_SNAPSHOT_BYTES);
}

void mcc_network_host_tick(void)
{
    struct {
        struct distributed_message_header header;
        struct mcc_network_inventory entries[64];
    } message;
    struct object_iterator iterator;
    short count = 0;
    long now = game_time_get();
    short limit = MIN(64, DATAGRAM_ENTRIES(struct mcc_network_inventory));
    mcc_network_campaign_tick(now);
    if (!mcc_grenades_active() || mcc_grenades_type_count() <= 2 ||
        game_connection() != _game_connection_network_server || now % 3) return;
    mcc_network_observe_clock(now);
    object_iterator_new(&iterator, _object_mask_unit, 0);
    while (object_iterator_next(&iterator)) {
        long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(iterator.index);
        struct mcc_network_history *previous;
        byte counts[2];
        if (slot < 0 || slot >= MAXIMUM_TRACKED_OBJECTS || !distributed_object_index_valid(iterator.index)) continue;
        counts[0] = (byte)mcc_grenades_get(iterator.index, 2);
        counts[1] = (byte)mcc_grenades_get(iterator.index, 3);
        previous = &mcc_network_sent[slot];
        if (previous->unit == iterator.index && previous->counts[0] == counts[0] && previous->counts[1] == counts[1] &&
            previous->sent != NONE && now - previous->sent < TICKS_PER_SECOND) continue;
        previous->unit = iterator.index; previous->counts[0] = counts[0]; previous->counts[1] = counts[1]; previous->sent = now;
        message.entries[count].unit = iterator.index;
        message.entries[count].counts[0] = counts[0]; message.entries[count].counts[1] = counts[1];
        message.entries[count].reserved[0] = message.entries[count].reserved[1] = 0;
        if (++count == limit) {
            distributed_send(&message, _distributed_message_mcc_grenades, count,
                (word)(sizeof(message.header) + count * sizeof(message.entries[0])), _distributed_to_clients);
            count = 0;
        }
    }
    if (count) distributed_send(&message, _distributed_message_mcc_grenades, count,
        (word)(sizeof(message.header) + count * sizeof(message.entries[0])), _distributed_to_clients);
}

static struct mcc_network_history *mcc_network_note_own(long unit_index, boolean compare)
{
    long slot = DATUM_INDEX_TO_ABSOLUTE_INDEX(unit_index);
    struct mcc_network_history *own;
    short type;
    if (slot < 0 || slot >= MAXIMUM_TRACKED_OBJECTS) return NULL;
    own = &mcc_network_own[slot];
    if (own->unit != unit_index) {
        own->unit = unit_index; compare = FALSE;
        own->changed[0] = own->changed[1] = NONE;
    }
    for (type = 0; type < 2; ++type) {
        byte count = (byte)mcc_grenades_get(unit_index, (short)(type + 2));
        if (compare && count < own->counts[type]) own->changed[type] = game_time_get();
        own->counts[type] = count;
    }
    return own;
}

void mcc_network_client_tick(void)
{
    short local;
    if (!mcc_grenades_active() || game_connection() != _game_connection_network_client) return;
    mcc_network_observe_clock(game_time_get());
    for (local = 0; local < MAXIMUM_LOCAL_PLAYERS; ++local) {
        long player_index = local_player_get_player_index(local);
        struct player_datum *player = player_index != NONE ? player_try_and_get(player_index) : NULL;
        long unit = distributed_living_unit(player);
        if (unit != NONE) mcc_network_note_own(unit, TRUE);
    }
}

void mcc_network_handle_inventories(void const *entries, short count)
{
    short entry;
    if (!mcc_grenades_active() || game_connection() != _game_connection_network_client || count < 0 || count > 255) return;
    mcc_network_observe_clock(game_time_get());
    for (entry = 0; entry < count; ++entry) {
        struct mcc_network_inventory inventory;
        struct unit_datum *unit;
        struct mcc_network_history *own = NULL;
        short type;
        real round_trip;
        csmemcpy(&inventory, (byte const *)entries + entry * sizeof(inventory), sizeof(inventory));
        if (inventory.reserved[0] || inventory.reserved[1] || inventory.counts[0] > 127 || inventory.counts[1] > 127 ||
            !distributed_object_index_valid(inventory.unit) || !network_objects_client_has(inventory.unit)) continue;
        unit = object_try_and_get_and_verify_type(inventory.unit, _object_mask_unit);
        if (!unit) continue;
        if (unit->unit.player_index != NONE && distributed_player_is_local(unit->unit.player_index))
            own = mcc_network_note_own(inventory.unit, TRUE);
        round_trip = distributed_own_round_trip_ticks();
        if (!(round_trip > 0.0f)) round_trip = 6.0f;
        if (round_trip > 60.0f) round_trip = 60.0f;
        for (type = 0; type < 2; ++type) {
            short current = mcc_grenades_get(inventory.unit, (short)(type + 2));
            if (!own || inventory.counts[type] < current || own->changed[type] == NONE ||
                game_time_get() - own->changed[type] > (long)ceil(round_trip) + 5)
                mcc_grenades_set(inventory.unit, (short)(type + 2), inventory.counts[type]);
        }
        if (own) mcc_network_note_own(inventory.unit, FALSE);
    }
}
