# R9: HCEX callback signature; no function-pointer cast at the call
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'static boolean find_closest_player_callback(\n\tlong object_index,\n\tlong const *excluded_player_index);\n',
        'static boolean find_closest_player_callback(\n\tlong object_index,\n\tvoid *custom_data);\n')
    sub(G, 'static boolean find_closest_player_callback(\n\tlong object_index,\n\tlong const *excluded_player_index)\n{\n'
        '\tlong excluded_player = *excluded_player_index;\n',
        'static boolean find_closest_player_callback(\n\tlong object_index,\n\tvoid *custom_data)\n{\n'
        '\tlong excluded_player = *(long *)custom_data;\n')
    sub(G, '\t\t\t(boolean (*)(long, void *))find_closest_player_callback,\n', '\t\t\tfind_closest_player_callback,\n')
