#!/usr/bin/env python
"""Generate the shared renderer's CPU-only state functions for native Mac.

No draw entry points or replacement gameplay callbacks are generated. Exact
source slices deliberately fail if upstream boundaries change.
"""
import sys
source = open('port/linux/src/d3d8_gl.c').read()
def between(first, last):
    start = source.index(first)
    return source[start:source.index(last, start)]
parts = [
    between('static void color_to_vec4(', '/* ---------- debugging settings'),
    between('static unsigned long vertex_type_bytes(', 'HRESULT WINAPI D3DDevice_CreateVertexShader('),
    between('static struct vertex_shader_object *vertex_shader_from_handle(', '/* the game names its model lighting programs'),
    between('void WINAPI D3DDevice_SetVertexShader(DWORD', 'void WINAPI D3DDevice_SetVertexShaderConstant('),
    between('void WINAPI D3DDevice_Begin(D3DPRIMITIVETYPE', 'void WINAPI D3DDevice_End(void)'),
    between('static void set_attribute(', '/* ---------- clearing */'),
    between('DWORD D3D__RenderState[D3DRS_MAX];', '/* ---------- vertex shaders */'),
    between('static unsigned long long constant_serials[', 'static void viewport_update_constants('),
    between('static void viewport_update_constants(', 'HRESULT WINAPI Direct3D_CreateDevice('),
    between('void WINAPI D3DDevice_GetBackBuffer(', 'static BOOL trace_frame(void);'),
    between('void WINAPI D3DDevice_SetViewport(', 'void WINAPI D3DDevice_SetTransform('),
    between('void D3DFASTCALL D3DDevice_SetRenderState_Simple(', '/* ---------- vertex shaders */'),
    between('void WINAPI D3DDevice_SetVertexShaderConstant(', '/* the program that runs:'),
    between('void WINAPI D3DDevice_SetStreamSource(', 'void WINAPI D3DDevice_SetIndices('),
    between('void WINAPI D3DDevice_InsertCallback(', '/* ---------- visibility (occlusion) tests */'),
    between('void WINAPI D3DDevice_SetTransform(', 'void WINAPI D3DDevice_SetFlickerFilter(')
]
with open(sys.argv[1], 'w') as out:
    out.write('/* Generated unchanged CPU function bodies from d3d8_gl.c. */\n')
    out.write('\n'.join(parts))
# Device defaults share the same upstream source of truth.
with open(sys.argv[2], 'w') as out:
    out.write(between('\t\tfor (index = 0; index < D3DTS_MAX; index++)\n\t\t{\n\t\t\tdevice.transforms[index]._11 = 1.0f;',
                      '\t\tdevice.viewport.Width = width;'))
    out.write(between('\t\tD3D__RenderState[D3DRS_ZENABLE] = TRUE;', '\t\tviewport_update_constants();'))
