#!/usr/bin/env python3
"""Export complete shader layouts already witnessed by the shared validator."""
from pathlib import Path
import re
source=Path('port/linux/game/tag_schema_render.c').read_text()
declarations=[]
names=('shader_layer','shader_transparent','shader_transparent_chicago_map','shader_transparent_chicago_definition','shader_transparent_chicago_extended_definition','shader_transparent_glass_definition','shader_transparent_meter_definition','shader_environment_definition','shader_model_definition')
for name in names:
    declaration_source=source
    if name=='shader_transparent_meter_definition':
        declaration_source=Path('source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c').read_text()
    match=re.search(r'struct '+name+r'\n\{[^{}]*\n\};',declaration_source)
    if not match:raise ValueError('shader declaration changed: '+name)
    declaration=match.group()
    if name=='shader_environment_definition':
        # rasterizer_xbox_environment.c reads these values from the byte spans
        # that the older tag-schema view called reserved. Preserve the witnessed
        # offsets while making the native decoder endian-convert the fields.
        renderer=Path('source/rasterizer/xbox/rasterizer_xbox_environment.c').read_text()
        specular=re.search(r'struct shader_environment_specular_properties\n\{[^{}]*\n\};',renderer)
        reflection=re.search(r'struct shader_environment_reflection_properties\n\{[^{}]*\n\};',renderer)
        if not specular or not reflection:
            raise ValueError('environment renderer property declarations changed')
        for member in ('real brightness;', 'real_rgb_color view_perpendicular_color;',
                       'real_rgb_color view_parallel_color;'):
            if member not in specular.group():
                raise ValueError('environment specular source field changed: '+member)
        for member in ('real lightmap_brightness_scale;',
                       'real view_perpendicular_brightness;',
                       'real view_parallel_brightness;'):
            if member not in reflection.group():
                raise ValueError('environment reflection source field changed: '+member)
        old_spec='byte reserved280[0x50];'
        old_reflection='byte reserved2D4[0x50];'
        if declaration.count(old_spec)!=1 or declaration.count(old_reflection)!=1:
            raise ValueError('environment shader reserved spans changed')
        declaration=declaration.replace(
            old_spec,
            'byte reserved280[0x10];\n\treal specular_brightness;\n'
            '\tbyte reserved294[0x14];\n'
            '\treal_rgb_color specular_view_perpendicular_color;\n'
            '\treal_rgb_color specular_view_parallel_color;\n'
            '\tbyte reserved2C0[0x10];')
        declaration=declaration.replace(
            old_reflection,
            'real reflection_lightmap_brightness_scale;\n'
            '\tbyte reserved2D8[0x1C];\n'
            '\treal reflection_view_perpendicular_brightness;\n'
            '\treal reflection_view_parallel_brightness;\n'
            '\tbyte reserved2FC[0x28];')
    declarations.append(declaration)
Path('port/mac/native_shader_layout.h').write_text('\n'.join([
    '/* Generated from tag_schema_render.c; do not edit. */',
    '#ifndef HALO_MAC_NATIVE_SHADER_LAYOUT_H','#define HALO_MAC_NATIVE_SHADER_LAYOUT_H',
    '#include "shaders/shader_definitions.h"',*declarations,
    'typedef char native_environment_shader_size[sizeof(struct shader_environment_definition)==0x344?1:-1];',
    'typedef char native_environment_shader_base_map[offsetof(struct shader_environment_definition,base_map)==0x88?1:-1];',
    '/* Offsets follow the nested source views in rasterizer_xbox_environment.c. */',
    'typedef char native_environment_specular_brightness[offsetof(struct shader_environment_definition,specular_brightness)==0x290?1:-1];',
    'typedef char native_environment_specular_perpendicular_color[offsetof(struct shader_environment_definition,specular_view_perpendicular_color)==0x2A8?1:-1];',
    'typedef char native_environment_specular_parallel_color[offsetof(struct shader_environment_definition,specular_view_parallel_color)==0x2B4?1:-1];',
    'typedef char native_environment_reflection_lightmap_scale[offsetof(struct shader_environment_definition,reflection_lightmap_brightness_scale)==0x2D4?1:-1];',
    'typedef char native_environment_reflection_perpendicular_brightness[offsetof(struct shader_environment_definition,reflection_view_perpendicular_brightness)==0x2F4?1:-1];',
    'typedef char native_environment_reflection_parallel_brightness[offsetof(struct shader_environment_definition,reflection_view_parallel_brightness)==0x2F8?1:-1];',
    'typedef char native_chicago_shader_size[sizeof(struct shader_transparent_chicago_definition)==0x6c?1:-1];',
    'typedef char native_chicago_extended_size[sizeof(struct shader_transparent_chicago_extended_definition)==0x78?1:-1];',
    'typedef char native_glass_shader_size[sizeof(struct shader_transparent_glass_definition)==0x1E0?1:-1];',
    'typedef char native_meter_shader_size[sizeof(struct shader_transparent_meter_definition)==0x104?1:-1];',
    'typedef char native_meter_shader_flags_offset[offsetof(struct shader_transparent_meter_definition,flags)==0x28?1:-1];',
    'typedef char native_meter_shader_map_offset[offsetof(struct shader_transparent_meter_definition,map)==0x4C?1:-1];',
    'typedef char native_meter_shader_gradient_min_offset[offsetof(struct shader_transparent_meter_definition,gradient_min_color)==0x7C?1:-1];',
    'typedef char native_meter_shader_gradient_max_offset[offsetof(struct shader_transparent_meter_definition,gradient_max_color)==0x88?1:-1];',
    'typedef char native_meter_shader_background_offset[offsetof(struct shader_transparent_meter_definition,background_color)==0x94?1:-1];',
    'typedef char native_meter_shader_flash_offset[offsetof(struct shader_transparent_meter_definition,flash_color)==0xA0?1:-1];',
    'typedef char native_meter_shader_tint_offset[offsetof(struct shader_transparent_meter_definition,tint_color)==0xAC?1:-1];',
    'typedef char native_meter_shader_meter_alpha_offset[offsetof(struct shader_transparent_meter_definition,meter_transparency)==0xB8?1:-1];',
    'typedef char native_meter_shader_background_alpha_offset[offsetof(struct shader_transparent_meter_definition,background_transparency)==0xBC?1:-1];',
    'typedef char native_meter_shader_brightness_source_offset[offsetof(struct shader_transparent_meter_definition,meter_brightness_source)==0xD8?1:-1];',
    'typedef char native_meter_shader_flash_source_offset[offsetof(struct shader_transparent_meter_definition,flash_brightness_source)==0xDA?1:-1];',
    'typedef char native_meter_shader_value_source_offset[offsetof(struct shader_transparent_meter_definition,value_source)==0xDC?1:-1];',
    'typedef char native_meter_shader_gradient_source_offset[offsetof(struct shader_transparent_meter_definition,gradient_source)==0xDE?1:-1];',
    'typedef char native_meter_shader_flash_extension_source_offset[offsetof(struct shader_transparent_meter_definition,flash_extension_source)==0xE0?1:-1];',
    'typedef char native_model_shader_size[sizeof(struct shader_model_definition)==0x1B8?1:-1];',
    'typedef char native_model_shader_base_map[offsetof(struct shader_model_definition,base_map)==0xA4?1:-1];',
    '#endif','']))
