"""The NV2A shader translators' Metal Shading Language (port/linux/src/nv2a_vsh.c,
nv2a_psh.c), which OpenCE.app's renderer draws with.

Every vertex program of the game (source/rasterizer/xbox/
rasterizer_xbox_vertex_shaders_data.inc), and its per-pixel lighting variant
where it has one, and pixel shader keys of each of the translators' paths are
translated in the VM by the guest build's own objects (port/macos/tests/
vm_nv2a.c), then all compiled by Metal with the renderer's options
(tools/msl_check.swift). The GLSL the GL builds draw with is written by the
same code: its output must not change with the MSL's.

Needs `ninja macos` in `nix develop`, and a Metal device.
"""

import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import macos_guest_image  # noqa: E402

TRANSLATORS = [macos_guest_image.OBJECTS / f"port/linux/src/{name}.o" for name in ("nv2a_vsh", "nv2a_psh", "xgpu_text")]
DATA = ROOT / "source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_data.inc"
TABLE = ROOT / "source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c"
CHECK = ROOT / "tools/msl_check.swift"

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not macos_guest_image.available() or shutil.which("ninja") is None,
                       reason="needs `ninja macos` in nix develop"),
    pytest.mark.skipif(shutil.which("xcrun") is None, reason="needs Xcode's swift"),
]

# the attributes as the game's vertex types feed them: (packed, present)
ATTRIBUTES = [(0x0000, 0xffff), (0x000c, 0x001f), (0x0000, 0x0001)]

PIXEL_CASES = ["plain", "diffuse", "texture", "fog", "fog_exp2", "mux", "mux_lsb", "alpha_test",
               "alpha_test_samples", "alpha_test_never", "alpha_kill", "lighting", "point_lighting", "meter",
               "blend", "count_samples"]

# (stage, PSTextureModes' mode, sampler): every mode, each with the samplers
# it is drawn with (_xgpu_sampler_2d 1, 3d 2, cube 3)
TEXTURE_MODES = [(0, mode, sampler) for mode, sampler in ((0x01, 1), (0x02, 2), (0x03, 3), (0x04, 1), (0x05, 1))] + \
    [(1, mode, sampler) for mode in (0x06, 0x07, 0x08, 0x09, 0x0f) for sampler in (1,)] + \
    [(2, 0x0a, 1), (2, 0x0b, 1), (2, 0x0e, 1), (2, 0x10, 1), (2, 0x11, 1), (2, 0x12, 1)] + \
    [(3, 0x0c, 3), (3, 0x0d, 3), (3, 0x0d, 2), (3, 0x01, 2), (3, 0x03, 3)]


def vertex_programs():
    """[(index, instruction words)] of the game's vertex shader table"""
    words = [int(word, 16) for word in re.findall(r"0x([0-9A-Fa-f]{8})", DATA.read_text().split("*/", 1)[1])]
    programs = []
    for index, (offset, size) in enumerate(re.findall(r"VERTEX_SHADER_ENTRY\((0x[0-9A-Fa-f]+), (0x[0-9A-Fa-f]+)\)",
                                                      TABLE.read_text())):
        start = int(offset, 16) // 4
        header = words[start]
        count = header >> 16
        assert header & 0xffff == 0x2078 and 4 + count * 16 == int(size, 16), index
        programs.append((index, words[start + 1:start + 1 + count * 4]))
    return programs


@pytest.fixture(scope="module")
def vm_nv2a(tmp_path_factory):
    missing = [str(obj) for obj in TRANSLATORS if not obj.is_file()]
    if missing:
        pytest.skip("needs `ninja macos`: " + ", ".join(missing))
    work = tmp_path_factory.mktemp("vm_nv2a")
    return macos_guest_image.build_guest_image(
        [ROOT / "port/macos/tests/vm_nv2a.c", ROOT / "port/macos/tests/vm_nv2a_stubs.c"], work, "vm_nv2a",
        like="port/linux/src/xgpu_text.c", objects=TRANSLATORS)


def translate(image, folder, *arguments):
    run = macos_guest_image.run_guest(image, [str(argument) for argument in arguments], folder)
    return run.returncode, run.stdout, run.stderr


@pytest.fixture(scope="module")
def shaders(vm_nv2a, tmp_path_factory):
    """{name: (stage, glsl, msl)} of every case"""
    folder = tmp_path_factory.mktemp("vm_nv2a_run")
    cases = {}
    for index, words in vertex_programs():
        hex_words = ["%x" % word for word in words]
        for packed, present in ATTRIBUTES:
            name = f"vertex_{index}_{packed:x}_{present:x}"
            outputs = []
            for language in ("glsl", "msl"):
                code, out, err = translate(vm_nv2a, folder, "vertex", language, "%x" % packed, "%x" % present,
                                           "unlit", *hex_words)
                assert code == 0, f"{name} {language}: {err}"
                outputs.append(out)
            cases[name] = ("vertex", *outputs)
        outputs = []
        for language in ("glsl", "msl"):
            code, out, err = translate(vm_nv2a, folder, "vertex", language, "0", "ffff", "lit", *hex_words)
            outputs.append(out if code == 0 else None)
        if outputs[1] is not None:
            cases[f"vertex_{index}_lit"] = ("vertex", *outputs)
    for case in PIXEL_CASES:
        outputs = []
        for language in ("glsl", "msl"):
            code, out, err = translate(vm_nv2a, folder, "pixel", language, case)
            assert code == 0, f"{case} {language}: {err}"
            outputs.append(out)
        cases[f"pixel_{case}"] = ("fragment", *outputs)
    for stage, mode, sampler in TEXTURE_MODES:
        outputs = []
        for language in ("glsl", "msl"):
            code, out, err = translate(vm_nv2a, folder, "pixel", language, "mode", stage, "0x%x" % mode, sampler)
            assert code == 0, f"mode {stage} {mode:#x} {sampler} {language}: {err}"
            outputs.append(out)
        cases[f"pixel_mode_{stage}_{mode:x}_{sampler}"] = ("fragment", *outputs)
    return cases


def metal_compile(paths):
    # Xcode's Swift, outside the Nix toolchain, as the app is built
    # (tools/macos_build.py)
    environment = {key: value for key, value in os.environ.items()
                   if key not in ("SDKROOT", "DEVELOPER_DIR", "MACOSX_DEPLOYMENT_TARGET", "CC", "CXX", "LD")}
    run = subprocess.run(["/usr/bin/xcrun", "swift", str(CHECK), *map(str, paths)], capture_output=True, text=True,
                         timeout=600, env=environment)
    results = {}
    for line in run.stdout.splitlines():
        if line.startswith("/"):
            path, _, message = line.partition(": ")
            results[Path(path).name] = message
        elif results:
            last = list(results)[-1]
            results[last] += "\n" + line
    if not results:
        pytest.skip("Metal could not be run: " + run.stdout + run.stderr)
    return run.returncode, results


def write(folder, shaders):
    paths = []
    for name, (stage, _, msl) in shaders.items():
        path = folder / f"{name}.{stage}.metal"
        path.write_text(msl)
        paths.append(path)
    return paths


def test_every_program_was_translated(shaders):
    vertex = [name for name in shaders if name.startswith("vertex_") and not name.endswith("_lit")]
    assert len(vertex) == len(vertex_programs()) * len(ATTRIBUTES) == 67 * len(ATTRIBUTES)
    # the game's four model lighting programs (rasterizer_xbox_vertex_shaders_
    # initialize.c names them to halo_vertex_shader_lighting), which the
    # analysis finds lighting in; it finds it in program 31 too, which the
    # game does not light
    lit = {name for name in shaders if name.endswith("_lit")}
    assert {f"vertex_{index}_lit" for index in (9, 10, 17, 27)} <= lit
    assert len(lit) < 10


def test_the_msl_is_metal_not_gles(shaders):
    for name, (stage, glsl, msl) in shaders.items():
        assert "#version" not in msl and "precision " not in msl, name
        assert "#include <metal_stdlib>" in msl, name
        assert "gl_" not in msl, name
        if stage == "vertex":
            assert "vertex nv2a_vertex_out vertex_main(" in msl, name
            assert "[[position, invariant]]" in msl, name
            # Metal's clip space is GL's with glClipControl(UPPER_LEFT,
            # ZERO_TO_ONE): no flip of y and no remap of z, as the GLES path has
            assert "position.y = -" not in msl, name
            assert "* 2.0 - " not in msl.split("output.position")[-1], name
            assert "static_assert(sizeof(nv2a_vertex_uniforms) == " in msl, name
        else:
            assert "fragment nv2a_fragment_out fragment_main(" in msl, name
            assert "discard;" not in msl, name
            assert "static_assert(sizeof(nv2a_pixel_uniforms) == " in msl, name


def test_the_paths_are_translated_to_metal(shaders):
    msl = {name: source for name, (_, _, source) in shaders.items()}
    # textures sample with the Xbox's LOD bias, which Metal's samplers lack too
    assert ".sample(sampler0, " in msl["pixel_texture"] and "bias(texture_lod_bias[0])" in msl["pixel_texture"]
    assert "texturecube<float> tex0 [[texture(0)]]" in msl["pixel_mode_0_3_3"]
    assert "texture3d<float> tex0 [[texture(0)]]" in msl["pixel_mode_0_2_2"]
    assert "texture2d<float>" not in msl["pixel_plain"]
    for case in ("alpha_test", "alpha_kill", "alpha_test_never"):
        assert "discard_fragment()" in msl[f"pixel_{case}"], case
    # the multisampled alpha test covers samples in proportion
    assert "[[sample_mask]]" in msl["pixel_alpha_test_samples"]
    assert "[[sample_mask]]" not in msl["pixel_alpha_test"]
    # per-pixel lighting: the normal (and the position, by the point lights)
    # from the vertex shader to the pixel shader
    assert "[[user(xWorldNormal)]]" in msl["vertex_9_lit"]
    assert "model_lights" in msl["pixel_lighting"] and "xWorldNormal" in msl["pixel_lighting"]
    assert "xWorldPosition" in msl["pixel_point_lighting"]
    # an absent attribute reads its constant; a packed one is unpacked
    assert "[[attribute(1)]]" not in msl["vertex_0_0_1"] and "u.attributes[1]" in msl["vertex_0_0_1"]
    assert "uint v2_packed [[attribute(2)]]" in msl["vertex_0_c_1f"]


def test_the_glsl_is_unchanged(shaders):
    # the GL builds' output, made by the same functions, has none of Metal's
    for name, (stage, glsl, _) in shaders.items():
        if glsl is None:
            continue
        assert glsl.startswith("#version 300 es"), name
        assert "metal" not in glsl and "[[" not in glsl and "static_assert" not in glsl, name


def test_metal_compiles_every_shader(shaders, tmp_path):
    paths = write(tmp_path, shaders)
    code, results = metal_compile(paths)
    failed = {name: message for name, message in results.items() if message != "ok"}
    assert not failed, "\n".join(f"{name}: {message}" for name, message in list(failed.items())[:10])
    assert code == 0 and len(results) == len(paths)


def test_metal_refuses_a_mismatched_uniform_layout(shaders, tmp_path):
    # the negative control: the layouts are checked against the C structs'
    # (a shader whose uniforms the C side lays out differently does not compile)
    _, _, msl = shaders["pixel_texture"]
    size = re.search(r"static_assert\(sizeof\(nv2a_pixel_uniforms\) == (\d+)", msl)
    assert size
    path = tmp_path / "mismatched.fragment.metal"
    path.write_text(msl.replace(size.group(0), size.group(0)[:-len(size.group(1))] + str(int(size.group(1)) + 16)))
    code, results = metal_compile([path])
    assert code == 1 and results["mismatched.fragment.metal"] != "ok"


def test_the_renderer_compiles_with_the_checked_options():
    # tools/msl_check.swift compiles as MetalRenderer.libraryOptions does
    renderer = (ROOT / "port/macos/app/Sources/OpenCERender/MetalRenderer.swift").read_text()
    check = CHECK.read_text()
    options = re.findall(r"options\.(\w+) = (\S+)", check)
    assert len(options) == 3
    for name, value in options:
        assert f"options.{name} = {value}" in renderer, name
    assert "makeLibrary(source: source, options: Self.libraryOptions)" in renderer
