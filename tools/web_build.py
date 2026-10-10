"""Ninja rules for the web build (``ninja web``).

The web port (port/web/README.md) runs the game as WebAssembly in a
browser. wasm32 is an ILP32 target (32-bit int, long and pointers), as the
game's data formats need, and the Xbox window at 0x80000000 lies inside the
WebAssembly memory, which is made 0x88000000 bytes so that the window is its
top 128 MB (port/linux/src/platform.h). This graph builds

- the game sources and the port's game units (port/linux/game);
- the platform layer shared with Linux (port/linux/src), with the OpenGL ES
  renderer (HALO_GLES, WebGL 2) and HALO_WEB for what a browser does
  differently, less the units a browser has no use for (WEB_EXCLUDE);
- the web runtime (port/web/src): the entry point, which mounts the
  browser's storage, the sockets the game opens among themselves, and the
  memory watch by page contents;
- SDL 3 as an Emscripten port (port/web/halo_sdl3.py), the release the
  other builds use;

and links build/web/site/halo.js and halo.wasm beside the page, its service
worker and the disc image importer (port/web/site), with version.json naming
the build (port/web/stamp_version.py). The site needs nothing else: the
menus, fonts and HUD are embedded as in the other builds, and the player's
own disc image gives the maps.

Calls in WebAssembly must match the callee's type exactly, where x86
shrugged. The link checks every direct call against its callee's type and
fails on a mismatch (wasm-ld's warning made an error, --fatal-warnings),
which the sources then correct (HALO_WEB, or a declaration given its
definition's type); the game files that call variadic functions without a
prototype get one, as on Android (VARIADIC_PROTOTYPE_FILES); calls through
function pointers are adapted in the sources where the type differs. Units
are not optimised together at the link (no -flto): LLVM's link-time
optimisation turns a call whose declared parameter types differ only in
width (a boolean passed where the definition takes a short, which
WebAssembly passes alike) into a trap.
"""

import json
import os
import shutil
from pathlib import Path
from typing import Any, Dict, List, Optional

from .android_build import VARIADIC_PROTOTYPE_FILES
from .embed_assets import hud_assets_build, hud_configure_inputs
from .linux_build import (EXPAT_DIR, EXPAT_SOURCES, KCP_DIR, MONOCYPHER_DIR, MUSL_MATH_DIR, TOML_DIR, XDK_INCLUDE,
                          ZLIB_DEFINES, ZLIB_DIR, ZLIB_SOURCES, compile_launcher, configuration_defines,
                          game_defines_and_includes, game_sources, musl_math_cflags, musl_math_sources,
                          opus_cflags, opus_sources, updater_defines, xdk_headers)
from .ninja_syntax import Writer

PORT_DIR = Path("port/web")
LINUX_DIR = Path("port/linux")
ANDROID_DIR = Path("port/android")
PORT_CONFIG = LINUX_DIR / "port.json"
SDL_PORT = PORT_DIR / "halo_sdl3.py"
# (the release port/web/halo_sdl3.py fetches, as the other builds')
SDL_TAG = "release-3.4.16"
# the page's fonts, the menus' own (port/assets/fonts)
SITE_FONTS = ["OpenCE-Regular.ttf", "Overpass-750.ttf"]
# the memory watch by page contents, shared with the Android host
# (port/web/src/web_memory_watch.c)
WATCH_HASH = ANDROID_DIR / "host" / "host_watch_hash.c"

# The Xbox window (port/linux/src/platform.h) is the top 128 MB of the
# WebAssembly memory, 0x80000000 to 0x88000000; the C heap has everything
# below it (port/web/src/web_main.c).
WEB_MEMORY_BYTES = 0x88000000

# The ABI the game was written for, as the other builds reproduce it
# (tools/linux_build.py LINUX_ABI_FLAGS): 16-bit wchar_t, MSVC's extensions,
# common symbols, no optimisations that assume the absence of what MSVC
# tolerated, and no fused multiply-adds (the same results on every port).
# wasm32 aligns doubles and 64-bit integers to 8 bytes already, as MSVC.
WEB_ABI_FLAGS = [
    "-DHALO_GLES=1",
    "-DHALO_WEB=1",
    "-fshort-wchar",
    "-ffp-contract=off",
    "-pthread",
    "-msimd128",
    "-mbulk-memory",
    "-mnontrapping-fptoint",
    "-O2",
    "-g2",
]

WEB_CODE_FLAGS = [
    "-fms-extensions",
    "-fcommon",
    "-fno-strict-aliasing",
    "-fwrapv",
    "-fno-delete-null-pointer-checks",
    # Emscripten's C library has a 32-bit wchar_t: no loop turned into a call
    # to its wide string functions
    *(f"-fno-builtin-{name}" for name in (
        "wcslen", "wcsnlen", "wcschr", "wcsrchr", "wcscmp", "wcsncmp", "wcscpy",
        "wcsncpy", "wcscat", "wcsncat", "wmemchr", "wmemcmp", "wmemcpy",
        "wmemmove", "wmemset",
    )),
]

GAME_FLAGS = [
    "-std=gnu89",
    "-D__STRICT_ANSI__",
    "-w",
    "-Wno-error=incompatible-pointer-types",
    "-Wno-error=incompatible-function-pointer-types",
    "-Wno-error=int-conversion",
    "-Wno-error=implicit-function-declaration",
    "-Wno-error=implicit-int",
    "-Wno-error=return-type",
]

# game files that call C library functions without a prototype, whose
# WebAssembly types differ from the undeclared ones (time_t is 64-bit)
PROTOTYPE_FILES = {
    "source/bungie_net/common/random_numbers.c": "time.h",
}

# platform units a browser does without (updater.c and xiso.c compile to
# nothing there: the site updates itself, and the page reads the disc image)
WEB_EXCLUDE = {
    "memory_watch.c",       # page protection: port/web/src/web_memory_watch.c
    "posix_net.c",          # sockets: port/web/src/web_net.c
    "posix_update.c",       # the self-updater's downloads
    "posix_upnp.c",         # internet play's router (port/web/src/web_stubs.c)
    "posix_trace_marker.c", # a Linux GPU driver's trace markers
}

# the web runtime's units that, like posix_*.c, are the C library's side of
# the platform layer, with its own ABI
WEB_POSIX_SOURCES = {"web_main.c", "web_net.c", "web_stubs.c"}


def _quote(path: Any) -> str:
    text = str(path).replace(os.sep, "/")
    return f'"{text}"' if " " in text else text


def find_emcc(sln: Any) -> Optional[str]:
    """--web-emcc's, emcc on the PATH, or ~/emsdk's"""
    explicit = getattr(sln, "web_emcc", None)
    if explicit:
        return explicit
    found = shutil.which("emcc")
    if found:
        return found
    for root in (os.environ.get("EMSDK"), str(Path.home() / "emsdk")):
        if root and (Path(root) / "upstream" / "emscripten" / "emcc").is_file():
            return str(Path(root) / "upstream" / "emscripten" / "emcc")
    return None


def site_sources() -> List[Path]:
    """the page's files (port/web/site), copied beside halo.js"""
    return sorted(path for path in (PORT_DIR / "site").rglob("*") if path.is_file())


def web_configure_inputs() -> List[Path]:
    return [Path(__file__), PORT_DIR / "src", PORT_DIR / "site", PORT_DIR / "site" / "icons", SDL_PORT,
            *hud_configure_inputs()]


def generate_web_build(n: Writer, sln: Any) -> None:
    if not PORT_CONFIG.is_file() or not (PORT_DIR / "src").is_dir():
        return
    emcc = find_emcc(sln)
    if not emcc:
        n.comment("Web build: no Emscripten found (put emcc on the PATH, or pass --web-emcc)")
        return
    config: Dict[str, Any] = json.loads(PORT_CONFIG.read_text(encoding="utf-8"))
    release = getattr(sln, "port_release", False)

    build_dir: Path = sln.build_dir / "web"
    obj_dir = build_dir / "obj"
    site_dir = build_dir / "site"
    semantics_header = sln.build_dir / "linux" / "halo_msvc_semantics.h"
    platform_semantics_header = sln.build_dir / "linux" / "platform_msvc_semantics.h"
    prefix_header = LINUX_DIR / "include" / "halo_linux_prefix.h"
    sdl_flag = f"--use-port={SDL_PORT}"

    n.comment("Web build (ninja web); see port/web/README.md")
    n.variable("web_emcc", _quote(emcc))
    n.rule(
        name="web_cc",
        command=f"{compile_launcher(sln)}$web_emcc -MMD -MF $out.d $cflags -c $in -o $out",
        description="WEB CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    # SDL's headers come with its port, which emcc fetches and builds the
    # first time a unit asks for it: one unit does that first (pool console:
    # its download shows), so that the others do not race for it
    # (and its license, from its source, for licenses.txt)
    sdl_stamp = build_dir / "sdl3.stamp"
    sdl_license = build_dir / "SDL3-LICENSE.txt"
    em_config = Path(emcc).with_name("em-config")
    n.rule(
        name="web_sdl3",
        command=f"echo '#include <SDL3/SDL.h>' > $out.c && $web_emcc {sdl_flag} -pthread -c $out.c -o $out.o && "
                f"rm -f $out.c $out.o && cp \"$$({_quote(em_config)} CACHE)/ports/halo_sdl3/SDL-{SDL_TAG}/LICENSE.txt\" "
                f"{sdl_license} && touch $out",
        description="WEB SDL3 (Emscripten port, fetched and built once)",
        pool="console",
    )
    n.build(outputs=sdl_stamp, implicit_outputs=[sdl_license], rule="web_sdl3", implicit=[SDL_PORT])

    abi = " ".join(WEB_ABI_FLAGS + configuration_defines(sln))
    code = " ".join(WEB_CODE_FLAGS)
    implicit_headers = [*xdk_headers(), prefix_header, semantics_header, platform_semantics_header, sdl_stamp]
    objects: List[Path] = []

    def add_object(source: Path, cflags: str) -> None:
        relative = source.relative_to(build_dir) if str(source).startswith(str(build_dir)) else source
        obj = obj_dir / relative.with_suffix(".o")
        n.build(outputs=obj, rule="web_cc", inputs=source, implicit=implicit_headers, variables={"cflags": cflags})
        objects.append(obj)

    # ---------- the game

    game_cflags = " ".join([
        abi, code, " ".join(GAME_FLAGS),
        f"-include {prefix_header}", f"-include {semantics_header}",
        f"-I{LINUX_DIR}/include",
        # the headers of the port's own game units (port/linux/game)
        f"-iquote {Path(config['game_sources'])}",
        game_defines_and_includes(config), f"-idirafter {XDK_INCLUDE}",
    ])
    for source in game_sources(config):
        name = source.as_posix()
        cflags = game_cflags
        if name == "source/main/main.c":
            cflags += " " + updater_defines(release)
        if name in VARIADIC_PROTOTYPE_FILES:
            cflags += f" -include {ANDROID_DIR}/include/halo_android_variadic_prototypes.h"
        if name in PROTOTYPE_FILES:
            cflags += f" -include {PROTOTYPE_FILES[name]}"
        if name == "source/shell/shell_xbox.c":
            # the game starts once the browser's storage is mounted
            # (port/web/src/web_main.c)
            cflags += " -Dmain=halo_game_main"
        add_object(source, cflags)
    for source in sorted(Path(config["game_sources"]).glob("*.c")):
        add_object(source, game_cflags)

    # ---------- the platform layer and the web runtime

    platform_dir = Path(config["platform_sources"])
    platform_cflags = " ".join([
        abi, code, sdl_flag, "-std=gnu11", "-D_GNU_SOURCE", "-DHALO_LINUX_PLATFORM_LAYER", "-w",
        f"-include {prefix_header}", f"-include {platform_semantics_header}",
        f"-I{platform_dir}", f"-I{LINUX_DIR}/include", f"-I{PORT_DIR}/src", f"-I{ANDROID_DIR}/host",
        f"-I{TOML_DIR}", f"-I{EXPAT_DIR}", f"-I{KCP_DIR}", f"-I{MONOCYPHER_DIR}", f"-I{ZLIB_DIR}",
        "-Isource -Isource/cseries", f"-idirafter {XDK_INCLUDE}",
    ])
    # posix_*.c talk to the C library only, with its own ABI (as on Linux)
    posix_cflags = " ".join([
        abi, "-std=gnu11", "-D_GNU_SOURCE", "-D_FILE_OFFSET_BITS=64", "-w", f"-I{platform_dir}",
    ])
    for source in sorted(platform_dir.glob("*.c")):
        if source.name in WEB_EXCLUDE:
            continue
        add_object(source, posix_cflags if source.name.startswith("posix_") else platform_cflags)
    for source in hud_assets_build(n, "web", build_dir / "generated" / "hud_hires_assets.c"):
        add_object(source, platform_cflags)
    for source in sorted((PORT_DIR / "src").glob("*.c")):
        add_object(source, posix_cflags if source.name in WEB_POSIX_SOURCES else platform_cflags)
    add_object(WATCH_HASH, " ".join([abi, "-std=gnu11", "-w"]))

    # ---------- third-party libraries, as the other builds have them

    add_object(TOML_DIR / "tomlc17.c", " ".join([abi, "-std=gnu11", "-w"]))
    for name in EXPAT_SOURCES:
        add_object(EXPAT_DIR / name, " ".join([abi, "-std=gnu11", f"-I{EXPAT_DIR}", "-w"]))
    add_object(KCP_DIR / "ikcp.c", " ".join([abi, "-std=gnu11", "-w"]))
    for source in opus_sources():
        add_object(source, opus_cflags(abi))
    for name in ("monocypher.c", "monocypher-ed25519.c"):
        add_object(MONOCYPHER_DIR / name, " ".join([abi, "-std=gnu11", "-w"]))
    for name in ZLIB_SOURCES:
        add_object(ZLIB_DIR / name, " ".join([abi, "-std=gnu11", *ZLIB_DEFINES, "-w"]))
    for source in musl_math_sources():
        add_object(source, musl_math_cflags(abi))

    # ---------- the link

    library_js = PORT_DIR / "src" / "web_library.js"
    pre_js = PORT_DIR / "src" / "web_pre.js"
    halo_js = site_dir / "halo.js"
    halo_wasm = site_dir / "halo.wasm"
    link_flags = [
        "-O2", "-g2", "-pthread", sdl_flag,
        # a call that does not match its callee's type is an error, not a
        # trap when it runs
        "-Wl,--fatal-warnings",
        # the game's main on a thread of its own, where it may block; the
        # page's thread only passes events
        "-sPROXY_TO_PTHREAD=1",
        # (the game's threads, SDL's, the file system's: about ten at once)
        "-sPTHREAD_POOL_SIZE=16",
        "-sOFFSCREENCANVAS_SUPPORT=1",
        "-sOFFSCREENCANVASES_TO_PTHREAD=#canvas",
        "-sMIN_WEBGL_VERSION=2",
        "-sMAX_WEBGL_VERSION=2",
        "-sFULL_ES3=1",
        "-sGL_ENABLE_GET_PROC_ADDRESS=1",
        "-sWASMFS=1",
        "-lopfs.js",
        f"-sINITIAL_MEMORY={WEB_MEMORY_BYTES}",
        "-sALLOW_MEMORY_GROWTH=0",
        # (the game recurses deeply in places: scripts, the BSP's walks)
        "-sSTACK_SIZE=8MB",
        "-sDEFAULT_PTHREAD_STACK_SIZE=2MB",
        "-sEXIT_RUNTIME=0",
        "-sENVIRONMENT=web,worker",
        "-sERROR_ON_UNDEFINED_SYMBOLS=1",
        # the page gives the game its arguments and reads its output
        "-sMODULARIZE=1",
        "-sEXPORT_NAME=createHalo",
        # (the memory, which the page reads web_state's words from)
        "-sEXPORTED_RUNTIME_METHODS=HEAPU32",
        f"-sASSERTIONS={0 if release else 1}",
        # function names in stack traces (the name section only)
        "--profiling-funcs",
        f"--js-library {library_js}",
        f"--pre-js {pre_js}",
    ]
    n.rule(
        name="web_link",
        command="$web_emcc $ldflags -o $out @$out.rsp",
        description="WEB LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=halo_js, implicit_outputs=[halo_wasm], rule="web_link", inputs=objects,
            implicit=[library_js, pre_js, SDL_PORT, sdl_stamp], variables={"ldflags": " ".join(link_flags)})

    # ---------- the site: the page, its service worker and the importer

    n.rule(name="web_copy", command="cp $in $out", description="WEB STAGE $out")
    site_files: List[Path] = []
    for source in site_sources():
        target = site_dir / source.relative_to(PORT_DIR / "site")
        n.build(outputs=target, rule="web_copy", inputs=source)
        site_files.append(target)
    for name in SITE_FONTS:
        target = site_dir / "fonts" / name
        n.build(outputs=target, rule="web_copy", inputs=Path("port/assets/fonts") / name)
        site_files.append(target)
    licenses = site_dir / "licenses.txt"
    n.rule(
        name="web_licenses",
        command=f"$python {PORT_DIR}/licenses.py $out {sdl_license}",
        description="WEB LICENSES $out",
    )
    n.build(outputs=licenses, rule="web_licenses", implicit=[PORT_DIR / "licenses.py", sdl_license])
    site_files.append(licenses)
    n.rule(
        name="web_version",
        command=f"$python {PORT_DIR}/stamp_version.py --flavor {'release' if release else 'debug'} $out $in",
        description="WEB VERSION $out",
    )
    version_file = site_dir / "version.json"
    n.build(outputs=version_file, rule="web_version", inputs=[halo_js, halo_wasm, *site_files],
            implicit=[PORT_DIR / "stamp_version.py", Path("port/linux/include/halo_port_limits.h")])
    n.build(outputs="web", rule="phony", inputs=[halo_js, halo_wasm, version_file, *site_files])
    n.newline()
