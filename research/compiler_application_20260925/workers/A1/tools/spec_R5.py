# R5: HCEX game matching options (TU-local) + game_engine_type constants in match_game_type
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'enum game_engine_mode\n{\n', 'enum\n{\n\t_game_engine_all = 12,\n\t_game_engine_all_non_team,\n\t_game_engine_all_normal,\n};\n\nenum game_engine_mode\n{\n')
    sub(G, '\t\t\tif (entry == 12)\n', '\t\t\tif (entry == _game_engine_all)\n')
    sub(G, '\t\t\telse if (entry == 13)\n', '\t\t\telse if (entry == _game_engine_all_non_team)\n')
    sub(G, '\t\t\t\tresult = result | (game_type != 1);\n', '\t\t\t\tresult = result | (game_type != game_engine_ctf);\n')
    sub(G, '\t\t\telse if (entry == 14)\n', '\t\t\telse if (entry == _game_engine_all_normal)\n')
    sub(G, '\t\t\t\tresult = result | (game_type != 1 && game_type != 5);\n',
        '\t\t\t\tresult = result | (game_type != game_engine_ctf && game_type != game_engine_race);\n')
    sub(G, '\t\t\tresult = result & (game_types[index] == 0);\n', '\t\t\tresult = result & (game_types[index] == game_engine_none);\n')
