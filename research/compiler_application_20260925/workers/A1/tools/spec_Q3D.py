# Q3D: debug_player_color -> the genuine short hs global; the teleporter flash values -> update_teleporter statics
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'struct\n{\n\tshort value;\n\tword pad;\n\tshort teleporter_flash_type;\n\tword teleporter_flash_pad;\n'
        '\treal teleporter_flash_maximum_intensity;\n\treal_argb_color teleporter_flash_color;\n'
        '\treal teleporter_flash_duration;\n} debug_player_color =\n{\n\tNONE,\n\t0,\n\t6,\n\t0,\n\t1.0f,\n'
        '\t{ 0.5f, 0.35f, 1.0f, 0.35f },\n\t1.0f\n};\n',
        'short debug_player_color = NONE;\n')
    sub(G, 'static void game_engine_update_teleporter(\n\tlong player_index)\n{\n\tstruct scenario *scenario',
        'static void game_engine_update_teleporter(\n\tlong player_index)\n{\n'
        '\tstatic short screen_flash_type = 6;\n\tstatic real max_intensity = 1.0f;\n\tstatic real alpha = 0.5f;\n'
        '\tstatic real red = 0.35f;\n\tstatic real green = 1.0f;\n\tstatic real blue = 0.35f;\n'
        '\tstatic real duration = 1.0f;\n\tstruct scenario *scenario')
    sub(G, '\t\t\t\tscreen_flash.type =\n\t\t\t\t\tdebug_player_color.teleporter_flash_type;\n'
        '\t\t\t\tscreen_flash.duration =\n\t\t\t\t\tdebug_player_color.teleporter_flash_duration;\n',
        '\t\t\t\tscreen_flash.type = screen_flash_type;\n\t\t\t\tscreen_flash.duration = duration;\n')
    sub(G, '\t\t\t\tscreen_flash.max_intensity =\n\t\t\t\t\tdebug_player_color.teleporter_flash_maximum_intensity;\n',
        '\t\t\t\tscreen_flash.max_intensity = max_intensity;\n')
    sub(G, '\t\t\t\tscreen_flash.screen_flash_color =\n\t\t\t\t\tdebug_player_color.teleporter_flash_color;\n',
        '\t\t\t\tscreen_flash.screen_flash_color.alpha = alpha;\n\t\t\t\tscreen_flash.screen_flash_color.red = red;\n'
        '\t\t\t\tscreen_flash.screen_flash_color.green = green;\n\t\t\t\tscreen_flash.screen_flash_color.blue = blue;\n')
    sub(G, '\t\tif (debug_player_color.value != NONE)\n\t\t\tcolor_index = debug_player_color.value;\n',
        '\t\tif (debug_player_color != NONE)\n\t\t\tcolor_index = debug_player_color;\n')
