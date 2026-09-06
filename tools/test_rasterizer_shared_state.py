"""Compile shared state with the pinned VC7 ABI; January, not HCEA, owns offsets."""
from pathlib import Path
import struct
import subprocess

import pytest

from tools import coff_compare as cc

ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "xbox/bin/vc7/CL.Exe"
DEBUG_FIELDS = [
    ["fps_accumulation",0,1],
    ["pad01",1,1],
    ["stats",2,2],
    ["mode",4,2],
    ["wireframe",6,1],
    ["debug_model_vertices",7,1],
    ["debug_model_lod",8,2],
    ["debug_transparents",10,1],
    ["debug_meter_shader",11,1],
    ["models",12,1],
    ["model_transparents",13,1],
    ["draw_first_person_weapon_first",14,1],
    ["stencil_mask",15,1],
    ["environment",16,1],
    ["environment_lightmaps",17,1],
    ["environment_shadows",18,1],
    ["environment_diffuse_lights",19,1],
    ["environment_diffuse_textures",20,1],
    ["environment_decals",21,1],
    ["environment_specular_lights",22,1],
    ["environment_specular_lightmaps",23,1],
    ["environment_reflection_lightmap_mask",24,1],
    ["environment_reflection_mirrors",25,1],
    ["environment_reflections",26,1],
    ["environment_transparents",27,1],
    ["environment_fog",28,1],
    ["environment_fog_screen",29,1],
    ["water",30,1],
    ["lens_flares",31,1],
    ["dynamic_unlit_geometry",32,1],
    ["dynamic_lit_geometry",33,1],
    ["dynamic_screen_geometry",34,1],
    ["hud_motion_sensor",35,1],
    ["detail_objects",36,1],
    ["debug_geometry",37,1],
    ["debug_geometry_multipass",38,1],
    ["fog_atmosphere",39,1],
    ["fog_plane",40,1],
    ["bump_mapping",41,1],
    ["pad2A[2]",42,2],
    ["lightmap_ambient",44,4],
    ["reserved30[2]",48,2],
    ["pad3",50,2],
    ["lightmaps_incident_radiosity",52,1],
    ["lightmaps_filtering",53,1],
    ["pad36[2]",54,2],
    ["model_lighting_ambient",56,4],
    ["environment_alpha_testing",60,1],
    ["environment_specular_mask",61,1],
    ["shadows_convolution",62,1],
    ["shadows_debug",63,1],
    ["water_mipmapping",64,1],
    ["active_camouflage",65,1],
    ["active_camouflage_multipass",66,1],
    ["plasma_energy",67,1],
    ["lens_flares_occlusion",68,1],
    ["lens_flares_occlusion_debug",69,1],
    ["ray_of_buddha",70,1],
    ["screen_flashes",71,1],
    ["screen_effects",72,1],
    ["DXTC_noise",73,1],
    ["soft_filter",74,1],
    ["secondary_render_target_debug",75,1],
    ["profile_log",76,1],
    ["pad4D[3]",77,3],
    ["detail_objects_offset_multiplier",80,4],
    ["zbias",84,4],
    ["zoffset",88,4],
    ["force_all_player_views_to_default_player",92,1],
    ["safe_frame_bounds",93,1],
    ["freeze_flying_camera",94,2],
    ["zsprites",96,1],
    ["filthy_decal_fog_hack",97,1],
    ["smart",98,1],
    ["splitscreen_VB_optimization",99,1],
    ["profile_print_locks",100,1],
    ["pad65[3]",101,3],
    ["profile_objectlock_time",104,4],
    ["pad3_scale",108,4],
    ["f[6]",112,24],
    ["transparent_pixel_counter_active",136,1],
    ["transparent_pixel_counter",137,1],
    ["pad8A[2]",138,2],
]
PIXEL_FIELDS = [
    ["alpha_inputs","PSAlphaInputs",0,32],
    ["final_combiner_inputs_abcd","PSFinalCombinerInputsABCD",32,4],
    ["final_combiner_inputs_efg","PSFinalCombinerInputsEFG",36,4],
    ["constant_0","PSConstant0",40,32],
    ["constant_1","PSConstant1",72,32],
    ["alpha_outputs","PSAlphaOutputs",104,32],
    ["rgb_inputs","PSRGBInputs",136,32],
    ["compare_mode","PSCompareMode",168,4],
    ["final_combiner_constant_0","PSFinalCombinerConstant0",172,4],
    ["final_combiner_constant_1","PSFinalCombinerConstant1",176,4],
    ["rgb_outputs","PSRGBOutputs",180,32],
    ["combiner_count","PSCombinerCount",212,4],
    ["texture_modes","PSTextureModes",216,4],
    ["dot_mapping","PSDotMapping",220,4],
    ["input_texture","PSInputTexture",224,4],
    ["c0_mapping","PSC0Mapping",228,4],
    ["c1_mapping","PSC1Mapping",232,4],
    ["final_combiner_constants","PSFinalCombinerConstants",236,4],
]


@pytest.fixture(scope="module")
def layouts(tmp_path_factory):
    if not COMPILER.is_file():
        pytest.skip("the locally supplied VC7/XDK compiler is unavailable")
    output = tmp_path_factory.mktemp("rasterizer-state") / "layout.obj"
    result = subprocess.run(
        [str(COMPILER), "/nologo", "/c", "/Dxbox",
         "/I" + str(ROOT / "source"), "/I" + str(ROOT / "source/cseries"),
         "/I" + str(ROOT / "xbox/include"), "/Fo" + str(output),
         str(ROOT / "tools/fixtures/rasterizer_shared_state.c")],
        cwd=ROOT, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return cc.load(output)


def words(obj, name, count):
    owners = [s for s in obj["symbols"] if s["name"] == name and s["section"] > 0]
    assert len(owners) == 1
    owner = owners[0]
    section = obj["sections"][owner["section"] - 1]
    assert section["raw"] and owner["value"] + count * 4 <= section["size"]
    return struct.unpack_from("<" + "I" * count, obj["data"],
                              section["raw"] + owner["value"])


def test_january_debug_options_layout(layouts):
    values = words(layouts, "_rasterizer_debug_options_layout", 1 + 2 * len(DEBUG_FIELDS))
    assert values[0] == 0x8C  # The later HCEA record is 0x90 and has a different tail.
    for index, (name, offset, size) in enumerate(DEBUG_FIELDS):
        assert values[1 + 2 * index:3 + 2 * index] == (offset, size), name


def test_xbox_pixel_shader_layout(layouts):
    values = words(layouts, "_rasterizer_pixel_shader_layout", 2 + 4 * len(PIXEL_FIELDS))
    assert values[:2] == (0xF0, 0xF0)
    for index, (name, sdk_name, offset, size) in enumerate(PIXEL_FIELDS):
        assert values[2 + 4 * index:6 + 4 * index] == (offset, offset, size, size), (name, sdk_name)
