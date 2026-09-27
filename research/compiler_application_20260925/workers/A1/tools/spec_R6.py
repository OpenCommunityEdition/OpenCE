# R6: HCEX multiplayer sound names (TU-local enum at the top constants block); equipment bit keeps its own enum
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'enum game_engine_mode\n{\n', 'enum\n{\n\t_multiplayer_sound_game_over = 0x1,\n'
        '\t_multiplayer_sound_double_kill = 0xE,\n\t_multiplayer_sound_triple_kill,\n\t_multiplayer_sound_killtacular_kill,\n'
        '\t_multiplayer_sound_running_riot,\n\t_multiplayer_sound_killing_spree,\n'
        '\t_multiplayer_sound_teleporter_activate = 0x1B,\n\t_multiplayer_sound_countdown_for_respawn = 0x1D,\n'
        '\t_multiplayer_sound_respawn = 0x1F,\n};\n\nenum game_engine_mode\n{\n')
    sub(G, 'enum\n{\n\t_multiplayer_sound_countdown_for_respawn = 0x1D,\n\t_multiplayer_sound_respawn = 0x1F,\n'
        '\t_equipment_created_at_rest_bit = 0,\n};\n', 'enum\n{\n\t_equipment_created_at_rest_bit = 0,\n};\n')
    sub(G, 'game_engine_play_multiplayer_sound(1);', 'game_engine_play_multiplayer_sound(_multiplayer_sound_game_over);')
    sub(G, 'game_engine_play_multiplayer_sound(0x1B);', 'game_engine_play_multiplayer_sound(_multiplayer_sound_teleporter_activate);')
    sub(G, 'game_engine_play_multiplayer_sound(0xE);', 'game_engine_play_multiplayer_sound(_multiplayer_sound_double_kill);', 2)
    sub(G, 'game_engine_play_multiplayer_sound(0xF);', 'game_engine_play_multiplayer_sound(_multiplayer_sound_triple_kill);', 2)
    sub(G, 'game_engine_play_multiplayer_sound(0x10);', 'game_engine_play_multiplayer_sound(_multiplayer_sound_killtacular_kill);', 2)
    sub(G, 'game_engine_play_multiplayer_sound(0x11);', 'game_engine_play_multiplayer_sound(_multiplayer_sound_running_riot);', 2)
    sub(G, 'game_engine_play_multiplayer_sound(0x12);', 'game_engine_play_multiplayer_sound(_multiplayer_sound_killing_spree);', 2)
