"""Ninja rules for the macOS build (``ninja macos``).

The macOS port (port/macos/README.md) runs the game the way the Android port
does: as ILP32 code - 32-bit pointers, as the game's data formats require -
inside an ordinary 64-bit process. On macOS that code is x32 (x86-64
instructions with 32-bit pointers) and the process is an x86-64 executable,
which Rosetta 2 runs on Apple silicon: arm64 macOS processes cannot map
anything below 4 GB, x86-64 ones linked with a small __PAGEZERO can. This
graph builds

- the guest image, build/macos/Halo/halo_guest.elf: the game sources, the
  platform layer shared with the Linux port (port/linux/src), the Android
  port's guest runtime (port/android/guest/runtime) with the desktop's SDL
  functions (port/macos/guest/runtime) and a subset of musl as its C
  library, compiled by clang for x86_64-linux-gnux32 and linked by ld.lld at
  a fixed address below 2 GB;
- the host executable, build/macos/Halo/halo: the loader and the services
  the guest calls (port/macos/host), over SDL3;
- SDL3 itself, for x86-64;

and stages them with ANGLE (OpenGL ES over Metal: libEGL.dylib and
libGLESv2.dylib, taken from an installed Chromium-based application unless
--macos-angle names a folder holding them).

Requirements: Xcode's clang, ninja, cmake, and ld.lld with llvm-ar (the
``ziglang`` Python package's are found automatically). Like the Linux and
Android builds, this is independent of the byte-matching graph.
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

from .android_build import (GUEST_CODE_FLAGS, MUSL_URL, MUSL_VERSION, SDL_TAG, SDL_URL, VARIADIC_PROTOTYPE_FILES,
                            KCP_DIR, TOML_DIR, _musl_sources, _quote)
from .linux_build import (MINIUPNPC_DEFINES, MINIUPNPC_DIR, MUSL_MATH_DIR, XDK_INCLUDE, compile_launcher,
                          miniupnpc_sources, musl_math_sources, xdk_headers)
from .ninja_syntax import Writer

PORT_DIR = Path("port/macos")
ANDROID_DIR = Path("port/android")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/macos")
THIRD_PARTY = BUILD / "third_party"
MUSL_DIR = THIRD_PARTY / f"musl-{MUSL_VERSION}"
MUSL_SHA256 = "a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4"
SDL_DIR = THIRD_PARTY / "SDL3"
SDL_COMMIT = "fa2c02bb6e21974a89ea9824bc53c9932abe5f9c"  # release-3.4.16
KHRONOS_DIR = THIRD_PARTY / "khronos"
KHRONOS_OPENGL = "https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/api"
KHRONOS_EGL = "https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/main/api"
STAGE = BUILD / "Halo"
MACOS_MINIMUM = "14.4"

# the guest image's address: below 2 GB, where x86-64 code can use
# sign-extended 32-bit absolute addresses; the host reserves the region
# above it (port/macos/host/host_memory.c)
GUEST_IMAGE_BASE = 0x20000000

GUEST_ABI_FLAGS = [
    "--target=x86_64-linux-gnux32",
    "-DHALO_MACOS=1",
    f"-DHALO_GUEST_IMAGE_BASE=0x{GUEST_IMAGE_BASE:08x}u",
    # what Rosetta 2 translates
    "-march=x86-64-v2",
    # as the MSVC runtime, and as the Android guest's musl headers say
    "-mlong-double-64",
    "-nostdinc",
    "-fshort-wchar",
    "-fno-stack-protector",
    "-fno-unwind-tables",
    "-fno-asynchronous-unwind-tables",
    "-femulated-tls",
    "-fno-pic",
    "-fno-pie",
    # no fused multiply-add, as on x86 (the game's debug assertions trip
    # on the different rounding)
    "-ffp-contract=off",
    "-O2",
]

HOST_FRAMEWORKS = ["Cocoa", "Metal", "QuartzCore", "IOKit"]


def _find_tool(name: str) -> Optional[str]:
    """ld.lld and llvm-ar: from PATH, else from the ziglang package"""
    found = shutil.which(name)
    if found:
        return found
    try:
        import ziglang  # type: ignore
    except ImportError:
        return None
    zig = Path(ziglang.__file__).parent / "zig"
    if not zig.is_file():
        return None
    return f"{_quote(zig)} {'ld.lld' if name == 'ld.lld' else 'ar'}"


def _find_angle(sln: Any) -> Optional[Path]:
    """a folder holding libEGL.dylib and libGLESv2.dylib with x86-64 code"""
    wanted = getattr(sln, "macos_angle", None)
    candidates = [Path(wanted)] if wanted else []
    home = Path.home()
    candidates += [
        home / "Library/Application Support/Steam/Steam.AppBundle/Steam/Contents/MacOS/Frameworks/"
        "Chromium Embedded Framework.framework/Versions/A/Libraries",
        Path("/Applications/Google Chrome.app/Contents/Frameworks/Google Chrome Framework.framework/"
             "Versions/Current/Libraries"),
    ]
    for folder in candidates:
        gles = folder / "libGLESv2.dylib"
        egl = folder / "libEGL.dylib"
        if not gles.is_file() or not egl.is_file():
            continue
        try:
            archs = subprocess.run(["lipo", "-archs", str(gles)], capture_output=True, text=True, check=True).stdout
        except (OSError, subprocess.CalledProcessError):
            continue
        if "x86_64" in archs.split():
            return folder
    return None


def fetch_third_party() -> None:
    """Download musl, SDL3 and the Khronos headers (configure time, once)."""
    import hashlib
    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    if not MUSL_DIR.is_dir():
        print(f"Downloading {MUSL_URL}")
        archive = THIRD_PARTY / f"musl-{MUSL_VERSION}.tar.gz"
        subprocess.run(["curl", "-sSfL", "-o", str(archive), MUSL_URL], check=True)
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        if digest != MUSL_SHA256:
            archive.unlink()
            raise OSError(f"{MUSL_URL}: unexpected SHA-256 {digest}")
        subprocess.run(["tar", "xzf", archive.name], cwd=THIRD_PARTY, check=True)
        archive.unlink()
    if not SDL_DIR.is_dir():
        print(f"Cloning SDL3 {SDL_TAG}")
        subprocess.run(["git", "clone", "-q", "--depth", "1", "--branch", SDL_TAG, SDL_URL, str(SDL_DIR)], check=True)
    commit = subprocess.run(["git", "-C", str(SDL_DIR), "rev-parse", "HEAD"], capture_output=True, text=True).stdout
    if commit.strip() != SDL_COMMIT:
        raise OSError(f"{SDL_DIR} is at {commit.strip()}, not {SDL_TAG} ({SDL_COMMIT})")
    headers = {
        "GLES3/gl32.h": KHRONOS_OPENGL, "GLES3/gl3platform.h": KHRONOS_OPENGL, "GLES2/gl2ext.h": KHRONOS_OPENGL,
        "GLES2/gl2.h": KHRONOS_OPENGL, "GLES2/gl2platform.h": KHRONOS_OPENGL, "KHR/khrplatform.h": KHRONOS_EGL,
    }
    for name, base in headers.items():
        target = KHRONOS_DIR / name
        if not target.is_file():
            target.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(["curl", "-sSfL", "-o", str(target), f"{base}/{name}"], check=True)


def _stage_angle(folder: Path) -> Path:
    """the x86-64 parts of ANGLE, copied to build/macos/third_party/angle
    (the originals' paths have spaces, which ninja commands do not quote)"""
    target = THIRD_PARTY / "angle"
    target.mkdir(parents=True, exist_ok=True)
    for name in ("libEGL.dylib", "libGLESv2.dylib"):
        source = folder / name
        staged = target / name
        if staged.is_file() and staged.stat().st_mtime >= source.stat().st_mtime:
            continue
        subprocess.run(["lipo", str(source), "-thin", "x86_64", "-output", str(staged)], check=True)
    return target


def macos_configure_inputs() -> List[Path]:
    return [Path(__file__), PORT_DIR / "host", PORT_DIR / "guest" / "runtime", ANDROID_DIR / "guest" / "runtime",
            LINUX_DIR / "src"]


def generate_macos_build(n: Writer, sln: Any) -> None:
    config_path = LINUX_DIR / "port.json"
    if sys.platform != "darwin" or not config_path.is_file() or not (PORT_DIR / "host").is_dir():
        return
    lld = _find_tool("ld.lld")
    ar = _find_tool("llvm-ar")
    if not lld or not ar:
        n.comment("macOS build: no ld.lld/llvm-ar found (pip install ziglang)")
        return
    angle = _find_angle(sln)
    if not angle:
        n.comment("macOS build: no x86-64 ANGLE (libEGL.dylib, libGLESv2.dylib) found; pass --macos-angle")
        return
    try:
        fetch_third_party()
        angle = _stage_angle(angle)
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"macOS build disabled: cannot fetch musl/SDL3/Khronos headers ({error})", file=sys.stderr)
        return
    import json
    config: Dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))

    guest_cc = "clang"
    guest_dir = BUILD / "guest"
    obj_dir = guest_dir / "obj"
    gen_dir = guest_dir / "gen"
    libc_include = guest_dir / "libc_include"
    libc_internal = guest_dir / "libc_internal"
    arch = PORT_DIR / "guest" / "libc" / "arch" / "x32"
    semantics_header = Path("build/linux/halo_msvc_semantics.h")
    platform_semantics_header = Path("build/linux/platform_msvc_semantics.h")
    prefix_header = LINUX_DIR / "include" / "halo_linux_prefix.h"
    image = STAGE / "halo_guest.elf"
    sdl_build = BUILD / "sdl3-build"
    libsdl = sdl_build / "libSDL3.0.dylib"
    host_executable = STAGE / "halo"
    python = "$python"

    n.comment("macOS build (ninja macos); see port/macos/README.md")
    n.variable("macos_guest_cc", guest_cc)
    n.variable("macos_lld", lld)
    n.variable("macos_ar", ar)

    # ---------- generated headers and sources

    alltypes = libc_include / "bits" / "alltypes.h"
    syscall_h = libc_include / "bits" / "syscall.h"
    version_h = libc_internal / "version.h"
    n.rule(
        name="macos_alltypes",
        command=f"mkdir -p $$(dirname $out) && sed -f {MUSL_DIR}/tools/mkalltypes.sed $in > $out",
        description="MACOS MUSL $out",
    )
    n.build(outputs=alltypes, rule="macos_alltypes",
            inputs=[arch / "bits" / "alltypes.h.in", MUSL_DIR / "include" / "alltypes.h.in"])
    n.rule(
        name="macos_syscall_h",
        command="mkdir -p $$(dirname $out) && cp $in $out && sed -n -e s/__NR_/SYS_/p < $in >> $out",
        description="MACOS MUSL $out",
    )
    n.build(outputs=syscall_h, rule="macos_syscall_h", inputs=arch / "bits" / "syscall.h.in")
    n.rule(
        name="macos_version_h",
        command=f"mkdir -p $$(dirname $out) && echo '#define VERSION \"{MUSL_VERSION}\"' > $out",
        description="MACOS MUSL $out",
    )
    n.build(outputs=version_h, rule="macos_version_h")

    guest_gl_c = gen_dir / "guest_gl.c"
    gl_imports = gen_dir / "gl_imports.list"
    n.rule(
        name="macos_gl_stubs",
        command=(f"{python} tools/android_gl_stubs.py --integer-registers 6 {LINUX_DIR}/src/gl.h "
                 f"{KHRONOS_DIR}/GLES3/gl32.h {KHRONOS_DIR}/GLES2/gl2ext.h {guest_gl_c} {gl_imports}"),
        description="MACOS GL STUBS",
    )
    n.build(outputs=[guest_gl_c, gl_imports], rule="macos_gl_stubs",
            implicit=[Path("tools/android_gl_stubs.py"), LINUX_DIR / "src" / "gl.h"])

    guest_posix_c = gen_dir / "guest_posix.c"
    posix_imports = gen_dir / "posix_imports.list"
    n.rule(
        name="macos_posix_stubs",
        command=f"{python} tools/android_posix_stubs.py {LINUX_DIR}/src/posix.h {guest_posix_c} {posix_imports}",
        description="MACOS POSIX STUBS",
    )
    n.build(outputs=[guest_posix_c, posix_imports], rule="macos_posix_stubs",
            implicit=[Path("tools/android_posix_stubs.py"), LINUX_DIR / "src" / "posix.h"])

    imports_s = gen_dir / "imports.s"
    host_table_c = BUILD / "host" / "host_import_table.c"
    host_imports_list = PORT_DIR / "host_imports.list"
    n.rule(
        name="macos_imports",
        command=f"{python} tools/android_imports.py --arch x86_64 --host-table {host_table_c} {imports_s} $in",
        description="MACOS IMPORTS",
    )
    n.build(outputs=[imports_s, host_table_c], rule="macos_imports",
            inputs=[host_imports_list, posix_imports, gl_imports],
            implicit=[Path("tools/android_imports.py")])

    generated_headers = [*xdk_headers(), alltypes, syscall_h, version_h, semantics_header, platform_semantics_header]

    # ---------- guest compilation: C -> x32 ELF object

    n.rule(
        name="macos_guest_cc",
        command=f"{compile_launcher(sln)}$macos_guest_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    n.rule(
        name="macos_guest_as",
        command="$macos_guest_cc --target=x86_64-linux-gnux32 -c $in -o $out",
        description="MACOS AS $out",
    )

    libc_includes = [
        f"-isystem {libc_include}", f"-isystem {arch}", f"-isystem {MUSL_DIR}/arch/generic",
        f"-isystem {MUSL_DIR}/include",
    ]
    guest_abi = " ".join(GUEST_ABI_FLAGS + (["-DHALO_RELEASE"] if getattr(sln, "port_release", False) else []))
    guest_code = " ".join(GUEST_CODE_FLAGS)
    tool_implicit = list(generated_headers)

    def guest_object(source: Path, cflags: str, prefix: str = "") -> Path:
        obj = obj_dir / prefix / Path(str(source).lstrip("/")).with_suffix(".o")
        if str(source).startswith(str(BUILD)):
            obj = obj_dir / prefix / source.relative_to(BUILD).with_suffix(".o")
        n.build(outputs=obj, rule="macos_guest_cc", inputs=source, implicit=tool_implicit,
                variables={"cflags": cflags})
        return obj

    # musl
    musl_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-w",
        f"-I{arch}", f"-I{MUSL_DIR}/arch/generic", f"-I{libc_internal}",
        f"-I{MUSL_DIR}/src/include", f"-I{MUSL_DIR}/src/internal", f"-I{libc_include}", f"-I{MUSL_DIR}/include",
    ])
    musl_objects = []
    for source in _musl_sources_in(MUSL_DIR):
        musl_objects.append(guest_object(source, musl_cflags, "musl"))
    libguestc = guest_dir / "libguestc.a"
    n.rule(
        name="macos_ar",
        command="rm -f $out && $macos_ar rcs $out @$out.rsp",
        description="MACOS AR $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=libguestc, rule="macos_ar", inputs=musl_objects)

    # the game
    objects: List[Path] = []
    excluded = set(config.get("exclude_sources", []))
    game_flags = [
        "-std=gnu89", "-D__STRICT_ANSI__", "-w",
        "-Wno-error=incompatible-pointer-types",
        "-Wno-error=incompatible-function-pointer-types",
        "-Wno-error=int-conversion",
        "-Wno-error=implicit-function-declaration",
        "-Wno-error=implicit-int",
        "-Wno-error=return-type",
    ]
    for proj in sln.projects:
        if proj.name not in config["projects"]:
            continue
        options = proj.options
        defines = " ".join(f"-D{d}" for d in options.get("defines") or [])
        includes = " ".join(
            f"-I{_quote(d)}" for d in options.get("include_dirs") or [] if Path(d) != Path("xbox/include")
        )
        game_cflags = " ".join([
            guest_abi, guest_code, " ".join(game_flags),
            f"-include {prefix_header}", f"-include {semantics_header}", defines,
            f"-I{LINUX_DIR}/include", includes, *libc_includes, f"-idirafter {XDK_INCLUDE}",
        ])
        for obj in proj.objects:
            name = str(obj.file_path).replace(os.sep, "/")
            if obj.status.name == "Missing" or name in excluded or obj.file_path.suffix.lower() != ".c":
                continue
            cflags = game_cflags
            if name in VARIADIC_PROTOTYPE_FILES:
                cflags += f" -include {ANDROID_DIR}/include/halo_android_variadic_prototypes.h"
            objects.append(guest_object(obj.file_path, cflags))
        for source in sorted(Path(config["game_sources"]).glob("*.c")):
            objects.append(guest_object(source, game_cflags))

    # the platform layer shared with Linux, and the guest runtime
    runtime_dirs = f"-I{ANDROID_DIR}/guest/runtime -I{PORT_DIR}/guest/runtime -I{ANDROID_DIR}/include"
    platform_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE", "-DHALO_LINUX_PLATFORM_LAYER", "-w",
        f"-include {prefix_header}", f"-include {platform_semantics_header}",
        f"-I{LINUX_DIR}/src", f"-I{LINUX_DIR}/include", runtime_dirs,
        f"-I{TOML_DIR}", f"-I{KCP_DIR}", "-Isource -Isource/cseries",
        f"-I{SDL_DIR}/include", f"-I{KHRONOS_DIR}", *libc_includes, f"-idirafter {XDK_INCLUDE}",
    ])
    # replaced by the guest runtime (memory_watch.c) or not part of the
    # macOS port (updater.c: no self-updating; posix_*.c: in the host)
    guest_host_only = {"memory_watch.c", "updater.c"}
    for source in sorted((LINUX_DIR / "src").glob("*.c")):
        if source.name.startswith("posix_") or source.name in guest_host_only:
            continue
        objects.append(guest_object(source, platform_cflags))
    objects.append(guest_object(TOML_DIR / "tomlc17.c", platform_cflags))
    objects.append(guest_object(KCP_DIR / "ikcp.c", platform_cflags))
    musl_math_cflags = " ".join([
        guest_abi, "-std=gnu11", "-w", *libc_includes, f"-I{MUSL_MATH_DIR}/include",
        f"-include {MUSL_MATH_DIR}/include/libm.h",
    ])
    for source in musl_math_sources():
        objects.append(guest_object(source, musl_math_cflags))
    runtime_internal_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-D_GNU_SOURCE",
        runtime_dirs, f"-I{arch}", f"-I{MUSL_DIR}/arch/generic", f"-I{libc_internal}",
        f"-I{MUSL_DIR}/src/include", f"-I{MUSL_DIR}/src/internal", f"-I{libc_include}", f"-I{MUSL_DIR}/include",
    ])
    runtime_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE", runtime_dirs, f"-I{LINUX_DIR}/src",
        f"-I{SDL_DIR}/include", f"-I{KHRONOS_DIR}", *libc_includes,
    ])
    runtime_sources = sorted((ANDROID_DIR / "guest" / "runtime").glob("*.c")) + \
        sorted((PORT_DIR / "guest" / "runtime").glob("*.c"))
    for source in runtime_sources:
        if source.name in ("guest_thread.c", "guest_start.c"):
            objects.append(guest_object(source, runtime_internal_cflags))
        elif source.name == "guest_memory_watch.c":
            objects.append(guest_object(source, platform_cflags))
        else:
            objects.append(guest_object(source, runtime_cflags))
    objects.append(guest_object(guest_gl_c, runtime_cflags))
    objects.append(guest_object(guest_posix_c, runtime_cflags))
    imports_o = obj_dir / "gen" / "imports.o"
    n.build(outputs=imports_o, rule="macos_guest_as", inputs=imports_s)
    objects.append(imports_o)

    # ---------- the guest image

    linker_script = PORT_DIR / "guest" / "guest.ld"
    n.rule(
        name="macos_guest_link",
        command=(f"mkdir -p {STAGE} && $macos_lld -m elf32_x86_64 -static -nostdlib -T {linker_script} "
                 f"-Map {BUILD}/halo_guest.map -o $out @$out.rsp {libguestc}"),
        description="MACOS LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=image, rule="macos_guest_link", inputs=objects, implicit=[libguestc, linker_script])

    # ---------- SDL3 (x86-64)

    n.rule(
        name="macos_sdl3",
        command=(f"cmake -S {SDL_DIR} -B {sdl_build} -G Ninja -DCMAKE_OSX_ARCHITECTURES=x86_64 "
                 f"-DCMAKE_OSX_DEPLOYMENT_TARGET={MACOS_MINIMUM} -DCMAKE_BUILD_TYPE=Release "
                 f"-DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF "
                 f"> {BUILD}/sdl3-configure.log && ninja -C {sdl_build} > {BUILD}/sdl3-build.log"),
        description="MACOS SDL3",
        pool="console",
    )
    n.build(outputs=libsdl, rule="macos_sdl3", implicit=[SDL_DIR / "CMakeLists.txt"])

    # ---------- the host executable

    host_objects: List[Path] = []
    host_obj_dir = BUILD / "host" / "obj"
    n.rule(
        name="macos_host_cc",
        command="clang -arch x86_64 -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    host_cflags = " ".join([
        f"-mmacosx-version-min={MACOS_MINIMUM}", "-O2", "-g", "-Wall", "-Wno-unused-function",
        "-Wno-deprecated-declarations", f"-DHALO_GUEST_IMAGE_BASE=0x{GUEST_IMAGE_BASE:08x}u",
        f"-I{ANDROID_DIR}/include", f"-I{PORT_DIR}/host", f"-I{SDL_DIR}/include", f"-I{LINUX_DIR}/src",
        f"-I{TOML_DIR}", f"-I{KHRONOS_DIR}",
    ])
    host_sources = sorted((PORT_DIR / "host").glob("*.c")) + [
        LINUX_DIR / "src" / "posix_files.c", LINUX_DIR / "src" / "posix_net.c",
    ]
    for source in host_sources:
        obj = host_obj_dir / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": host_cflags},
                implicit=[libsdl])
        host_objects.append(obj)
    miniupnpc_cflags = " ".join([host_cflags, f"-I{MINIUPNPC_DIR / 'include'}", f"-I{MINIUPNPC_DIR / 'src'}",
                                 *MINIUPNPC_DEFINES])
    for source in [LINUX_DIR / "src" / "posix_upnp.c", *miniupnpc_sources()]:
        obj = host_obj_dir / ("miniupnpc_" + source.name + ".o" if source.parent.parent == MINIUPNPC_DIR
                              else source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source,
                variables={"cflags": miniupnpc_cflags + (" -w" if source.name != "posix_upnp.c" else "")})
        host_objects.append(obj)
    table_obj = host_obj_dir / "host_import_table.c.o"
    n.build(outputs=table_obj, rule="macos_host_cc", inputs=host_table_c, variables={"cflags": host_cflags})
    host_objects.append(table_obj)
    n.rule(
        name="macos_host_link",
        # a 64 KB __PAGEZERO leaves the low 4 GB to the guest
        command=(f"mkdir -p {STAGE} && clang -arch x86_64 -mmacosx-version-min={MACOS_MINIMUM} "
                 f"-Wl,-pagezero_size,0x10000 -o $out $in -L{sdl_build} -lSDL3 -Wl,-rpath,@executable_path "
                 + " ".join(f"-framework {f}" for f in HOST_FRAMEWORKS)),
        description="MACOS HOST LINK $out",
    )
    n.build(outputs=host_executable, rule="macos_host_link", inputs=host_objects, implicit=[libsdl])

    # ---------- staging: SDL3 and ANGLE next to the executable

    staged_sdl = STAGE / "libSDL3.0.dylib"
    n.rule(name="macos_copy", command="mkdir -p $$(dirname $out) && cp $in $out", description="MACOS STAGE $out")
    n.build(outputs=staged_sdl, rule="macos_copy", inputs=libsdl)
    staged_angle = []
    for name in ("libEGL.dylib", "libGLESv2.dylib"):
        staged = STAGE / name
        n.build(outputs=staged, rule="macos_copy", inputs=angle / name)
        staged_angle.append(staged)
    n.build(outputs="macos", rule="phony", inputs=[host_executable, image, staged_sdl, *staged_angle])
    n.newline()


def _musl_sources_in(musl_dir: Path) -> List[Path]:
    """android_build._musl_sources, for this build's copy of musl"""
    from . import android_build
    saved = android_build.MUSL_DIR
    android_build.MUSL_DIR = musl_dir
    try:
        return _musl_sources()
    finally:
        android_build.MUSL_DIR = saved
