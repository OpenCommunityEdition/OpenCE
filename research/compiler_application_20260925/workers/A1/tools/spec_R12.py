# R12: boolean variant members get FALSE/TRUE (byte-identical macros)
import re
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    t = read(G)
    members = ['moving_hill', 'no_death_bonus', 'no_kill_penalty', 'kill_in_order', 'assault', 'reset_on_capture',
               'flag_must_reset', 'flag_at_home_to_score', 'random_start', 'ball_spawn_delay']
    n = 0
    for m in members:
        pat = re.compile(r'(game_engine_variant\.[a-z_]+\.' + m + r' = )([01]);')
        n += len(pat.findall(t))
        t = pat.sub(lambda mm: mm.group(1) + ('TRUE' if mm.group(2) == '1' else 'FALSE') + ';', t)
    print('R12 replaced', n)
    write(G, t)
