# R8: drop the 14 unused TU prototypes of build_game_variant_* (game_engine_playlist.h declares all 26 builders)
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    names = ['race', 'team_race', 'rally', 'slayer', 'team_slayer', 'elimination', 'stalker', 'team_oddball',
             'accumulation', 'oddball', 'ctf', 'iron_ctf', 'king', 'team_king']
    block = ''.join('struct game_variant *build_game_variant_%s(\n\tstruct game_variant *variant);\n' % n for n in names)
    sub(G, block + '\n/* ---------- globals */\n', '/* ---------- globals */\n')
