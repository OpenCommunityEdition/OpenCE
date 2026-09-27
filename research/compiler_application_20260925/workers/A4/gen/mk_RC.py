"""R-C (OWNER-GATED, fifty-objects owner-queue item 2): replace the invented rasterizer_xbox_d3d_globals aggregate
and its 23 #define redirects with 23 '= 0' file statics, in place, on an R-A(+R-B) tree.
    python mk_RC.py <root>
Also writes gen/sym_RC.json (R-A rows + the aggregate row -> 23 static rows)."""
import json
import sys

root = sys.argv[1].replace(chr(92), '/').rstrip('/')
p = root + '/source/rasterizer/xbox/rasterizer_xbox.c'
s = open(p, 'rb').read().decode('latin-1')
assert '\r\n' in s
s = s.replace('\r\n', '\n')


def rep(s, old, new, count=1):
    n = s.count(old)
    assert n == count, (n, old[:80])
    return s.replace(old, new)


LAYOUT = [  # name, byte offset in .bss 0x45E028 (image file offset 4579368 + off)
    ('node_matrix_constants', 0x000), ('bitmap_dimensions_non_blocking', 0x840), ('bitmap_dimensions', 0x844),
    ('d3d', 0x848), ('global_d3d_texture_render_primary', 0x84c), ('global_d3d_surface_render_primary', 0x854),
    ('global_d3d_surface_render_primary_z', 0x858), ('global_d3d_texture_render_secondary', 0x85c),
    ('global_d3d_texture_render_secondary_z', 0x860), ('global_d3d_surface_render_secondary', 0x864),
    ('global_d3d_surface_render_secondary_z', 0x868), ('global_d3d_texture_shadow_primary', 0x86c),
    ('global_d3d_surface_shadow_primary', 0x870), ('global_d3d_texture_shadow_secondary', 0x874),
    ('global_d3d_surface_shadow_secondary', 0x878), ('global_d3d_texture_sun_glow_primary', 0x87c),
    ('global_d3d_surface_sun_glow_primary', 0x880), ('global_d3d_texture_sun_glow_secondary', 0x884),
    ('global_d3d_surface_sun_glow_secondary', 0x888), ('global_d3d_texture_water', 0x88c),
    ('global_d3d_surface_water', 0x890), ('global_d3d_texture_render_primary_copy', 0x8a0),
    ('global_d3d_surface_render_primary_copy', 0x8a4)]

# symbol-listing comment
s = rep(s, '\t_rasterizer_xbox_d3d_globals (0000)\n',
        ''.join('\t_%s (%04x)\n' % (n, o) for n, o in LAYOUT))

# the aggregate, its instance and its 23 redirects
start = s.index('/* January keeps this object\'s private Direct3D state in one contiguous owner.\n')
end = s.index('#define global_d3d_surface_render_primary_copy rasterizer_xbox_d3d_globals.global_d3d_surface_render_primary_copy\n')
end = end + len('#define global_d3d_surface_render_primary_copy rasterizer_xbox_d3d_globals.global_d3d_surface_render_primary_copy\n')
old = s[start:end]
assert old.count('#define ') == 23 and 'struct rasterizer_xbox_d3d_globals rasterizer_xbox_d3d_globals = { 0 };' in old
new = (
    "/* this object's private Direct3D state, as separate file statics: 14 of these\n"
    " * names appear bare in January's IDirect3D*() error strings (no aggregate is\n"
    " * named anywhere), and cachebeta.pdb has no public in 0x0045E028..0x0045E8CF.\n"
    " * node_matrix_constants, bitmap_dimensions*, and the render_primary,\n"
    " * render_secondary_z and render_primary_copy slots have no first-party name\n"
    " * (descriptive).  The '= 0' initialisers keep January's declaration-order\n"
    " * .bss layout. */\n"
    "static real node_matrix_constants[RASTERIZER_MAXIMUM_NODES_PER_MODEL][3][4] = { 0 };\n"
    "static point2d bitmap_dimensions_non_blocking = { 0 };\n"
    "static point2d bitmap_dimensions = { 0 };\n"
    "static Direct3D *d3d = NULL;\n"
    "/* hand-built descriptors wrapping the two back buffers */\n"
    "static D3DBaseTexture *global_d3d_texture_render_primary[2] = { 0 };\n"
    "static D3DSurface *global_d3d_surface_render_primary = NULL;\n"
    "static D3DSurface *global_d3d_surface_render_primary_z = NULL;\n"
    "static D3DTexture *global_d3d_texture_render_secondary = NULL;\n"
    "/* the render-secondary target has no depth buffer; both slots are\n"
    " * explicitly cleared at initialization.  The texture name is inferred\n"
    " * from the surface name immediately behind it. */\n"
    "static D3DTexture *global_d3d_texture_render_secondary_z = NULL;\n"
    "static D3DSurface *global_d3d_surface_render_secondary = NULL;\n"
    "static D3DSurface *global_d3d_surface_render_secondary_z = NULL;\n"
    "static D3DTexture *global_d3d_texture_shadow_primary = NULL;\n"
    "static D3DSurface *global_d3d_surface_shadow_primary = NULL;\n"
    "static D3DTexture *global_d3d_texture_shadow_secondary = NULL;\n"
    "static D3DSurface *global_d3d_surface_shadow_secondary = NULL;\n"
    "static D3DTexture *global_d3d_texture_sun_glow_primary = NULL;\n"
    "static D3DSurface *global_d3d_surface_sun_glow_primary = NULL;\n"
    "static D3DTexture *global_d3d_texture_sun_glow_secondary = NULL;\n"
    "static D3DSurface *global_d3d_surface_sun_glow_secondary = NULL;\n"
    "static D3DTexture *global_d3d_texture_water = NULL;\n"
    "static D3DSurface *global_d3d_surface_water[RASTERIZER_TARGET_WATER_MAX_MIPMAP_LEVELS] = { 0 };\n"
    "static D3DBaseTexture *global_d3d_texture_render_primary_copy = NULL;\n"
    "static D3DSurface *global_d3d_surface_render_primary_copy = NULL;\n")
s = s[:start] + new + s[end:]
assert 'rasterizer_xbox_d3d_globals' not in s
open(p, 'wb').write(s.replace('\n', '\r\n').encode('latin-1'))

# symbols.json spec: R-A rows + the aggregate row -> 23 static rows
WT = 'C:/halo-worktrees/claude-compiler-application-20260925'
spec = json.load(open(WT + '/research/compiler_application_20260925/workers/A4/gen/sym_RA.json'))
lines = open(WT + '/config/symbols.json', 'rb').read().decode('utf-8').split('\r\n')
l = lines[23141 - 1]
assert l == '{ "file_offset": 4579368, "flags": 0, "name": "_rasterizer_xbox_d3d_globals" },', l
spec.append({'line': 23141, 'find': l, 'replace': [
    '{ "file_offset": %d, "flags": 0, "name": "_%s", "static": true },' % (4579368 + o, n) for n, o in LAYOUT]})
assert lines[23142 - 1].startswith('{ "file_offset": 4581584,'), lines[23142 - 1]
json.dump(spec, open(WT + '/research/compiler_application_20260925/workers/A4/gen/sym_RC.json', 'w'), indent=1)
print('R-C applied; sym_RC.json rows:', len(spec))
