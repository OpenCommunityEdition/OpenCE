"""Android omits optional lighting code, even for old configs or host glow policy."""
import io
from pathlib import Path
import re
import subprocess
from types import SimpleNamespace
import xml.etree.ElementTree as ET

from tools import android_build, ninja_syntax
from test_mcc_identity import compile_tool
from test_web_build import graph as web_graph

ROOT = Path(__file__).resolve().parents[1]


def test_android_build_excludes_feature_modules(monkeypatch, tmp_path):
    monkeypatch.chdir(ROOT)
    ndk = tmp_path / 'ndk'
    (ndk / 'toolchains/llvm/prebuilt/test-host').mkdir(parents=True)
    monkeypatch.setenv('ANDROID_NDK_HOST_TAG', 'test-host')
    monkeypatch.setattr(android_build, 'fetch_third_party', lambda: None)
    sln = SimpleNamespace(build_dir=tmp_path / 'graph', android_ndk=ndk,
                          android_guest_cc='clang', compiler_launcher=None,
                          port_release=False, port_profile=False, port_pgo='off',
                          port_pgo_profile=None, port_portable=True, port_lto='off')
    output = io.StringIO()
    android_build.generate_android_build(ninja_syntax.Writer(output), sln)
    android = output.getvalue().replace('\\', '/')
    web = web_graph().replace('\\', '/')
    for folder, names in [('game', android_build.DYNAMIC_LIGHTING_GAME_SOURCES),
                          ('src', android_build.DYNAMIC_LIGHTING_PLATFORM_SOURCES)]:
        for name in names:
            source = f'port/linux/{folder}/{name}'
            assert (ROOT / source).is_file()
            assert source not in android, source
            assert source in web, source
    stub = 'port/android/guest/dynamic_lighting_disabled.c'
    assert stub in android and stub not in web
    # Stock lighting, surface animation and network policy are still compiled.
    for source in ['source/objects/object_lights.c', 'port/linux/game/overshield_effect.c',
                   'port/linux/game/network_shield_effects.c']:
        assert source in android


def test_android_hooks_preserve_stock_values_and_surface_reset(tmp_path):
    (tmp_path / 'cseries.h').write_text('''
typedef int boolean;
typedef float real;
#define FALSE 0
#define NONE (-1)
''')
    (tmp_path / 'real_math.h').write_text('''
typedef struct { float x, y, z; } real_point3d;
typedef struct { float i, j, k; } real_vector3d;
typedef struct { float red, green, blue; } real_rgb_color;
''')
    (tmp_path / 'plasma_model_shader.h').write_text(
        (ROOT / 'port/linux/include/plasma_model_shader.h').read_text())
    # Compile the whole replacement against the real feature headers. There
    # are deliberately no config, network, allocation, collision or GL stubs:
    # an accidental dependency on those services must fail the link.
    source = tmp_path / 'check.c'
    production = (ROOT / 'port/android/guest/dynamic_lighting_disabled.c').read_text()
    source.write_text(production + r'''
#include <string.h>
static int resets;
void overshield_effect_reset(void) { resets++; }
int main(void)
{
    int map;
    real_rgb_color color = {0.2f, 0.7f, 0.3f}, saved = color;
    real_point3d position = {1, 2, 3}, old_position = position;
    real_vector3d direction = {0, 0, 1}, old_direction = direction;
    unsigned long pixel[64], old_pixel[64];
    float radius = 2.5f, axis[3] = {0, 0, 1}, origin[3] = {1, 2, 3};
    memset(pixel, 0x5a, sizeof(pixel));
    memcpy(old_pixel, pixel, sizeof(pixel));
    for (map = 0; map < 3; map++)
    {
        shield_glow_reset(); plasma_lighting_reset();
        muzzle_lighting_forget(); covenant_lighting_forget();
        covenant_emitter_lighting_forget(); rocket_lighting_forget();
        ammo_counter_lighting_forget(); needler_lighting_forget();
        plasma_lighting_window_begin(); shield_glow_window_begin();
        muzzle_lighting_window_begin(); covenant_lighting_window_begin();
        covenant_emitter_lighting_window_begin(); rocket_lighting_window_begin();
        muzzle_lighting_shot(42, 0); muzzle_lighting_shot(42, 1);
        muzzle_lighting_update(); covenant_lighting_update();
        covenant_emitter_lighting_update(); rocket_lighting_update(); shield_glow_update();
        plasma_lighting_prepare(17, 42, NONE); plasma_lighting_submitted(17, 1);
        muzzle_lighting_submitted(17, 1); covenant_lighting_submitted(17, 1);
        covenant_emitter_lighting_submitted(17, 1); rocket_lighting_submitted(17, 1);
        shield_glow_submitted(17, 1); plasma_model_lighting_reset();
        plasma_model_lighting_slot(17, 0); plasma_model_lighting_slot(17, 1);
        if (plasma_lighting_selected(1) || muzzle_lighting_selected(1) ||
            covenant_lighting_selected(1) || covenant_emitter_lighting_selected(1) ||
            rocket_lighting_selected(1) || shield_glow_selected(1) ||
            plasma_model_lighting_mask() || shield_glow_enabled() ||
            covenant_emitter_lighting_enabled() || covenant_lighting_owns(17, 42)) return 1;
        if (plasma_lighting_radius(17, 42, radius) != radius ||
            muzzle_lighting_radius(17, radius) != radius ||
            covenant_lighting_radius(17, 42, radius) != radius ||
            covenant_emitter_lighting_radius(17, radius) != radius ||
            rocket_lighting_radius(17, 42, radius) != radius ||
            shield_glow_radius(17, radius) != radius) return 2;
        plasma_specular_begin(1); plasma_falloff_begin(1); plasma_falloff_specular_begin(1);
        if (plasma_specular_color(0x7f2040ffUL) != 0x7f2040ffUL) return 3;
        covenant_lighting_color(42, 10, &color);
        ammo_counter_lighting_screen(0, &color); needler_lighting_material(0, &color);
        ammo_counter_lighting_draw_begin(0, 0x12345678UL); ammo_counter_lighting_draw_end();
        needler_lighting_transparent(0, (struct pixel_shader_definition *)pixel, 4);
        if (memcmp(&color, &saved, sizeof(color)) || memcmp(pixel, old_pixel, sizeof(pixel))) return 4;
        if (ammo_counter_lighting_position(42, &position, &direction) ||
            needler_lighting_position(42, &position, &direction) ||
            memcmp(&position, &old_position, sizeof(position)) ||
            memcmp(&direction, &old_direction, sizeof(direction))) return 5;
        if (muzzle_lighting_vehicle_occlusion(17) || plasma_lighting_vehicle_occlusion(17) ||
            covenant_emitter_lighting_vehicle_occlusion(17) ||
            covenant_emitter_lighting_occlusion_ignore(17) != NONE ||
            covenant_emitter_lighting_cone(1, axis, origin, &radius) || radius != 2.5f) return 6;
        plasma_lighting_frame_end(); muzzle_lighting_frame_end(); covenant_lighting_frame_end();
        covenant_emitter_lighting_frame_end(); rocket_lighting_frame_end(); shield_glow_frame_end();
        plasma_falloff_end(); plasma_falloff_dispose();
    }
    return resets == 3 ? 0 : 7;
}
''')
    # Unused hook arguments are intentional on the unsupported platform.
    source.write_text('#pragma clang diagnostic ignored "-Wunused-parameter"\n' + source.read_text())
    executable = compile_tool(tmp_path, [source])
    result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr


def test_android_menu_and_default_config_exclude_dynamic_lighting():
    import port_settings
    config = (ROOT / 'port/linux/src/port_config.c').read_text()
    spec = port_settings.SCREENS['video_settings/dynamic_lighting']
    category = next(row for row in port_settings.SCREENS['video_settings']['categories']
                    if row[1] == 'video_settings/dynamic_lighting')
    assert category[3] == 'desktop web'
    for _, setting, _, _, platform in spec['rows']:
        assert platform == 'desktop web'
        groups = {
            'lighting.weapon_glow': ['display.weapon_glow', 'display.fuel_rod_lighting',
                                    'display.ammo_counter_lighting', 'display.needler_lighting'],
            'lighting.shield_emitters': ['display.jackal_shield_lighting', 'display.covenant_shield_lighting'],
        }
        for member in groups.get(setting, [setting]):
            declaration = re.search(r'\{ "' + re.escape(member) + r'",[^\n]+', config)[0]
            assert '_platform_desktop,' in declaration
            assert '"false"' in declaration or member == 'display.shield_glow_intensity'
    for name, lines in port_settings.settings_files().items():
        if name.endswith(('video_settings.xml', 'dynamic_lighting.xml')):
            assert (ROOT / 'port/assets/menus/ce' / name).read_text() == '\n'.join(lines)
            widgets = ET.fromstring('\n'.join(lines))
            visible_on = set()
            for child in widgets.iter('child'):
                if child.get('widget', '').endswith('/op_dynamic_lighting'):
                    visible_on.update(child.get('platform', '').split())
            if name.endswith('video_settings.xml'):
                assert visible_on == {'desktop', 'web'}
