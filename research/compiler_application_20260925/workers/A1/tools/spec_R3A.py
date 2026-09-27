# R3A: count-neutral HCEX renames inside the kept union (fallback if R3B moves any players.h consumer)
def edit(sub, rm, write, read):
    P = 'source/game/players.h'
    sub(P, '\tlong unknown70;\n', '\tlong teleporter_index;\n')
    sub(P, '\tlong unknown7c;\n', '\tlong player_display_index;\n')
    sub(P, '\t\tlong target_hold_time;\n', '\t\tlong player_display_count;\n')
    sub(P, 'typedef char player_datum_target_hold_time_offset_assert[\n\toffsetof(struct player_datum, target_hold_time) == 0x80 ? 1 : -1];\n',
        'typedef char player_datum_player_display_count_offset_assert[\n\toffsetof(struct player_datum, player_display_count) == 0x80 ? 1 : -1];\n')
    G = 'source/game/game_engine.c'
    sub(G, 'unknown70', 'teleporter_index', 6)
    sub(G, 'unknown7c', 'player_display_index', 6)
    sub(G, 'target_hold_time', 'player_display_count', 6)
