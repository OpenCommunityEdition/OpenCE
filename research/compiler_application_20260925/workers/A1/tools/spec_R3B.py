# R3B: HCEX multiplayer_player_info member names in players.h (flat), placeholder union removed; game_engine.c uses
def edit(sub, rm, write, read):
    P = 'source/game/players.h'
    sub(P, '\tlong unknown70;\n', '\tlong teleporter_index;\n')
    sub(P, '\tlong unknown7c;\n\tunion\n\t{\n\t\tbyte unknown80[4];\n\t\tlong target_hold_time;\n\t};\n',
        '\tlong player_display_index;\n\tlong player_display_count;\n')
    sub(P, 'typedef char player_datum_target_hold_time_offset_assert[\n\toffsetof(struct player_datum, target_hold_time) == 0x80 ? 1 : -1];\n',
        'typedef char player_datum_player_display_count_offset_assert[\n\toffsetof(struct player_datum, player_display_count) == 0x80 ? 1 : -1];\n')
    G = 'source/game/game_engine.c'
    t = read(G)
    assert t.count('unknown70') == 6 and t.count('unknown7c') == 6 and t.count('target_hold_time') == 6
    sub(G, 'unknown70', 'teleporter_index', 6)
    sub(G, 'unknown7c', 'player_display_index', 6)
    sub(G, 'target_hold_time', 'player_display_count', 6)
