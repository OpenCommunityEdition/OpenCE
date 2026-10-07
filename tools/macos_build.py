"""Ninja rules for the macOS build (``ninja macos``).

The macOS port (port/macos/README.md) runs the game as the Android port
does: ILP32 AArch64 code (32-bit pointers, as the game's data formats
require) inside an ordinary 64-bit process. This graph builds

- the guest image, build/macos/halo_guest.elf (tools/guest_build.py), with
  every memory access rebased onto a register (tools/guest_asm_rebase.py),
  since a native macOS process cannot map anything below 4 GB;
- SDL3, as a static library;
- the host, build/macos/halo: the loader and the services the guest calls,
  over SDL3 and OpenGL 4.1;

and puts them together as build/macos/Halo.app.

It needs Apple's command line tools (clang, which has the arm64_32 target),
ld.lld (Homebrew's lld), CMake and a network connection for the first
build (musl, SDL3 and the Khronos OpenGL ES headers are downloaded to
build/macos/third_party).
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

from .embed_assets import hud_configure_inputs
from .guest_build import GuestTarget, fetch_third_party, generate_guest
from .linux_build import MINIUPNPC_DEFINES, MINIUPNPC_DIR, compile_launcher, miniupnpc_sources
from .ninja_syntax import Writer

PORT_DIR = Path("port/macos")
ANDROID_DIR = Path("port/android")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/macos")
THIRD_PARTY = BUILD / "third_party"
TOML_DIR = Path("port/third_party/tomlc17")
# the OpenGL ES headers the guest is compiled against (it draws with the
# OpenGL ES 3 subset of OpenGL 4.1), at fixed revisions of Khronos' registries
KHRONOS_DIR = THIRD_PARTY / "khronos"
KHRONOS_GL = "https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/6af574a14089ccfee87efe230ebcdd8742859813"
KHRONOS_EGL = "https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/db3425b8246136faccb5e2782b5694960bd6edf1"
KHRONOS_HEADERS = [
    (f"{KHRONOS_GL}/api/GLES2/gl2ext.h", "GLES2/gl2ext.h"),
    (f"{KHRONOS_GL}/api/GLES2/gl2platform.h", "GLES2/gl2platform.h"),
    (f"{KHRONOS_GL}/api/GLES3/gl32.h", "GLES3/gl32.h"),
    (f"{KHRONOS_GL}/api/GLES3/gl3platform.h", "GLES3/gl3platform.h"),
    (f"{KHRONOS_EGL}/api/KHR/khrplatform.h", "KHR/khrplatform.h"),
]
# the oldest macOS the host runs on
MACOS_MINIMUM = "13.0"
HOST_FRAMEWORKS = [
    "AudioToolbox", "AVFoundation", "Carbon", "Cocoa", "CoreAudio", "CoreFoundation", "CoreHaptics",
    "CoreMedia", "CoreVideo", "ForceFeedback", "GameController", "IOKit", "Metal", "OpenGL", "QuartzCore",
    "UniformTypeIdentifiers",
]


def _find_lld(sln: Any) -> Optional[str]:
    explicit = getattr(sln, "macos_lld", None)
    if explicit:
        return explicit
    found = shutil.which("ld.lld")
    if found:
        return found
    for candidate in ("/opt/homebrew/opt/lld/bin/ld.lld", "/opt/homebrew/opt/llvm/bin/ld.lld",
                      "/usr/local/opt/lld/bin/ld.lld", "/usr/local/opt/llvm/bin/ld.lld"):
        if Path(candidate).is_file():
            return candidate
    return None


def fetch_khronos_headers() -> None:
    for url, name in KHRONOS_HEADERS:
        path = KHRONOS_DIR / name
        if path.is_file():
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        print(f"Downloading {url}")
        subprocess.run(["curl", "-sSfL", "-o", str(path), url], check=True)


def macos_configure_inputs() -> List[Path]:
    return [Path(__file__), Path("tools/guest_build.py"), PORT_DIR / "host", PORT_DIR / "guest", PORT_DIR / "host_imports.list",
            ANDROID_DIR / "guest" / "runtime", LINUX_DIR / "src", *hud_configure_inputs()]


def generate_macos_build(n: Writer, sln: Any) -> None:
    config_path = LINUX_DIR / "port.json"
    if sys.platform != "darwin" or not config_path.is_file() or not (PORT_DIR / "host").is_dir():
        return
    lld = _find_lld(sln)
    if not lld:
        n.comment("macOS build: no ld.lld found (brew install lld, or pass --macos-lld)")
        print("macOS build disabled: no ld.lld (brew install lld)", file=sys.stderr)
        return
    try:
        fetch_third_party(THIRD_PARTY)
        fetch_khronos_headers()
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"macOS build disabled: cannot fetch musl/SDL3/OpenGL headers ({error})", file=sys.stderr)
        return
    import json
    config: Dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))
    guest_cc = getattr(sln, "macos_guest_cc", None) or "clang"
    host_cc = getattr(sln, "macos_host_cc", None) or "clang"
    sdl_dir = THIRD_PARTY / "SDL3"
    host_table_c = BUILD / "host" / "host_import_table.c"
    python = "$python"

    n.comment("macOS build (ninja macos); see port/macos/README.md")
    n.variable("macos_host_cc", host_cc)

    # ---------- the guest image (tools/guest_build.py)

    image = generate_guest(n, sln, GuestTarget(
        name="macos",
        build=BUILD,
        third_party=THIRD_PARTY,
        guest_cc=guest_cc,
        # x27 and x28 are the rebased code's (tools/guest_asm_rebase.py),
        # which also lengthens the code between a jump table's targets past
        # what byte and halfword entries hold
        defines=["-DHALO_MACOS=1", "-ffixed-x27", "-ffixed-x28", "-mllvm", "-aarch64-enable-compress-jump-tables=false",
                 "-Wno-incompatible-sysroot"],
        gl_headers=KHRONOS_DIR.resolve(),
        import_lists=[PORT_DIR / "host_imports.list"],
        host_table=host_table_c,
        linker=lld,
        archiver=None,
        rebase=True,
        runtime_sources=sorted((PORT_DIR / "guest").glob("*.c")),
        runtime_includes=[PORT_DIR / "guest", PORT_DIR / "include"],
        assembler_target="aarch64-linux-gnu",
        posix_wrappers=True,
    ), config)
    n.build(outputs="macos_guest", rule="phony", inputs=image)

    # ---------- SDL3, as a static library

    sdl_build = BUILD / "sdl3-build"
    libsdl = sdl_build / "libSDL3.a"
    n.rule(
        name="macos_sdl3",
        command=(f"cmake -S {sdl_dir} -B {sdl_build} -G Ninja -DCMAKE_BUILD_TYPE=Release "
                 f"-DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET={MACOS_MINIMUM} "
                 "-DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF "
                 f"> {BUILD}/sdl3-configure.log && ninja -C {sdl_build} > {BUILD}/sdl3-build.log"),
        description="MACOS SDL3",
        pool="console",
    )
    n.build(outputs=libsdl, rule="macos_sdl3", implicit=[sdl_dir / "CMakeLists.txt"])

    # ---------- the host

    host_dir = BUILD / "host"
    host_obj_dir = host_dir / "obj"
    gl_wrappers_c = host_dir / "host_gl_wrappers.c"
    posix_wrappers_c = host_dir / "host_posix_wrappers.c"
    n.rule(
        name="macos_gl_wrappers",
        command=(f"{python} tools/macos_host_wrappers.py gl {LINUX_DIR}/src/gl.h {KHRONOS_DIR}/GLES3/gl32.h "
                 f"{KHRONOS_DIR}/GLES2/gl2ext.h $out"),
        description="MACOS GL WRAPPERS",
    )
    n.build(outputs=gl_wrappers_c, rule="macos_gl_wrappers",
            implicit=[Path("tools/macos_host_wrappers.py"), Path("tools/android_gl_stubs.py"), LINUX_DIR / "src" / "gl.h"])
    n.rule(
        name="macos_posix_wrappers",
        command=f"{python} tools/macos_host_wrappers.py posix {LINUX_DIR}/src/posix.h $out",
        description="MACOS POSIX WRAPPERS",
    )
    n.build(outputs=posix_wrappers_c, rule="macos_posix_wrappers",
            implicit=[Path("tools/macos_host_wrappers.py"), LINUX_DIR / "src" / "posix.h"])

    n.rule(
        name="macos_host_cc",
        command=f"{compile_launcher(sln)}$macos_host_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    host_arch = f"-arch arm64 -mmacosx-version-min={MACOS_MINIMUM}"
    host_cflags = " ".join([
        host_arch, "-O2", "-g", "-Wall", "-Wno-unused-function", "-D_DARWIN_C_SOURCE",
        f"-I{PORT_DIR}/host", f"-I{PORT_DIR}/include", f"-I{ANDROID_DIR}/include", f"-I{sdl_dir}/include",
        f"-I{LINUX_DIR}/src",
    ])
    host_objects: List[Path] = []

    def host_object(source: Path, cflags: str, name: Optional[str] = None) -> None:
        obj = host_obj_dir / (name or source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": cflags},
                order_only=[libsdl])
        host_objects.append(obj)

    for source in sorted((PORT_DIR / "host").glob("*.c")) + sorted((PORT_DIR / "host").glob("*.S")):
        host_object(source, host_cflags)
    # (the generated ones: the import table, and the wrappers that add the
    # guest's base to its pointers, the GL ones with the OpenGL ES headers)
    host_object(host_table_c, host_cflags)
    host_object(gl_wrappers_c, f"{host_cflags} -I{KHRONOS_DIR}")
    host_object(posix_wrappers_c, host_cflags)
    for source in (LINUX_DIR / "src" / "posix_files.c", LINUX_DIR / "src" / "posix_net.c"):
        host_object(source, host_cflags)
    # internet play's UPnP (posix_upnp.c, with port/third_party/miniupnpc)
    miniupnpc_cflags = " ".join([host_cflags, f"-I{MINIUPNPC_DIR / 'include'}", f"-I{MINIUPNPC_DIR / 'src'}",
                                 *MINIUPNPC_DEFINES])
    host_object(LINUX_DIR / "src" / "posix_upnp.c", miniupnpc_cflags)
    for source in miniupnpc_sources():
        host_object(source, miniupnpc_cflags + " -w", "miniupnpc_" + source.name + ".o")

    executable = BUILD / "halo"
    n.rule(
        name="macos_host_link",
        command=("$macos_host_cc " + host_arch + " -o $out @$out.rsp " + str(libsdl) + " -liconv "
                 + " ".join(f"-framework {framework}" for framework in HOST_FRAMEWORKS)
                 + " && codesign --force --sign - $out > /dev/null 2>&1"),
        description="MACOS HOST LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=executable, rule="macos_host_link", inputs=host_objects, implicit=[libsdl])

    # ---------- beside the executable, and the app

    n.rule(name="macos_copy", command="cp $in $out", description="MACOS COPY $out")
    brokers = BUILD / "brokers.txt"
    n.build(outputs=brokers, rule="macos_copy", inputs=Path("port/assets/network/brokers.txt"))

    app = BUILD / "Halo.app"
    contents = app / "Contents"
    icon = BUILD / "Halo.icns"
    n.rule(
        name="macos_icon",
        command=(f"rm -rf $out.iconset && mkdir -p $out.iconset && "
                 + " && ".join(f"sips -z {size} {size} $in --out $out.iconset/icon_{name}.png > /dev/null"
                               for size, name in ((16, "16x16"), (32, "16x16@2x"), (32, "32x32"),
                                                  (64, "32x32@2x"), (128, "128x128"), (256, "128x128@2x"),
                                                  (256, "256x256"), (512, "256x256@2x"), (512, "512x512")))
                 + " && iconutil -c icns $out.iconset -o $out && rm -rf $out.iconset"),
        description="MACOS ICON $out",
    )
    n.build(outputs=icon, rule="macos_icon", inputs=ANDROID_DIR / "art" / "android-icon.png")
    n.rule(
        name="macos_app",
        command=(f"rm -rf {app} && mkdir -p {contents}/MacOS {contents}/Resources && "
                 f"cp {PORT_DIR}/Info.plist {contents}/Info.plist && "
                 f"cp {executable} {contents}/MacOS/halo && "
                 f"cp {image} {brokers} {icon} {contents}/Resources/ && "
                 f"codesign --force --sign - {app} > /dev/null 2>&1 && touch $out"),
        description="MACOS APP $out",
    )
    app_stamp = BUILD / "Halo.app.stamp"
    n.build(outputs=app_stamp, rule="macos_app",
            inputs=[executable, image, brokers, icon, PORT_DIR / "Info.plist"])
    n.build(outputs="macos", rule="phony", inputs=[executable, image, brokers, app_stamp])
    n.newline()
