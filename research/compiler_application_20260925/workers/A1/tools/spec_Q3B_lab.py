# LAB (C07): the two .bss teleporter values as HCEX static locals of update_teleporter
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'static long game_engine_teleport_message_ticks = 0;\nstatic short game_engine_teleport_flash_fade_function = 0;\n', '')
    sub(G, '\tstatic short screen_flash_type = 6;\n', '\tstatic long blocked_message_delay = 0;\n\tstatic short fade_function = 0;\n\tstatic short screen_flash_type = 6;\n')
    sub(G, 'game_engine_teleport_message_ticks', 'blocked_message_delay', 3)
    sub(G, 'game_engine_teleport_flash_fade_function', 'fade_function', 1)
