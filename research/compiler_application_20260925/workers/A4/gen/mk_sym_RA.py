"""Build the R-A symbols.json edit spec (line-surgical) -> gen/sym_RA.json. Verifies each line's name."""
import json, os
WT = r'C:\halo-worktrees\claude-compiler-application-20260925'
lines = open(os.path.join(WT, 'config', 'symbols.json'), 'rb').read().decode('utf-8').split('\r\n')
static_rows = {5495: '_Direct3D_Release@0', 5496: '_IDirect3D8_Release@4', 5497: '_IDirect3D8_CreateDevice@28',
               5502: '_D3DDevice_SetRenderState', 5503: '_D3DDevice_SetTextureStageState', 5504: '_IDirect3DDevice8_Release@4',
               5505: '_IDirect3DDevice8_GetDeviceCaps@8', 5506: '_IDirect3DDevice8_Present@20', 5507: '_IDirect3DDevice8_GetBackBuffer@16',
               5512: '_IDirect3DDevice8_SetRenderTarget@12', 5517: '_IDirect3DDevice8_Clear@28', 5518: '_IDirect3DDevice8_SetViewport@8',
               5520: '_IDirect3DDevice8_SetRenderState@12', 5525: '_IDirect3DDevice8_SetTextureStageState@16',
               5533: '_IDirect3DDevice8_SetVertexData2s@16', 5534: '_IDirect3DDevice8_Begin@8', 5535: '_IDirect3DDevice8_End@4',
               5568: '_IDirect3DSurface8_Release@4', 5570: '_IDirect3DSurface8_GetDesc@8', 5571: '_IDirect3DSurface8_LockRect@16',
               22725: '_rasterizer_state_cache', 23143: '_d3d_palette'}
spec = []
def mk_static(l):
    assert l.endswith(' },') and '"static"' not in l, l
    return l[:-3] + ', "static": true },'
for ln, name in sorted(static_rows.items()):
    l = lines[ln - 1]
    assert ('"name": "%s"' % name) in l, (ln, l)
    spec.append({'line': ln, 'find': l, 'replace': [mk_static(l)]})
l = lines[5578 - 1]
assert l == '{ "file_offset": 1336912, "flags": 32, "name": "_rasterizer_filthy_bitmap_default_initialize" },', l
spec.append({'line': 5578, 'find': l, 'replace': ['{ "file_offset": 1336912, "flags": 32, "name": "_rasterizer_filthy_bitmap_defaults_initialize", "static": true },']})
l = lines[19677 - 1]
assert l == '{ "file_offset": 2670508, "flags": 0, "name": "_framebuffer_blend_function_states" },', l
spec.append({'line': 19677, 'find': l, 'replace': [
    '{ "file_offset": 2670508, "flags": 0, "name": "?srcblend_table@?1??rasterizer_set_framebuffer_blend_function@@9@9", "static": true },',
    '{ "file_offset": 2670544, "flags": 0, "name": "?destblend_table@?1??rasterizer_set_framebuffer_blend_function@@9@9", "static": true },',
    '{ "file_offset": 2670580, "flags": 0, "name": "?blendop_table@?1??rasterizer_set_framebuffer_blend_function@@9@9", "static": true },']})
# neighbours of the blend rows must stay address-ordered
prev, nxt = lines[19676 - 1], lines[19678 - 1]
print('prev:', prev); print('next:', nxt)
json.dump(spec, open(os.path.join(WT, r'research\compiler_application_20260925\workers\A4\gen\sym_RA.json'), 'w'), indent=1)
print(len(spec), 'edits')
