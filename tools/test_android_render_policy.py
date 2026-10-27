"""Chip detection and startup policy, using the production host and guest code."""
from pathlib import Path
import os
import shutil
import subprocess

import pytest

from harness import function
from test_android_ce_texture_cache import run_c

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def policy(tmp_path_factory):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang required")
    work = tmp_path_factory.mktemp("android-render-policy")
    source = work / "policy.c"
    header = ROOT / "port/android/include"
    host = (ROOT / "port/android/host/host_render_policy.c").read_text()
    code = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "android_render_policy.h"
#define PROP_VALUE_MAX 92
#define HOST_LOG_INFO 4
static const char *properties[4];
static unsigned queries;
static void host_logf(int priority, const char *format, ...) { (void)priority; (void)format; }
static int __system_property_get(const char *key, char *value)
{
    static const char *keys[] = { "ro.soc.manufacturer", "ro.soc.model", "ro.hardware", "ro.board.platform" };
    unsigned i;
    for (i = 0; i < 4; i++)
        if (!strcmp(key, keys[i])) {
            queries |= 1U << i;
            strcpy(value, properties[i]);
            return (int)strlen(value);
        }
    abort();
}
'''
    code += function(host, "host_android_chip_family")
    code += r'''
int main(int argc, char **argv)
{
    int i, expected;
    if (argc != 8) return 2;
    for (i = 0; i < 4; i++) properties[i] = argv[i + 1];
    expected = atoi(argv[5]);
    android_render_policy_initialize();
    if (queries != 15 || android_render_chip != expected ||
        android_geometry_fixes != atoi(argv[6]) || android_texture_fixes != atoi(argv[7])) {
        fprintf(stderr, "family=%d geometry=%d textures=%d queries=%u\n",
            android_render_chip, android_geometry_fixes, android_texture_fixes, queries);
        return 1;
    }
    return 0;
}
'''
    source.write_text(code)
    executable = work / ("policy.exe" if os.name == "nt" else "policy")
    target = ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"] if os.name == "nt" else ["-m32"]
    result = subprocess.run([compiler, *target, "-std=gnu89", "-Wall", "-Wextra", "-Werror",
                             "-I", str(header), str(source),
                             str(ROOT / "port/android/host/android_chip.c"),
                             str(ROOT / "port/android/guest/runtime/guest_render_policy.c"),
                             "-o", str(executable)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return executable


@pytest.mark.parametrize("manufacturer,model,hardware,platform,family,geometry,textures", [
    ("Qualcomm", "SM8450", "", "", 1, 1, 1),
    (" QUALCOMM Technologies, Inc. ", "SM8550-AB", "", "", 1, 1, 1),
    ("", "Snapdragon 8 Elite", "", "", 1, 1, 1),
    ("", "", "qcom", "msm8998", 1, 1, 1),
    ("", "", "", "sdm845", 1, 1, 1),
    ("QTI", "", "", "", 1, 1, 1),
    ("MediaTek", "MT6897", "", "", 2, 0, 1),
    ("", "", "mt6989", "MT6989Z", 2, 0, 1),
    ("Google", "Tensor G5", "", "", 3, 0, 0),
    ("", "", "", "gs201", 3, 0, 0),
    ("Samsung", "Exynos 2200", "", "", 3, 0, 0),
    ("", "", "samsungexynos9810", "", 3, 0, 0),
    ("UNISOC", "T820", "", "", 3, 0, 0),
    ("HiSilicon", "Kirin 9000", "", "", 3, 0, 0),
    ("NVIDIA", "Tegra", "", "", 3, 0, 0),
    ("Rockchip", "RK3588", "", "", 3, 0, 0),
    ("", "", "", "", 0, 1, 1),
    ("unknown", "unknown", "ranchu", "", 0, 1, 1),
    ("Future vendor", "new chip", "", "", 0, 1, 1),
    ("ARM", "Mali-G615", "", "", 0, 1, 1),
    ("Imagination", "PowerVR", "", "", 0, 1, 1),
    ("", "SM84500", "", "", 0, 1, 1),
    ("", "MT68", "", "", 0, 1, 1),
    ("", "SM", "", "", 0, 1, 1),
    ("notqualcomm", "", "", "", 0, 1, 1),
    ("Qualcomm", "Tensor G5", "qcom", "", 0, 1, 1),
    ("", "", "qcom", "mt6897", 0, 1, 1),
    ("Google", "Tensor G5", "qcom", "", 3, 0, 0),
])
def test_chip_policy(policy, manufacturer, model, hardware, platform, family, geometry, textures):
    result = subprocess.run([str(policy), manufacturer, model, hardware, platform,
                             str(family), str(geometry), str(textures)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("family", [-1, 4, 100])
def test_invalid_host_result_keeps_fixes(tmp_path, family):
    guest = (ROOT / "port/android/guest/runtime/guest_render_policy.c").read_text()
    guest = "\n".join(line for line in guest.splitlines() if not line.startswith("#include"))
    code = (ROOT / "port/android/include/android_render_policy.h").read_text()
    code += f"\nstatic int host_android_chip_family(void) {{ return {family}; }}\n" + guest
    code += "\nint main(void) { android_render_policy_initialize(); return android_render_chip || !android_geometry_fixes || !android_texture_fixes; }\n"
    run_c(tmp_path, code)


def test_policy_is_selected_before_game_resources():
    start = function((ROOT / "port/android/guest/runtime/guest_start.c").read_text(), "__guest_start")
    assert start.index("__guest_thread_initialize_main();") < start.index("android_render_policy_initialize();")
    assert start.index("android_render_policy_initialize();") < start.index("(*constructor)();") < start.index("result = main(")
    imports = (ROOT / "port/android/host_imports.list").read_text().splitlines()
    assert imports.count("host_android_chip_family") == 1


@pytest.mark.parametrize("enabled,ce,cached,valid,loads,legacy", [
    (1, 1, 1, 1, 1, 0), (0, 1, 1, 1, 0, 1),
    (1, 0, 1, 1, 0, 1), (0, 0, 1, 1, 0, 1),
    (1, 1, 0, 1, 0, 1), (1, 1, 1, 0, 0, 0),
])
def test_texture_dispatch_boundaries(tmp_path, enabled, ce, cached, valid, loads, legacy):
    source = (ROOT / "source/cache/xbox_texture_cache.c").read_text()
    get = function(source, "_texture_cache_bitmap_get_hardware_format")
    block = get[get.index("#ifdef HALO_ANDROID"):get.index("#endif") + len("#endif")]
    code = r'''
#include <stddef.h>
#define HALO_ANDROID 1
#define TEST_FLAG(f,b) ((f) & (1 << (b)))
#define _bitmap_cached_bit 0
#define MAX(a,b) ((a) > (b) ? (a) : (b))
struct bitmap_data { int flags, pixels_size; };
static int ce, valid, android_texture_fixes, loads, legacy;
static int custom_edition_cache_tags_loaded(void) { return ce; }
static int texture_cache_bitmap_valid(struct bitmap_data *b) { return valid; }
static int rasterizer_xbox_bitmap_get_pixel_data_size(struct bitmap_data *b) { return 1024; }
static void texture_cache_read_android_ce(struct bitmap_data *b, void *p) {}
static void *android_ce_texture_get(struct bitmap_data *b, int size, int load, void (*read)(struct bitmap_data *,void *)) { loads++; return b; }
static void dispatch(struct bitmap_data *bitmap) {
 void *hardware_format = NULL;
 int load = 1;
'''
    code += block + "\n{ legacy++; }\n}\nint main(void) { struct bitmap_data bitmap={0};\n"
    code += f"android_texture_fixes={enabled};ce={ce};valid={valid};bitmap.flags={cached};dispatch(&bitmap);\n"
    code += f"return loads!={loads} || legacy!={legacy}; }}\n"
    run_c(tmp_path, code)
