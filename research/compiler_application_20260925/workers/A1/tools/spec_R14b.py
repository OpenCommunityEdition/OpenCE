# R14b: update_player_no_shield's scalar-only block -> function-scope player
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'static void game_engine_update_player_no_shield(\n\tlong player_index)\n{\n'
        '\tif (game_engine_has_shield(player_index))\n\t\treturn;\n\n\t{\n\t\tstruct player_datum *player;\n\n'
        '\t\tplayer = player_get(player_index);\n\t\tif (player->unit_index != NONE)\n\t\t{\n'
        '\t\t\tstruct unit_datum *unit = unit_get(player->unit_index);\n\n'
        '\t\t\tunit->object.shield_vitality = 0.0f;\n\t\t\tunit->object.maximum_shield_vitality = 0.0f;\n\t\t}\n\t}\n\n\treturn;\n}\n',
        'static void game_engine_update_player_no_shield(\n\tlong player_index)\n{\n\tstruct player_datum *player;\n\n'
        '\tif (game_engine_has_shield(player_index))\n\t\treturn;\n\n'
        '\tplayer = player_get(player_index);\n\tif (player->unit_index != NONE)\n\t{\n'
        '\t\tstruct unit_datum *unit = unit_get(player->unit_index);\n\n'
        '\t\tunit->object.shield_vitality = 0.0f;\n\t\tunit->object.maximum_shield_vitality = 0.0f;\n\t}\n\n\treturn;\n}\n')
