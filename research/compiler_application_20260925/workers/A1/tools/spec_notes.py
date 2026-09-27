# Honest labels (rule 15) for the two descriptive .bss statics; comments only (byte-inert, C1-neutral)
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'static long game_engine_teleport_message_ticks = 0;\n',
        '/* descriptive names: January\'s statics carry no recoverable names (HCEX: blocked_message_delay and\n'
        '   fade_function, static locals of game_engine_update_teleporter) */\n'
        'static long game_engine_teleport_message_ticks = 0;\n')
