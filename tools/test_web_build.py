"""The web build (tools/web_build.py, port/web): the ninja graph's flags, what it
compiles and leaves out and what the site holds; the memory layout the build
and the sources agree on; version.json and licenses.txt; the sockets inside
the page (port/web/tests/web_net_test.c, built for this computer); and the
menus' platforms.

    python -m pytest -q tools/test_web_build.py

Needs no Emscripten: the graph is generated with a stand-in for emcc.
"""

import io
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "tools"))

from tools import ninja_syntax, web_build  # noqa: E402


@pytest.fixture(autouse=True)
def in_the_repository(monkeypatch):
    """(configure.py runs from the repository's root, and so do these)"""
    monkeypatch.chdir(ROOT)


def graph(release=False, emcc="/opt/emsdk/upstream/emscripten/emcc"):
    """build.ninja's web part, as configure.py writes it, its long lines
    joined again"""
    out = io.StringIO()
    sln = SimpleNamespace(build_dir=Path("build"), web_emcc=emcc, port_release=release, port_profile=False,
                          compiler_launcher=None)
    web_build.generate_web_build(ninja_syntax.Writer(out), sln)
    return re.sub(r" \$\n +", " ", out.getvalue())


def compile_flags(text, source):
    """the cflags of the unit compiled from source"""
    match = re.search(r"^build [^\n]*: web_cc " + re.escape(source.replace(" ", "$ ")) + r"[ \n].*?\n  cflags = ([^\n]*)",
                      text, re.M | re.S)
    assert match, f"{source} is not compiled"
    return match.group(1)


def link_flags(text):
    return re.search(r"^build build/web/site/halo\.js.*?\n  ldflags = ([^\n]*)", text, re.M | re.S).group(1)


def test_without_emscripten_the_graph_says_so():
    import tools.web_build as module

    saved = module.find_emcc
    module.find_emcc = lambda sln: None
    try:
        text = graph(emcc=None)
    finally:
        module.find_emcc = saved
    assert "no Emscripten found" in text
    assert "\nbuild web:" not in text


def test_the_game_is_compiled_as_the_web_port():
    text = graph()
    flags = compile_flags(text, "source/main/main.c")
    for flag in ("-DHALO_GLES=1", "-DHALO_WEB=1", "-fshort-wchar", "-ffp-contract=off", "-pthread", "-std=gnu89",
                 "-fms-extensions", "-fwrapv", "-fno-delete-null-pointer-checks", "-fno-builtin-wcslen"):
        assert flag in flags.split(), flag
    # the browser is not the Android app, and calls must keep their types at
    # the link (no link-time optimisation: tools/web_build.py)
    assert "HALO_ANDROID" not in text
    assert "-flto" not in text
    assert "-DHALO_RELEASE" not in flags
    assert "-Dmain=halo_game_main" in compile_flags(text, "source/shell/shell_xbox.c")
    assert "halo_android_variadic_prototypes.h" in compile_flags(text, "source/hs/hs.c")
    assert "-include time.h" in compile_flags(text, "source/bungie_net/common/random_numbers.c")


def test_the_platform_layer_leaves_out_what_a_browser_has_not():
    text = graph()
    for name in web_build.WEB_EXCLUDE:
        assert f"port/linux/src/{name}" not in text, name
    for source in sorted((ROOT / "port/web/src").glob("*.c")):
        compile_flags(text, f"port/web/src/{source.name}")
    compile_flags(text, "port/android/host/host_watch_hash.c")
    # the C library's side keeps its own ABI (no MSVC semantics header)
    assert "msvc_semantics" not in compile_flags(text, "port/web/src/web_net.c")
    assert "platform_msvc_semantics.h" in compile_flags(text, "port/linux/src/d3d8_gl.c")


def test_the_link_makes_the_memory_the_window_needs():
    flags = link_flags(graph()).split()
    for flag in ("-sPROXY_TO_PTHREAD=1", "-sOFFSCREENCANVASES_TO_PTHREAD=#canvas", "-sMIN_WEBGL_VERSION=2",
                 "-sMAX_WEBGL_VERSION=2", "-sWASMFS=1", "-lopfs.js", "-sALLOW_MEMORY_GROWTH=0",
                 "-Wl,--fatal-warnings", "-sASSERTIONS=1", "--profiling-funcs"):
        assert flag in flags, flag
    assert f"-sINITIAL_MEMORY={0x88000000}" in flags
    release = graph(release=True)
    assert "-DHALO_RELEASE" in compile_flags(release, "source/main/main.c")
    assert "-sASSERTIONS=0" in link_flags(release).split()


def test_the_memory_layout_agrees_with_the_sources():
    platform = (ROOT / "port/linux/src/platform.h").read_text()
    base = int(re.search(r"#define PLATFORM_CONTIGUOUS_BASE (0x[0-9A-Fa-f]+)", platform).group(1), 16)
    sizes = re.search(r"#if defined\(HALO_ARM64_GUEST\) \|\| defined\(HALO_WEB\).*?#define PLATFORM_CONTIGUOUS_SIZE "
                      r"(0x[0-9A-Fa-f]+)", platform, re.S)
    assert sizes, "platform.h gives the web build the 128 MB window"
    assert web_build.WEB_MEMORY_BYTES == base + int(sizes.group(1), 16)
    main = (ROOT / "port/web/src/web_main.c").read_text()
    assert int(re.search(r"#define WEB_HEAP_LIMIT (0x[0-9A-Fa-f]+)", main).group(1), 16) == base
    app = (ROOT / "port/web/site/app.js").read_text()
    assert int(re.search(r"const WEB_MEMORY_BYTES = (0x[0-9A-Fa-f]+)", app).group(1), 16) == web_build.WEB_MEMORY_BYTES


def test_the_site_holds_the_page_its_fonts_licences_and_version():
    text = graph()
    phony = re.search(r"^build web: phony ([^\n]*)", text, re.M).group(1)
    outputs = set(phony.split())
    for name in ("halo.js", "halo.wasm", "version.json", "licenses.txt", "fonts/OpenCE-Regular.ttf",
                 "fonts/Overpass-750.ttf"):
        assert f"build/web/site/{name}" in outputs, name
    for source in web_build.site_sources():
        assert f"build/web/site/{source.relative_to(Path('port/web/site')).as_posix()}" in outputs, source


def test_version_json_names_the_build_and_its_files(tmp_path, monkeypatch):
    sys.path.insert(0, str(ROOT / "port/web"))
    import stamp_version

    site = tmp_path / "site"
    (site / "icons").mkdir(parents=True)
    (site / "halo.js").write_text("js")
    (site / "icons" / "icon.png").write_bytes(b"png")
    (site / ".nojekyll").write_text("")
    monkeypatch.setenv("HALO_BUILD_NUMBER", "321")
    monkeypatch.setenv("GITHUB_SHA", "0123abc")
    version = stamp_version.stamp(site / "version.json", [site / "halo.js", site / "icons" / "icon.png",
                                                          site / ".nojekyll"], "release")
    written = json.loads((site / "version.json").read_text())
    assert written == version
    assert written["build"] == 321 and written["commit"] == "0123abc" and written["flavor"] == "release"
    assert written["files"] == ["halo.js", "icons/icon.png"]
    limits = (ROOT / "port/linux/include/halo_port_limits.h").read_text()
    assert written["network_version"] == int(re.search(r"HALO_PORT_NETWORK_VERSION (\d+)", limits).group(1))
    (site / "halo.js").write_text("js changed")
    again = stamp_version.stamp(site / "version.json", [site / "halo.js", site / "icons" / "icon.png"], "release")
    assert again["version"] != written["version"]


def test_licenses_txt_has_every_notice():
    sys.path.insert(0, str(ROOT / "port/web"))
    import licenses

    for title, path in licenses.NOTICES:
        assert (ROOT / path).is_file(), (title, path)


@pytest.mark.skipif(not (shutil.which("cc") or shutil.which("clang")), reason="no C compiler")
def test_the_sockets_inside_the_page(tmp_path):
    compiler = shutil.which("cc") or shutil.which("clang")
    program = tmp_path / "web_net_test"
    subprocess.run([compiler, "-std=gnu11", "-Wall", "-g", "-pthread", f"-I{ROOT / 'port/linux/src'}",
                    str(ROOT / "port/web/tests/web_net_test.c"), str(ROOT / "port/web/src/web_net.c"),
                    "-o", str(program)], check=True)
    result = subprocess.run([str(program)], capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stderr
    assert "web_net: ok" in result.stdout


def test_the_menus_name_known_platforms_and_match_their_generator():
    import port_settings

    for path in sorted((ROOT / "port/assets/menus/ce").glob("*.xml")):
        for value in re.findall(r'platform="([^"]*)"', path.read_text()):
            assert value.split() and set(value.split()) <= {"desktop", "android", "web"}, (path.name, value)
    for name, lines in [*port_settings.settings_files().items(), *port_settings.multiplayer_files().items()]:
        assert (ROOT / "port/assets/menus/ce" / name).read_text() == "\n".join(lines), name
    # the rows a browser has no use for are not on its screens
    network = (ROOT / "port/assets/menus/ce/main_menu.settings_select.player_setup.player_profile_edit."
               "network_setup.xml").read_text()
    for row in ("op_online", "op_allow_upnp", "op_join_from_clipboard", "op_auto"):
        assert re.search(rf'<child widget="[^"]*/{row}"[^>]*platform="desktop android"', network), row
