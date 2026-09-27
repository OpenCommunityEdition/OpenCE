# C02: nav-point trio into the genuine owner header interface/hud.h (applied on top of npA).
NL = '\n'
T = '\t'
HUD_H_NAV = NL.join([
    '/* ---------- prototypes/HUD_NAV_POINTS.C */',
    '',
    'short find_nav_point(',
    T + 'char const *name);',
    'short hud_get_nav_point_render_type(',
    T + 'short local_player_index,',
    T + 'union real_point3d const *head,',
    T + 'union real_point3d const *position,',
    T + 'long reference_object_index);',
    'void custom_render_nav_point(',
    T + 'short local_player_index,',
    T + 'union real_point3d const *position,',
    T + 'short nav_index,',
    T + 'short waypoint_type);',
    '',
    '',
])


def edit(sub, rm, write, read):
    sub('source/game/game_engine.c',
        '#include "interface/hud_messaging.h"\n#include "interface/hud_nav_points.h"\n',
        '#include "interface/hud.h"\n#include "interface/hud_messaging.h"\n')
    sub('source/interface/hud.h', 'union real_argb_color;\n', 'union real_argb_color;\nunion real_point3d;\n')
    sub('source/interface/hud.h', '/* ---------- prototypes/HUD_SOUNDS.C */\n',
        HUD_H_NAV + '/* ---------- prototypes/HUD_SOUNDS.C */\n')
    sub('source/interface/hud_nav_points.c', '#include "interface/hud_nav_points.h"\n', '')
    rm('source/interface/hud_nav_points.h')
