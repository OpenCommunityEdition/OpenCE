"""Production layouts shared by the CPU-only transparent rendering tests."""
from harness import enum_with, read, structure


def transparent_types():
    declarations = []
    for path, names in [
        ("source/tag_files/tag_groups.h", ["tag_block", "tag_reference"]),
        ("source/shaders/shader_definitions.h", ["shader_radiosity_properties", "shader_physics_properties", "shader_base", "shader"]),
        ("source/rasterizer/rasterizer_geometry.h", ["vertex_buffer", "triangle_buffer"]),
        ("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c", ["shader_transparent_generic", "shader_transparent_generic_definition", "shader_transparent_glass_definition", "transparent_geometry_group"]),
    ]:
        text = read(path)
        declarations += [structure(text, name) for name in names]
    text = read("source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c")
    for member in ["_shader_type_screen", "_shader_transparent_flag_alpha_tested_bit", "_shader_transparent_glass_flag_alpha_tested_bit", "_shader_transparent_glass_reflection_type_bumped_cube_map", "_shader_radiosity_FILTHY_transparent_lit_bit", "_framebuffer_fade_mode_none", "_framebuffer_blend_function_alpha_blend", "_rasterizer_geometry_no_sort_bit"]:
        declarations.append(enum_with(text, member))
    text = read("source/rasterizer/rasterizer_geometry.h")
    declarations += [enum_with(text, "_rasterizer_vertex_type_environment_uncompressed"), enum_with(text, "_triangle_buffer_type_triangles")]
    return "\n".join(declarations)
