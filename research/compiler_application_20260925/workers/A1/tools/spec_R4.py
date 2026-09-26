# R4: HCEX enum goal_radar (TU-local, top constants block) + the 29 goal_radar sites
import re
def edit(sub, rm, write, read):
    G = 'source/game/game_engine.c'
    sub(G, 'enum game_engine_vehicles\n{\n', 'enum goal_radar\n{\n\t_radar_motion_tracker = 0,\n\t_radar_nav_point,\n\t_radar_none,\n};\n\nenum game_engine_vehicles\n{\n')
    t = read(G)
    names = {'0': '_radar_motion_tracker', '1': '_radar_nav_point', '2': '_radar_none'}
    n_cmp = len(re.findall(r'goal_radar == [012]\b', t))
    n_set = len(re.findall(r'goal_radar = [012];', t))
    assert n_cmp == 3 and n_set == 26, (n_cmp, n_set)
    t = re.sub(r'goal_radar == ([012])\b', lambda m: 'goal_radar == ' + names[m.group(1)], t)
    t = re.sub(r'goal_radar = ([012]);', lambda m: 'goal_radar = ' + names[m.group(1)] + ';', t)
    write(G, t)
