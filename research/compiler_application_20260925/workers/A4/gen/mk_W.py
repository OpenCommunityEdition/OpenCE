"""Apply the W (global_window_parameters owner declaration) patch to an alternate source root, in place.
    python mk_W.py <root>        (root contains source/...)
Every edit is an exact-match replacement with an asserted count; CRLF is preserved."""
import glob
import re
import sys

ROOT = sys.argv[1].replace(chr(92), '/').rstrip('/') + '/source/'


def load(rel):
    b = open(ROOT + rel, 'rb').read().decode('latin-1')
    assert '\r\n' in b
    return b.replace('\r\n', '\n')


def save(rel, s):
    open(ROOT + rel, 'wb').write(s.replace('\n', '\r\n').encode('latin-1'))


def rep(s, old, new, count=1):
    n = s.count(old)
    assert n == count, (n, count, old[:90])
    return s.replace(old, new)


def drop_struct(s, name):
    m = list(re.finditer(r'\nstruct ' + re.escape(name) + r'\n\{\n.*?\n\};\n\n', s, re.S))
    assert len(m) == 1, (name, len(m))
    return s[:m[0].start()] + '\n' + s[m[0].end():]


def drop_typedef(s, name):
    m = list(re.finditer(r'\ntypedef char ' + re.escape(name) + r'\[\n.*?\];\n', s, re.S))
    assert len(m) == 1, (name, len(m))
    return s[:m[0].start()] + '\n' + s[m[0].end():]


GENUINE_EXTERN = 'extern struct rasterizer_window_begin_parameters global_window_parameters;\n'


def drop_extern(s, typ):
    if OWNER == 'none':
        # W0: keep a consumer-local extern, but always of the genuine type
        if typ == 'rasterizer_window_begin_parameters':
            return s
        return rep(s, 'extern struct %s global_window_parameters;\n' % typ, GENUINE_EXTERN)
    return rep(s, 'extern struct %s global_window_parameters;\n' % typ, '')


def add_include(s):
    return rep(s, '#include <xtl.h>\n', '#include "rasterizer/rasterizer.h"\n#include <xtl.h>\n')


G = 'global_window_parameters.'
VB = G + 'camera.viewport_bounds.'

# ---- owner declaration
#   'rasterizer': rasterizer.h globals block, beside the sibling pooled frame-parameters global (type's home)
#   'internal'  : rasterizer_xbox_internal.h (backend interface of rasterizer_xbox.c, the global's sole writer)
OWNER = sys.argv[2] if len(sys.argv) > 2 else 'rasterizer'
if OWNER == 'rasterizer':
    h = load('rasterizer/rasterizer.h')
    h = rep(h, 'extern struct rasterizer_frame_begin_parameters global_frame_parameters;\n',
            'extern struct rasterizer_frame_begin_parameters global_frame_parameters;\n'
            'extern struct rasterizer_window_begin_parameters global_window_parameters;\n')
    save('rasterizer/rasterizer.h', h)
elif OWNER == 'none':
    pass    # W0: no owner declaration; every consumer keeps a local extern, but of the genuine type
elif OWNER == 'internal':
    h = load('rasterizer/xbox/rasterizer_xbox_internal.h')
    h = rep(h, '\n#endif /* __RASTERIZER_XBOX_INTERNAL_H */\n',
            '\n/* ---------- globals */\n\nextern struct rasterizer_window_begin_parameters global_window_parameters;\n'
            '\n#endif /* __RASTERIZER_XBOX_INTERNAL_H */\n')
    save('rasterizer/xbox/rasterizer_xbox_internal.h', h)
else:
    sys.exit('unknown owner placement')

# ---- consumers already using the genuine type: drop the local extern only
for rel in ('rasterizer/rasterizer_text.c', 'rasterizer/rasterizer_transparent_geometry.c',
            'rasterizer/xbox/rasterizer_xbox.c', 'rasterizer/xbox/rasterizer_xbox_active_camouflage.c',
            'rasterizer/xbox/rasterizer_xbox_draw_primitives.c', 'rasterizer/xbox/rasterizer_xbox_dynavobgeom.c',
            'rasterizer/xbox/rasterizer_xbox_environment.c', 'rasterizer/xbox/rasterizer_xbox_environment_fog.c',
            'rasterizer/xbox/rasterizer_xbox_models.c', 'rasterizer/xbox/rasterizer_xbox_profile.c',
            'rasterizer/xbox/rasterizer_xbox_screen_effect.c', 'rasterizer/xbox/rasterizer_xbox_transparent_geometry.c',
            'rasterizer/xbox/rasterizer_xbox_water.c'):
    s = load(rel)
    s = drop_extern(s, 'rasterizer_window_begin_parameters')
    save(rel, s)

# ---- rasterizer.c
rel = 'rasterizer/rasterizer.c'
s = load(rel)
s = drop_struct(s, 'rasterizer_window_parameters')
s = drop_extern(s, 'rasterizer_window_parameters')
s = rep(s, G + 'camera_forward', G + 'camera.forward', 4)
s = rep(s, G + 'camera_position', G + 'camera.position', 2)
save(rel, s)

# ---- rasterizer_debug.c
rel = 'rasterizer/rasterizer_debug.c'
s = load(rel)
s = drop_struct(s, 'rasterizer_debug_window_parameters_prefix')
s = drop_extern(s, 'rasterizer_debug_window_parameters_prefix')
print(rel, 'camera_position', s.count(G + 'camera_position'), 'camera_forward', s.count(G + 'camera_forward'))
s = s.replace(G + 'camera_position', G + 'camera.position').replace(G + 'camera_forward', G + 'camera.forward')
save(rel, s)

# ---- rasterizer_lights.c
rel = 'rasterizer/rasterizer_lights.c'
s = load(rel)
s = drop_struct(s, 'rasterizer_lights_window_parameters')
s = drop_typedef(s, 'verify_rasterizer_lights_window_parameters_camera_forward_offset')
s = drop_extern(s, 'rasterizer_lights_window_parameters')
print(rel, 'camera_position', s.count(G + 'camera_position'), 'camera_forward', s.count(G + 'camera_forward'),
      'view_to_world', s.count(G + 'view_to_world'))
s = s.replace(G + 'camera_position', G + 'camera.position').replace(G + 'camera_forward', G + 'camera.forward')
s = s.replace(G + 'view_to_world', G + 'frustum.view_to_world')
save(rel, s)

# ---- xbox_debug / text / motion_sensor: the four shorts at +0x34 are camera.viewport_bounds {y0, x0, y1, x1};
#      the local that receives x1 - x0 feeds the x scale, so the two locals are renamed to what they hold
#      (declaration positions unchanged).
rel = 'rasterizer/xbox/rasterizer_xbox_debug.c'
s = load(rel)
s = drop_struct(s, 'rasterizer_xbox_debug_window_parameters_prefix')
s = drop_extern(s, 'rasterizer_xbox_debug_window_parameters_prefix')
s = add_include(s)
s = rep(s, '\tshort window_width;\n\tshort window_height;\n', '\tshort window_height;\n\tshort window_width;\n')
s = rep(s, '\twindow_height = global_window_parameters.bottom -\n\t\tglobal_window_parameters.top;\n'
           '\twindow_width = global_window_parameters.right -\n\t\tglobal_window_parameters.left;\n',
        '\twindow_width = ' + VB + 'x1 -\n\t\t' + VB + 'x0;\n'
        '\twindow_height = ' + VB + 'y1 -\n\t\t' + VB + 'y0;\n')
s = rep(s, '= 2.0f / window_height;', '= 2.0f / window_width;@@')
s = rep(s, '-1.0f - 1.0f / window_height;', '-1.0f - 1.0f / window_width;@@')
s = rep(s, '= -2.0f / window_width;', '= -2.0f / window_height;@@')
s = rep(s, '1.0f / window_width + 1.0f;', '1.0f / window_height + 1.0f;@@')
s = s.replace('@@', '')
save(rel, s)

rel = 'rasterizer/xbox/rasterizer_xbox_text.c'
s = load(rel)
s = drop_struct(s, 'rasterizer_text_window_parameters')
s = drop_extern(s, 'rasterizer_text_window_parameters')
s = add_include(s)
s = rep(s, '\tshort window_width;\n\tshort window_height;\n', '\tshort window_height;\n\tshort window_width;\n')
s = rep(s, '\t\twindow_height = global_window_parameters.bottom -\n\t\t\tglobal_window_parameters.top;\n'
           '\t\twindow_width = global_window_parameters.right -\n\t\t\tglobal_window_parameters.left;\n',
        '\t\twindow_width = ' + VB + 'x1 -\n\t\t\t' + VB + 'x0;\n'
        '\t\twindow_height = ' + VB + 'y1 -\n\t\t\t' + VB + 'y0;\n')
s = rep(s, 'parameters->scale->i / window_height;', 'parameters->scale->i / window_width;@@')
s = rep(s, 'parameters->scale->j / window_width;', 'parameters->scale->j / window_height;@@')
s = rep(s, '= 2.0f / window_height;', '= 2.0f / window_width;@@')
s = rep(s, '(1.0f + 1.0f / window_height);', '(1.0f + 1.0f / window_width);@@')
s = rep(s, '= -2.0f / window_width;', '= -2.0f / window_height;@@')
s = rep(s, '1.0f / window_width + 1.0f;', '1.0f / window_height + 1.0f;@@')
s = s.replace('@@', '')
save(rel, s)

rel = 'rasterizer/xbox/rasterizer_xbox_motion_sensor.c'
s = load(rel)
s = drop_struct(s, 'motion_sensor_window_parameters')
s = drop_extern(s, 'motion_sensor_window_parameters')
s = add_include(s)
s = rep(s, '\tshort width;\n\tshort height;\n', '\tshort height;\n\tshort width;\n')
s = rep(s, '\t\t\theight = global_window_parameters.bottom - global_window_parameters.top;\n'
           '\t\t\twidth = global_window_parameters.right - global_window_parameters.left;\n',
        '\t\t\twidth = ' + VB + 'x1 - ' + VB + 'x0;\n'
        '\t\t\theight = ' + VB + 'y1 - ' + VB + 'y0;\n')
s = rep(s, '= 2.0f / height;', '= 2.0f / width;@@')
s = rep(s, '-1.0f - 1.0f / height;', '-1.0f - 1.0f / width;@@')
s = rep(s, '= -2.0f / width;', '= -2.0f / height;@@')
s = rep(s, '1.0f + 1.0f / width;', '1.0f + 1.0f / height;@@')
s = s.replace('@@', '')
save(rel, s)

# ---- decals
rel = 'rasterizer/xbox/rasterizer_xbox_decals.c'
s = load(rel)
s = drop_struct(s, 'rasterizer_decals_window_parameters')
s = drop_extern(s, 'rasterizer_decals_window_parameters')
s = rep(s, G + 'atmospheric_fog_maximum_density', G + 'fog.atmospheric_maximum_density')
save(rel, s)

# ---- xbox_lights / widgets: camera, viewport and frustum views
for rel, typ, tdef in (('rasterizer/xbox/rasterizer_xbox_lights.c', 'rasterizer_lights_window_parameters', None),
                       ('rasterizer/xbox/rasterizer_xbox_widgets.c', 'rasterizer_widget_window_parameters',
                        'rasterizer_widget_window_parameters_offset_assert')):
    s = load(rel)
    s = drop_struct(s, typ)
    if tdef:
        s = drop_typedef(s, tdef)
    s = drop_extern(s, typ)
    if 'lights' in rel:
        s = add_include(s)
    counts = {}
    for a, b in (('camera_position', 'camera.position'), ('camera_forward', 'camera.forward'),
                 ('viewport_bounds', 'camera.viewport_bounds'), ('world_to_view', 'frustum.world_to_view'),
                 ('projection_matrix', 'frustum.projection_matrix')):
        counts[a] = s.count(G + a)
        s = s.replace(G + a, G + b)
    print(rel, counts)
    save(rel, s)

# ---- shadows: window prefix view, renamed copy of rasterizer_frame_begin_parameters, duplicate profile enumerator
rel = 'rasterizer/xbox/rasterizer_xbox_shadows.c'
s = load(rel)
s = drop_struct(s, 'rasterizer_shadows_window_parameters_prefix')
s = drop_extern(s, 'rasterizer_shadows_window_parameters_prefix')
s = drop_struct(s, 'rasterizer_shadows_frame_parameters')
s = rep(s, 'extern struct rasterizer_shadows_frame_parameters global_frame_parameters;\n', '')
s = rep(s, 'enum\n{\n\t_rasterizer_profile_environment_shadows = 4,\n};\n\n', '')
s = add_include(s)
save(rel, s)

# ---- internal owner: every consumer must see rasterizer_xbox_internal.h
if OWNER == 'internal':
    consumers = ['rasterizer/rasterizer.c', 'rasterizer/rasterizer_debug.c', 'rasterizer/rasterizer_lights.c',
                 'rasterizer/rasterizer_text.c', 'rasterizer/rasterizer_transparent_geometry.c']
    consumers += ['rasterizer/xbox/rasterizer_xbox%s.c' % x for x in (
        '', '_active_camouflage', '_debug', '_decals', '_draw_primitives', '_dynavobgeom', '_environment',
        '_environment_fog', '_lights', '_models', '_motion_sensor', '_profile', '_screen_effect', '_shadows', '_text',
        '_transparent_geometry', '_water', '_widgets')]
    for rel in consumers:
        s = load(rel)
        if 'rasterizer_xbox_internal.h' in s:
            continue
        if '#include <xtl.h>\n' in s:
            s = rep(s, '#include <xtl.h>\n', '#include <xtl.h>\n#include "rasterizer/xbox/rasterizer_xbox_internal.h"\n')
        else:
            last = list(re.finditer(r'^#include [^\n]*\n', s, re.M))[-1]
            s = s[:last.end()] + '#include "rasterizer/xbox/rasterizer_xbox_internal.h"\n' + s[last.end():]
        save(rel, s)
        print('added internal include:', rel)

# ---- sanity: no view type names or local externs remain
left = []
views = ('rasterizer_window_parameters', 'rasterizer_debug_window_parameters_prefix',
         'rasterizer_lights_window_parameters', 'rasterizer_xbox_debug_window_parameters_prefix',
         'rasterizer_decals_window_parameters', 'motion_sensor_window_parameters',
         'rasterizer_shadows_window_parameters_prefix', 'rasterizer_text_window_parameters',
         'rasterizer_widget_window_parameters', 'rasterizer_shadows_frame_parameters')
for p in glob.glob(ROOT + 'rasterizer/**/*.c', recursive=True):
    t = open(p, 'rb').read().decode('latin-1')
    for v in views:
        if re.search(r'\b' + v + r'\b', t):
            left.append((p, v))
    for m in re.finditer(r'^extern[^;]*global_window_parameters;', t, re.M):
        if OWNER != 'none' or m.group(0) + '\n' != GENUINE_EXTERN:
            left.append((p, m.group(0)))
print('leftovers:', left)
