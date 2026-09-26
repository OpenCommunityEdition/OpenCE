# R11: conventional for-loop initialiser in find_netgame_flags
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, '\tscenario = global_scenario_get();\n\tflag_index = 0;\n\n\tfor (; flag_index < scenario->netgame_flags.count; flag_index++)\n',
        '\tscenario = global_scenario_get();\n\n\tfor (flag_index = 0; flag_index < scenario->netgame_flags.count; flag_index++)\n')
