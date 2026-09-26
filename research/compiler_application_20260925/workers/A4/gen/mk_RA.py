"""Generate the R-A (non-name-gated) rasterizer_xbox.c and rasterizer_xbox_internal.h from pristine copies.
    python mk_RA.py <pristine.c> <out.c> <pristine.h> <out.h>"""
import sys
def load(p):
    b = open(p, 'rb').read()
    assert b'\r\n' in b
    return b.decode('latin-1').replace('\r\n', '\n')
def save(p, s):
    open(p, 'wb').write(s.replace('\n', '\r\n').encode('latin-1'))
def rep(s, old, new, count=1):
    n = s.count(old)
    assert n == count, (n, old[:80])
    return s.replace(old, new)
c = load(sys.argv[1])
# 1-2: top-of-file symbol listing
c = rep(c, '\t_rasterizer_filthy_bitmap_default_initialize (0000)\n', '\t_rasterizer_filthy_bitmap_defaults_initialize (0000)\n')
c = rep(c, '\t_framebuffer_blend_function_states (0000)\n',
        '\t?srcblend_table@?1??rasterizer_set_framebuffer_blend_function@@9@9 (0000)\n'
        '\t?destblend_table@?1??rasterizer_set_framebuffer_blend_function@@9@9 (0024)\n'
        '\t?blendop_table@?1??rasterizer_set_framebuffer_blend_function@@9@9 (0048)\n')
# 3: invented blend-state enum
c = rep(c, 'enum\n{\n\t_framebuffer_blend_state_source_blend = 0,\n\t_framebuffer_blend_state_destination_blend,\n'
           '\t_framebuffer_blend_state_blend_operation,\n\tNUMBER_OF_FRAMEBUFFER_BLEND_STATES\n};\n\n', '')
# 4: static prototype for the private initializer
c = rep(c, '/* ---------- prototypes */\n\nvoid SetupSmartStates(\n\tvoid);\n',
        '/* ---------- prototypes */\n\nstatic void rasterizer_filthy_bitmap_defaults_initialize(\n\tvoid);\n\nvoid SetupSmartStates(\n\tvoid);\n')
# 5: d3d_palette storage (+ stale csplit remark)
c = rep(c, ' * &d3d_palette) error string names it, and csplit anchors it on\n * _global_d3d_device + 4. */\nD3DPalette *d3d_palette = NULL;\n',
        ' * &d3d_palette) error string names it. */\nstatic D3DPalette *d3d_palette = NULL;\n')
# 6: state cache storage
c = rep(c, '\nstruct rasterizer_hardware_state_cache rasterizer_state_cache =\n', '\nstatic struct rasterizer_hardware_state_cache rasterizer_state_cache =\n')
# 7: remove the file-scope [3][9] table
start = c.index('static const long framebuffer_blend_function_states\n')
end = c.index('};\n\n', start) + len('};\n\n')
old_table = c[start:end]
assert old_table.count('{') == 4 and 'NONE' in old_table
c = c[:start] + c[end:]
# 8: HCEX static-local tables in rasterizer_set_framebuffer_blend_function
tables = ('\t/* three separate NONE-terminated tables (January .rdata is 4-byte aligned, so\n'
          '\t * no single 108-byte array); names, scope and type follow HCEX.pdb\'s static\n'
          '\t * locals of this function (const unsigned long srcblend_table[9], ...). */\n'
          '\tstatic const unsigned long srcblend_table[NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS + 1] =\n\t{\n'
          '\t\tD3DBLEND_SRCALPHA,\n\t\tD3DBLEND_DESTCOLOR,\n\t\tD3DBLEND_DESTCOLOR,\n\t\tD3DBLEND_ONE,\n\t\tD3DBLEND_ONE,\n'
          '\t\tD3DBLEND_ONE,\n\t\tD3DBLEND_ONE,\n\t\tD3DBLEND_ONE,\n\t\tNONE\n\t};\n\n'
          '\tstatic const unsigned long destblend_table[NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS + 1] =\n\t{\n'
          '\t\tD3DBLEND_INVSRCALPHA,\n\t\tD3DBLEND_ZERO,\n\t\tD3DBLEND_SRCCOLOR,\n\t\tD3DBLEND_ONE,\n\t\tD3DBLEND_ONE,\n'
          '\t\tD3DBLEND_ONE,\n\t\tD3DBLEND_ONE,\n\t\tD3DBLEND_INVSRCALPHA,\n\t\tNONE\n\t};\n\n'
          '\tstatic const unsigned long blendop_table[NUMBER_OF_SHADER_FRAMEBUFFER_BLEND_FUNCTIONS + 1] =\n\t{\n'
          '\t\tD3DBLENDOP_ADD,\n\t\tD3DBLENDOP_ADD,\n\t\tD3DBLENDOP_ADD,\n\t\tD3DBLENDOP_ADD,\n\t\tD3DBLENDOP_REVSUBTRACT,\n'
          '\t\tD3DBLENDOP_MIN,\n\t\tD3DBLENDOP_MAX,\n\t\tD3DBLENDOP_ADD,\n\t\tNONE\n\t};\n\n')
c = rep(c, 'void rasterizer_set_framebuffer_blend_function(\n\tshort framebuffer_blend_function)\n{\n',
        'void rasterizer_set_framebuffer_blend_function(\n\tshort framebuffer_blend_function)\n{\n' + tables)
c = rep(c, 'framebuffer_blend_function_states[_framebuffer_blend_state_source_blend][framebuffer_blend_function]', 'srcblend_table[framebuffer_blend_function]')
c = rep(c, 'framebuffer_blend_function_states[_framebuffer_blend_state_destination_blend][framebuffer_blend_function]', 'destblend_table[framebuffer_blend_function]')
c = rep(c, 'framebuffer_blend_function_states[_framebuffer_blend_state_blend_operation][framebuffer_blend_function]', 'blendop_table[framebuffer_blend_function]')
# 9: direct flicker/soft-display calls
c = rep(c, 'IDirect3DDevice8_SetFlickerFilter(global_d3d_device, RASTERIZER_FLICKER_FILTER_LEVEL);', 'D3DDevice_SetFlickerFilter(RASTERIZER_FLICKER_FILTER_LEVEL);')
c = rep(c, 'IDirect3DDevice8_SetSoftDisplayFilter(global_d3d_device, FALSE);', 'D3DDevice_SetSoftDisplayFilter(FALSE);')
# 10-11: private initializer rename + storage (assert text stays verbatim)
c = rep(c, '\trasterizer_filthy_bitmap_default_initialize();\n', '\trasterizer_filthy_bitmap_defaults_initialize();\n')
c = rep(c, '\nvoid rasterizer_filthy_bitmap_default_initialize(\n\tvoid)\n{\n', '\nstatic void rasterizer_filthy_bitmap_defaults_initialize(\n\tvoid)\n{\n')
assert 'rasterizer_filthy_bitmap_default_initialize failed' in c
assert 'framebuffer_blend_function_states' not in c and '_framebuffer_blend_state_' not in c
save(sys.argv[2], c)
h = load(sys.argv[3])
h = rep(h, 'void rasterizer_filthy_bitmap_default_initialize(\n\tvoid);\n', '')
save(sys.argv[4], h)
print('ok')
