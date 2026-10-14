"""Ninja rules for the desktop guest builds: Linux on 64-bit ARM
(``ninja linux_arm64``).

The game's data needs 32-bit pointers, recent 64-bit ARM processors have no
32-bit mode, and few distributions have 32-bit ARM libraries. So this build
runs the game as the Android port does (port/android/README.md, "How the port operates"): the
guest image is ILP32 code of the computer's architecture (tools/guest_arch.py;
tools/android_build.py, generate_guest_image), with the Linux desktop's code
paths instead of the app's, and the host is a Linux executable instead of an
app's library:

- build/<name>/halo: the host (port/android/host, with
  port/linux/host/host_main.c as its entry point), with the guest image
  inside it;
- build/<name>/libSDL3.so.0: SDL3, next to the executable, which finds
  it there (an rpath of $ORIGIN), as few distributions have SDL3 yet, and
  its license, SDL3-LICENSE.txt.

Each build is a LinuxGuest in LINUX_GUESTS, generated on a computer of its
architecture. Refer to "64-bit ARM" in port/linux/README.md.
"""

import json
import platform
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from .android_build import (GUEST_ARCH_SCRIPT, SDL_TAG, TOML_DIR, fetch_third_party, generate_guest_image)
from .guest_arch import ARM64_32, GuestArch
from .linux_build import (MBEDTLS_DIR, MINIUPNPC_DEFINES, MINIUPNPC_DIR, SDL_RPATH_LDFLAGS, compile_launcher,
                          miniupnpc_sources, sdl3_rule, updater_defines)
from .ninja_syntax import Writer, escape

# the desktop host's own files (its entry point, SDL's desktop functions)
PORT_DIR = Path("port/linux/host")
ANDROID_DIR = Path("port/android")
LINUX_DIR = Path("port/linux")
# the system's OpenGL ES, EGL and OpenGL headers (libgles-dev, libegl-dev and
# libgl-dev on Debian and Ubuntu, mesa on Arch Linux)
GL_HEADERS = Path("/usr/include")
HOST_LIBRARIES = ["SDL3", "GLESv2", "EGL", "m", "dl", "pthread"]


@dataclass(frozen=True)
class LinuxGuest:
    # the ninja target, the build's folder (build/<name>), the prefix of its
    # rules and the configure.py settings that are its own (<name>_cc, and
    # <name>_gl)
    name: str
    # what its rules' descriptions start with
    label: str
    # what it is, for the build file's comments and messages
    title: str
    # the guest's architecture
    arch: GuestArch
    # the processors (platform.machine()) of the computers that build it
    machines: Tuple[str, ...]


LINUX_GUESTS = (
    LinuxGuest(name="linux_arm64", label="LINUX ARM64", title="64-bit ARM Linux", arch=ARM64_32,
               machines=("aarch64", "arm64")),
)


def native_linux_guest() -> Optional[LinuxGuest]:
    """the desktop guest build of this computer, if there is one"""
    if not sys.platform.startswith("linux"):
        return None
    machine = platform.machine().lower()
    return next((target for target in LINUX_GUESTS if machine in target.machines), None)


def linux_guest_configure_inputs() -> List[Path]:
    return [Path(__file__), GUEST_ARCH_SCRIPT, PORT_DIR]


def generate_linux_guest_builds(n: Writer, sln: Any) -> None:
    for target in LINUX_GUESTS:
        generate_linux_guest_build(n, sln, target)


def _unavailable(n: Writer, target: LinuxGuest, reason: str) -> None:
    """a target of the build's name that stops with the reason it cannot be
    built, rather than none (ninja would then say only that it is unknown)"""
    message = f"{target.title} build ({target.name}) unavailable: {reason}"
    print(message, file=sys.stderr)
    n.comment(message)
    n.rule(
        name=f"{target.name}_unavailable",
        command=escape(f"echo {shlex.quote(message)} >&2 && exit 1"),
        description=f"{target.label} UNAVAILABLE",
    )
    n.build(outputs=target.name, rule=f"{target.name}_unavailable")
    n.newline()


def generate_linux_guest_build(n: Writer, sln: Any, target: LinuxGuest) -> None:
    config_path = LINUX_DIR / "port.json"
    if not config_path.is_file() or not PORT_DIR.is_dir():
        return
    if native_linux_guest() != target:
        n.comment(f"{target.title} build ({target.name}): only on a {target.title} computer")
        return
    prefix, label = target.name, target.label
    build = Path("build") / target.name
    third_party = build / "third_party"
    sdl_dir = third_party / "SDL3"
    cc = getattr(sln, f"{prefix}_cc", None) or "clang"
    # the renderer: OpenGL ES 3 (HALO_GLES, as on Android), or desktop OpenGL
    # 4.5 (--linux-arm64-gl=desktop)
    desktop_gl = getattr(sln, f"{prefix}_gl", "gles") == "desktop"
    # (the host's OpenGL ES and EGL, and the guest's entry points: OpenGL ES's,
    # or desktop OpenGL's from glcorearb.h)
    headers = ["GLES3/gl32.h", "GLES2/gl2ext.h", "EGL/egl.h", "KHR/khrplatform.h"]
    if desktop_gl:
        headers.append("GL/glcorearb.h")
    missing = [header for header in headers if not (GL_HEADERS / header).is_file()]
    if missing:
        _unavailable(n, target, f"no {', '.join(missing)} in {GL_HEADERS} (libgles-dev, libegl-dev"
                     + (", libgl-dev" if desktop_gl else "") + ")")
        return
    try:
        fetch_third_party(third_party)
    except (subprocess.CalledProcessError, OSError, RuntimeError) as error:
        _unavailable(n, target, f"cannot fetch musl or SDL3 ({error})")
        return
    config: Dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))
    # the guest's and the host's code paths: the desktop's (no HALO_ANDROID),
    # and the renderer's
    guest_defines = [] if desktop_gl else ["-DHALO_GLES=1"]
    host_defines = ["-DHALO_DESKTOP_GL=1"] if desktop_gl else []

    n.comment(f"{target.title} build (ninja {target.name}); see port/linux/README.md")
    n.variable(f"{prefix}_guest_cc", cc)
    n.variable(f"{prefix}_cc", cc)

    guest = generate_guest_image(
        n, sln, config, arch=target.arch, prefix=prefix, label=label, build=build, third_party=third_party,
        guest_cc=cc, gl_headers=GL_HEADERS, ar="llvm-ar", ld="ld.lld", builtins_cc=f"${prefix}_cc",
        environment="gnu", platform_defines=guest_defines, extra_runtime=[PORT_DIR / "guest_desktop.c"],
        extra_imports=[PORT_DIR / "host_imports.list"],
        updater_cflags=updater_defines(getattr(sln, "port_release", False)), desktop_gl=desktop_gl)

    # ---------- SDL3, built from the same source as the guest's headers

    sdl_build = build / "sdl3-build"
    libsdl = sdl_build / "libSDL3.so.0"
    sdl3_rule(n, sln, name=f"{prefix}_sdl3", description=f"{label} SDL3 {SDL_TAG}", source=sdl_dir,
              sdl_build=sdl_build, log_dir=build, cc=f"${prefix}_cc")
    n.build(outputs=libsdl, rule=f"{prefix}_sdl3", implicit=[sdl_dir / "CMakeLists.txt"])

    # ---------- the host

    host_obj_dir = build / "host" / "obj"
    host_cc = f"{prefix}_host_cc"
    n.rule(
        name=host_cc,
        command=f"{compile_launcher(sln)}${prefix}_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description=f"{label} HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    # (frame pointers: the sampler and the crash report walk the host's
    # frames too, host_arch.c)
    host_cflags = " ".join([
        "-O2", "-g", "-fno-omit-frame-pointer", "-fPIC", "-Wall", "-Wno-unused-function", "-D_GNU_SOURCE",
        f"-I{ANDROID_DIR}/include", f"-I{ANDROID_DIR}/host", f"-I{sdl_dir}/include", f"-I{LINUX_DIR}/src",
        f"-I{TOML_DIR}", *host_defines,
    ])
    # (the app's own: its entry point, and the touch controls through Java)
    android_only = {"host_main.c", "host_touch.c"}
    host_sources = [source for source in sorted((ANDROID_DIR / "host").glob("*.c")) if source.name not in android_only]
    host_sources += [
        PORT_DIR / "host_main.c", PORT_DIR / "host_desktop.c",
        LINUX_DIR / "src" / "posix_files.c", LINUX_DIR / "src" / "posix_net.c",
        LINUX_DIR / "src" / "posix_trace_marker.c",
        TOML_DIR / "tomlc17.c",
    ]
    host_objects: List[Path] = []
    for source in host_sources:
        obj = host_obj_dir / (source.name + ".o")
        n.build(outputs=obj, rule=host_cc, inputs=source, variables={"cflags": host_cflags})
        host_objects.append(obj)
    miniupnpc_cflags = " ".join([host_cflags, f"-I{MINIUPNPC_DIR / 'include'}", f"-I{MINIUPNPC_DIR / 'src'}",
                                 *MINIUPNPC_DEFINES])
    for source in [LINUX_DIR / "src" / "posix_upnp.c", *miniupnpc_sources()]:
        obj = host_obj_dir / ("miniupnpc_" + source.name + ".o" if source.parent.parent == MINIUPNPC_DIR
                              else source.name + ".o")
        n.build(outputs=obj, rule=host_cc, inputs=source,
                variables={"cflags": miniupnpc_cflags + (" -w" if source.name != "posix_upnp.c" else "")})
        host_objects.append(obj)
    # the self-updater's download (posix_update.c) and internet play's DTLS
    # with browsers (posix_dtls.c), with port/third_party/mbedtls
    mbedtls_cflags = " ".join([host_cflags, f"-I{MBEDTLS_DIR / 'include'}"])
    for name in ("posix_update.c", "posix_dtls.c"):
        n.build(outputs=host_obj_dir / (name + ".o"), rule=host_cc,
                inputs=LINUX_DIR / "src" / name, variables={"cflags": mbedtls_cflags})
        host_objects.append(host_obj_dir / (name + ".o"))
    for source in sorted((MBEDTLS_DIR / "library").glob("*.c")):
        obj = host_obj_dir / "mbedtls" / (source.name + ".o")
        n.build(outputs=obj, rule=host_cc, inputs=source,
                variables={"cflags": f"{mbedtls_cflags} -I{MBEDTLS_DIR / 'library'} -w"})
        host_objects.append(obj)
    table_obj = host_obj_dir / "host_import_table.c.o"
    n.build(outputs=table_obj, rule=host_cc, inputs=guest["host_table_c"],
            variables={"cflags": host_cflags})
    host_objects.append(table_obj)
    # the guest image, inside the executable (host_main.c)
    image_obj = host_obj_dir / "guest_image.S.o"
    n.build(outputs=image_obj, rule=host_cc, inputs=PORT_DIR / "guest_image.S",
            implicit=[guest["image"]], variables={"cflags": f'-DGUEST_IMAGE=\\"{guest["image"]}\\"'})
    host_objects.append(image_obj)

    output = build / "halo"
    staged_sdl = build / "libSDL3.so.0"
    n.rule(
        name=f"{prefix}_host_link",
        command=(f"${prefix}_cc -o $out $in -L{sdl_build} "
                 + " ".join(f"-l{lib}" for lib in HOST_LIBRARIES)
                 # (the SDL beside the executable, as the portable x86 build's)
                 + " " + " ".join(SDL_RPATH_LDFLAGS) + " -Wl,--no-undefined"
                 # (posix_trace_marker.c's, which the GPU driver's calls must reach)
                 + "".join(f" -Wl,--export-dynamic-symbol={name}" for name in ("open", "open64", "openat", "openat64"))),
        description=f"{label} LINK $out",
    )
    n.build(outputs=output, rule=f"{prefix}_host_link", inputs=host_objects, implicit=[libsdl])
    n.rule(name=f"{prefix}_copy", command="cp -L $in $out", description=f"{label} STAGE $out")
    n.build(outputs=staged_sdl, rule=f"{prefix}_copy", inputs=libsdl)
    # (SDL's license, which the release carries beside it)
    sdl_license = build / "SDL3-LICENSE.txt"
    n.build(outputs=sdl_license, rule=f"{prefix}_copy", inputs=sdl_dir / "LICENSE.txt")
    # internet play's MQTT brokers, a file beside the game (network.brokers_file)
    brokers = build / "brokers.txt"
    n.build(outputs=brokers, rule=f"{prefix}_copy", inputs=Path("port/assets/network/brokers.txt"))
    n.build(outputs=target.name, rule="phony", inputs=[output, staged_sdl, sdl_license, brokers])
    n.newline()
