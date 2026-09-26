# R1 (rule 14) + R2 (rule 13) on game_engine.c
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'if (killing_player->statistics.multiple_kills >=\n\t\t\t_game_engine_message_killed_by_player)\n',
        'if (killing_player->statistics.multiple_kills >= 4)\n')
    sub(G, '\t\t/* Engine type 5 is assigned by the four race variant builders below. */\n', '')
    sub(G, '\t\t\t\t/* The target counts netgame flag type 4 for race games. */\n\t\t\t\tif (flag->type == 4)\n',
        '\t\t\t\tif (flag->type == _netgame_flag_race_vehicle)\n')
