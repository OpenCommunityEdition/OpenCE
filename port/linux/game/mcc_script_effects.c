/* Map-authored campaign effects, with native cheat permissions unchanged.
 * Clients use only the host's replicated prediction flags, never cheat globals. */
#include "mcc_script_effects.h"
#include "mcc_cache.h"
#include "game/game.h"
#include "game/cheats.h"
#include "game/players.h"
#include "objects/objects.h"
#include "units/units.h"
#include "units/bipeds.h"

static struct {
    char const *name;
    boolean *value;
} const mcc_effects[MCC_EFFECT_COUNT] = {
    {"cheat_jetpack", &cheat.jetpack},
    {"cheat_super_jump", &cheat.super_jump},
    {"cheat_bottomless_clip", &cheat.bottomless_clip},
    {"cheat_bump_possession", &cheat.bump_possession},
    {"cheat_medusa", &cheat.medusa},
    {"cheat_omnipotent", &cheat.omnipotent},
    {"cheat_deathless_player", &cheat.deathless_player}
};
static boolean mcc_effects_owned;
static unsigned long mcc_effects_original, mcc_effects_remote;

static unsigned long mcc_effects_native(void)
{
    unsigned long flags=0;
    short i;
    for (i=0;i<MCC_EFFECT_COUNT;i++) if (*mcc_effects[i].value) flags|=1u<<i;
    return flags;
}

static void mcc_effects_set_native(unsigned long flags)
{
    short i;
    for (i=0;i<MCC_EFFECT_COUNT;i++) *mcc_effects[i].value=(flags&(1u<<i))!=0;
}

boolean mcc_script_effect_global(char const *name)
{
    short i;
    if (!mcc_cache_tags_loaded() || !name) return FALSE;
    for (i=0;i<MCC_EFFECT_COUNT;i++) if (!csstrcasecmp(name,mcc_effects[i].name)) return TRUE;
    return FALSE;
}

void mcc_script_effects_begin(void)
{
    if (!mcc_effects_owned) mcc_effects_original=mcc_effects_native();
    mcc_effects_owned=TRUE;
    mcc_effects_remote=0;
    mcc_effects_set_native(0);
}

void mcc_script_effects_dispose(void)
{
    if (mcc_effects_owned) mcc_effects_set_native(
        game_connection()==_game_connection_network_client ? 0 : mcc_effects_original);
    mcc_effects_owned=FALSE;
    mcc_effects_remote=0;
}

unsigned long mcc_script_effects_snapshot(void)
{
    if (!mcc_effects_owned || !mcc_cache_tags_loaded()) return 0;
    return game_connection()==_game_connection_network_client ? mcc_effects_remote : mcc_effects_native();
}

void mcc_script_effects_restore(unsigned long flags)
{
    if (!mcc_effects_owned || !mcc_cache_tags_loaded() || (flags&~MCC_EFFECT_MASK)) return;
    mcc_effects_remote=flags;
    if (game_connection()!=_game_connection_network_client) mcc_effects_set_native(flags);
}

boolean mcc_script_effect_enabled(short effect)
{
    return effect>=0 && effect<MCC_EFFECT_COUNT &&
        (mcc_script_effects_snapshot()&(1u<<effect))!=0;
}

boolean mcc_script_effects_host_movement(void)
{
    /* Jetpack acceleration/braking and super-jump do not fit stock movement
     * bounds. Simulate on the host from validated input instead of relaxing
     * those bounds to accept arbitrary client positions or velocities. */
    return mcc_script_effect_enabled(MCC_EFFECT_JETPACK) ||
        mcc_script_effect_enabled(MCC_EFFECT_SUPER_JUMP);
}

boolean mcc_script_effects_preserve_body(void)
{
    /* MCC possession can change a player's unit without killing the old one.
     * Host object/damage replication, not the attachment change, owns its fate. */
    return mcc_cache_tags_loaded();
}

boolean mcc_script_effects_bump(long source, long target)
{
    struct biped_datum *from, *to;
    struct player_datum *player;
    void network_player_detach_unit(long index);
    void network_player_attach_unit(long index,long unit);
    long owner;
    if (!mcc_cache_tags_loaded()) return FALSE;
    /* Consume the MCC branch even on clients: they must never use the local
     * possession helper, including with a modified native cheat flag. */
    if (!mcc_script_effect_enabled(MCC_EFFECT_POSSESSION) ||
        game_connection()==_game_connection_network_client) return TRUE;
    from=(struct biped_datum *)object_try_and_get_and_verify_type(source,_object_mask_biped);
    to=(struct biped_datum *)object_try_and_get_and_verify_type(target,_object_mask_biped);
    if (!from || !to || source==target || from->unit.player_index==NONE ||
        to->unit.player_index!=NONE || from->object.parent_object_index!=NONE ||
        to->object.parent_object_index!=NONE ||
        TEST_FLAG(from->object.damage_flags,_object_dead_bit) ||
        TEST_FLAG(to->object.damage_flags,_object_dead_bit)) return TRUE;
    owner=from->unit.player_index;
    player=player_try_and_get(owner);
    if (!player || player->unit_index!=source) return TRUE;
    to->biped.bump_ticks=-15;
    if (game_connection()==_game_connection_local) {
        if (player->local_player_index!=NONE)
            players_set_local_player_unit(player->local_player_index,target);
    } else if (game_connection()==_game_connection_network_server) {
        network_player_detach_unit(owner);
        /* Attachment normally accepts a respawn's team. Possession must not
         * let an enemy's body change the player's multiplayer team. */
        to->object.owner_team_index=(short)player->team_index;
        network_player_attach_unit(owner,target);
        player->dead_unit_index=NONE;
    }
    return TRUE;
}
