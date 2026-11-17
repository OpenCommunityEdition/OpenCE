/* MCC additions to the generic damage ledger. Included after the common
 * projectile/reach/throw validators, so their established policy stays the
 * same for all four MCC grenade kinds. Native two-kind paths are untouched. */
static void mcc_network_note_extra_grenade(long unit_index, short type)
{
    struct unit_datum *unit;
    long player;
    short slot = 0, index;
    if (!mcc_grenades_active() || type < 2 || type >= mcc_grenades_type_count() ||
        game_connection() != _game_connection_network_server) return;
    unit = object_try_and_get_and_verify_type(unit_index, _object_mask_unit);
    if (!unit || unit->unit.player_index == NONE) return;
    player = DATUM_INDEX_TO_ABSOLUTE_INDEX(unit->unit.player_index);
    if (player < 0 || player >= MAXIMUM_TRACKED_PLAYERS) return;
    for (index = 0; index < MAXIMUM_GRENADE_THROWS; ++index) {
        if (damage_players[player].throw_times[index] == NONE) { slot = index; break; }
        if (damage_players[player].throw_times[index] < damage_players[player].throw_times[slot]) slot = index;
    }
    damage_players[player].throw_times[slot] = game_time_get();
    damage_players[player].throw_types[slot] = type;
}

static void mcc_network_extra_grenade_damage(short player, long damage_index,
    byte *kinds, struct damage_reach *reach, short *grenade_type, real *grenade_rate)
{
    struct game_globals *globals;
    short type;
    if (!mcc_grenades_active()) return;
    globals = scenario_get_game_globals();
    for (type = 2; type < mcc_grenades_type_count() && type < globals->grenades.count; ++type) {
        struct game_globals_grenade *grenade = TAG_BLOCK_GET_ELEMENT(&globals->grenades, type, struct game_globals_grenade);
        boolean thrown = distributed_grenade_throw(player, type) != NONE;
        if (grenade->projectile.index != NONE && (thrown || distributed_explosion_this_tick(player, type, NULL))) {
            byte extra_kinds;
            struct damage_reach extra_reach;
            real rate = distributed_source_deals(grenade->projectile.index, damage_index, TRUE, &extra_kinds, &extra_reach);
            if (rate > 0.0f) {
                distributed_reach_combine(reach, &extra_reach);
                if (*grenade_type == NONE || thrown) *grenade_type = type;
            }
            *grenade_rate = MAX(*grenade_rate, rate);
            *kinds |= extra_kinds;
        }
    }
}
