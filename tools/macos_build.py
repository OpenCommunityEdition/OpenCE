"""Ninja rules for the macOS build (``ninja macos``).

The macOS port (port/macos/README.md) is the Android port's design
(tools/android_build.py) with x86 in place of AArch64. It runs the game as
x32 code - x86-64 instructions with 32-bit pointers, as the game's data
formats require - inside an ordinary 64-bit x86-64 app, which runs under
Rosetta 2 on Apple silicon and natively on Intel Macs. This graph builds

- the guest image, build/macos/halo_guest.elf: the game sources, the
  platform layer shared with the Linux port (port/linux/src) and the guest
  runtime shared with Android (port/android/guest/runtime) with a subset of
  musl as its C library (port/macos/guest/libc), all compiled by clang for
  x86_64-linux-muslx32 straight to ELF objects and linked by ld.lld at a
  fixed address below 2 GB;
- SDL3, built from source with CMake as a static x86_64 library;
- the host executable, build/macos/halo, an x86_64 Mach-O built by Apple's
  clang: the loader and the services the guest calls (port/macos/host), over
  SDL3 and OpenGL;

and puts them in an app bundle, build/macos/Halo.app, signed ad hoc.

It is generated only on a Mac with a clang that has the x32 target and an
ld.lld (Homebrew's llvm and lld packages): configure.py --macos-guest-cc and
--macos-lld name others.
"""

import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, List, Optional

from .android_build import (EXPAT_DIR, EXPAT_SOURCES, GUEST_CODE_FLAGS, KCP_DIR, MONOCYPHER_DIR, MUSL_VERSION,
                            SDL_TAG, TOML_DIR, VARIADIC_PROTOTYPE_FILES, ZLIB_DEFINES, ZLIB_DIR, ZLIB_SOURCES,
                            _musl_sources, fetch_musl_and_sdl)
from .embed_assets import hud_assets_build, hud_configure_inputs
from .linux_build import (LINUX_PROFILE, MINIUPNPC_DEFINES, MINIUPNPC_DIR, MUSL_MATH_DIR, XDK_INCLUDE,
                          compile_launcher, game_defines_and_includes, game_sources, miniupnpc_sources,
                          musl_math_sources, pgo_mode, pgo_profile, profile_use_flags, updater_defines,
                          xdk_headers)
from .ninja_syntax import Writer

PORT_DIR = Path("port/macos")
ANDROID_DIR = Path("port/android")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/macos")
THIRD_PARTY = BUILD / "third_party"
MUSL_DIR = THIRD_PARTY / f"musl-{MUSL_VERSION}"
SDL_DIR = THIRD_PARTY / "SDL3"
# The OpenGL ES headers the guest's renderer is compiled against (its GLES
# code path, port/linux/src/gl.h) and the GL stub generator reads; Android
# takes them from the NDK. Pinned to a commit of Khronos's registries and
# checked against these hashes.
KHRONOS_DIR = THIRD_PARTY / "khronos"
KHRONOS_GL_URL = ("https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/"
                  "6af574a14089ccfee87efe230ebcdd8742859813/api")
KHRONOS_EGL_URL = ("https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/"
                   "db3425b8246136faccb5e2782b5694960bd6edf1/api")
KHRONOS_HEADERS = (
    (KHRONOS_GL_URL, "GLES2/gl2.h", "ba5e8e1755642efeb2d0f34b4cfe261cdbf6471e7763a40c7de10dff4c3c4921"),
    (KHRONOS_GL_URL, "GLES2/gl2ext.h", "9afc725e9dda7c8b476e7337e75b22fa660ed462c169e203ec28c10e62f91f4c"),
    (KHRONOS_GL_URL, "GLES2/gl2platform.h", "f5da0747540a50be5f44aad264aae45bdf157a192c40f17487dd9a2f99c71b6c"),
    (KHRONOS_GL_URL, "GLES3/gl3.h", "a0e4880142bd059bd4d7446f257920b5020b8bbb0a86eb0149cbd1ea2fcf8cb0"),
    (KHRONOS_GL_URL, "GLES3/gl31.h", "bb17bfde4aeba912d3686a0cc68ac04710ddacbcc5df7354994927747357daf3"),
    (KHRONOS_GL_URL, "GLES3/gl32.h", "f203257863a7de3fce851efa61490907171db82082381d05f86f5d9bd39e8e17"),
    (KHRONOS_GL_URL, "GLES3/gl3platform.h", "a9e060dae5a2b11c5a889b679692b7089a10a7e03ebfbb6cf28217f6e322fb08"),
    (KHRONOS_EGL_URL, "KHR/khrplatform.h", "7b1e01aaa7ad8f6fc34b5c7bdf79ebf5189bb09e2c4d2e79fc5d350623d11e83"),
)
# the oldest macOS the app runs on (SDL3 3.4 supports 10.13; the host's
# small page zero is accepted silently up to 12)
MACOS_MINIMUM = "12.0"
# the bundle's name and the executable's in it (port/macos/Info.plist)
APP_NAME = "Halo.app"
EXECUTABLE_NAME = "halo"

# The guest ABI: x86-64 code with 32-bit pointers (x32) for Linux, which
# clang writes as ELF objects directly. -march: SSE4.2-era processors, which
# every Intel Mac macOS 12 runs on has and Rosetta 2 translates (never
# -march=native, which names an Apple CPU here). long double is double, as
# in MSVC and on Android (the guest's float.h). No position independence:
# the image is linked at a fixed address (guest.ld).
GUEST_ABI_FLAGS = [
    "--target=x86_64-linux-muslx32",
    "-march=x86-64-v2",
    "-mlong-double-64",
    "-fno-pic",
    "-DHALO_MACOS=1",
    "-DHALO_GUEST=1",
    "-DHALO_GLES=1",
    "-nostdinc",
    "-fshort-wchar",
    "-fno-stack-protector",
    "-fno-unwind-tables",
    "-fno-asynchronous-unwind-tables",
    # the guest has no thread pointer register of its own (port/macos/guest/libc/arch/x32/pthread_arch.h)
    "-femulated-tls",
    # no fused multiply-add, as on every port (port/include/halo_math.h)
    "-ffp-contract=off",
    "-O2",
]

# what SDL3's static library needs from the system (Libs in the sdl3.pc it
# writes for this configuration; the two weak ones are newer than some
# systems SDL supports), and OpenGL and CoreFoundation for the host itself
# (host_gl.c, host_main.c)
HOST_FRAMEWORKS = [
    "CoreMedia", "CoreVideo", "Cocoa", "IOKit", "ForceFeedback", "Carbon", "CoreAudio", "AudioToolbox",
    "AVFoundation", "Foundation", "GameController", "Metal", "QuartzCore", "OpenGL", "CoreFoundation",
]
HOST_WEAK_FRAMEWORKS = ["UniformTypeIdentifiers", "CoreHaptics"]
HOST_LIBRARIES = ["pthread", "m"]

HOMEBREW_PREFIXES = (Path("/opt/homebrew"), Path("/usr/local"))
APPLE_SILICON_CMAKE = Path("/opt/homebrew/opt/cmake/bin/cmake")


def _quote(path: Any) -> str:
    text = str(path).replace(os.sep, "/")
    return f'"{text}"' if " " in text else text


def _homebrew_tool(package: str, name: str) -> Optional[str]:
    for prefix in HOMEBREW_PREFIXES:
        candidate = prefix / "opt" / package / "bin" / name
        if candidate.is_file():
            return str(candidate)
    return None


def _guest_cc(sln: Any) -> str:
    """--macos-guest-cc, or Homebrew's clang (Apple's has the target too, but
    Xcode ships no ld.lld and no llvm-ar)"""
    return getattr(sln, "macos_guest_cc", None) or _homebrew_tool("llvm", "clang") or "clang"


def _lld(sln: Any) -> Optional[str]:
    explicit = getattr(sln, "macos_lld", None)
    if explicit:
        return explicit
    return _homebrew_tool("lld", "ld.lld") or _homebrew_tool("llvm", "ld.lld") or shutil.which("ld.lld")


def _llvm_ar(guest_cc: str) -> Optional[str]:
    """the llvm-ar beside the guest's clang, or another"""
    found = shutil.which(guest_cc)
    if found:
        beside = Path(found).parent / "llvm-ar"
        if beside.is_file():
            return str(beside)
    return _homebrew_tool("llvm", "llvm-ar") or shutil.which("llvm-ar")


def _has_x32_target(cc: str) -> bool:
    try:
        return subprocess.run([cc, "--target=x86_64-linux-muslx32", "-fsyntax-only", "-x", "c", os.devnull],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False).returncode == 0
    except OSError:
        return False


def fetch_khronos_headers() -> None:
    """Download the OpenGL ES headers (configure time, once), each checked
    against its hash."""
    for base, name, digest in KHRONOS_HEADERS:
        target = KHRONOS_DIR / name
        if target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == digest:
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        print(f"Downloading {name} from Khronos")
        with tempfile.TemporaryDirectory(dir=THIRD_PARTY) as scratch:
            download = Path(scratch) / Path(name).name
            subprocess.run(["curl", "-sSfL", "-o", str(download), f"{base}/{name}"], check=True)
            actual = hashlib.sha256(download.read_bytes()).hexdigest()
            if actual != digest:
                raise OSError(f"{name}: sha256 {actual}, expected {digest}")
            download.replace(target)


def macos_configure_inputs() -> List[Path]:
    # (none elsewhere: the graph is only generated on a Mac)
    if sys.platform != "darwin":
        return []
    return [Path(__file__), PORT_DIR, PORT_DIR / "host", ANDROID_DIR / "guest" / "runtime", LINUX_DIR / "src",
            *hud_configure_inputs()]


def generate_macos_build(n: Writer, sln: Any) -> None:
    config_path = LINUX_DIR / "port.json"
    if sys.platform != "darwin" or not config_path.is_file() or not (PORT_DIR / "host").is_dir():
        return
    guest_cc = _guest_cc(sln)
    lld = _lld(sln)
    llvm_ar = _llvm_ar(guest_cc)
    # (Apple silicon Homebrew's first, then the PATH's: an Intel Homebrew's
    # in /usr/local would run under Rosetta)
    cmake = str(APPLE_SILICON_CMAKE) if APPLE_SILICON_CMAKE.is_file() else shutil.which("cmake")
    missing = [what for what, found in (("a clang with the x32 target (--macos-guest-cc)", _has_x32_target(guest_cc)),
                                        ("ld.lld (--macos-lld)", lld), ("llvm-ar", llvm_ar), ("cmake", cmake))
               if not found]
    if missing:
        n.comment("macOS build: needs " + ", ".join(missing) + " (brew install llvm lld cmake)")
        return
    try:
        fetch_musl_and_sdl(THIRD_PARTY)
        fetch_khronos_headers()
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"macOS build disabled: cannot fetch musl/SDL3/OpenGL ES headers ({error})", file=sys.stderr)
        return
    import json
    config: Dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))

    host_cc = "/usr/bin/clang"
    guest_dir = BUILD / "guest"
    obj_dir = guest_dir / "obj"
    gen_dir = guest_dir / "gen"
    libc_include = guest_dir / "libc_include"
    libc_internal = guest_dir / "libc_internal"
    gl_include = KHRONOS_DIR
    arch = PORT_DIR / "guest" / "libc" / "arch" / "x32"
    runtime_dir = ANDROID_DIR / "guest" / "runtime"
    semantics_header = Path("build/linux/halo_msvc_semantics.h")
    platform_semantics_header = Path("build/linux/platform_msvc_semantics.h")
    prefix_header = LINUX_DIR / "include" / "halo_linux_prefix.h"
    image = BUILD / "halo_guest.elf"
    sdl_build = BUILD / "sdl3-build"
    libsdl = sdl_build / "libSDL3.a"
    host_dir = BUILD / "host"
    host_gen = host_dir / "gen"
    executable = BUILD / EXECUTABLE_NAME
    python = "$python"

    n.comment("macOS build (ninja macos); see port/macos/README.md")
    n.variable("macos_guest_cc", _quote(guest_cc))
    n.variable("macos_lld", _quote(lld))
    n.variable("macos_ar", _quote(llvm_ar))
    n.variable("macos_host_cc", host_cc)
    n.variable("macos_cmake", _quote(cmake))

    # ---------- generated headers and sources

    alltypes = libc_include / "bits" / "alltypes.h"
    syscall_h = libc_include / "bits" / "syscall.h"
    version_h = libc_internal / "version.h"
    n.rule(
        name="macos_alltypes",
        command=f"sed -f {MUSL_DIR}/tools/mkalltypes.sed $in > $out",
        description="MACOS MUSL $out",
    )
    n.build(outputs=alltypes, rule="macos_alltypes",
            inputs=[arch / "bits" / "alltypes.h.in", MUSL_DIR / "include" / "alltypes.h.in"])
    n.rule(
        name="macos_syscall_h",
        command="cp $in $out && sed -n -e s/__NR_/SYS_/p < $in >> $out",
        description="MACOS MUSL $out",
    )
    n.build(outputs=syscall_h, rule="macos_syscall_h", inputs=arch / "bits" / "syscall.h.in")
    n.rule(
        name="macos_version_h",
        command=f"echo '#define VERSION \"{MUSL_VERSION}\"' > $out",
        description="MACOS MUSL $out",
    )
    n.build(outputs=version_h, rule="macos_version_h")
    # the guest's system call numbers for the host (host_syscall.c), from
    # the same list: AArch64 Linux's, the guest-host protocol, never the
    # numbers of Darwin's <sys/syscall.h>
    guest_syscalls_h = host_gen / "guest_syscall_numbers.h"
    n.rule(
        name="macos_guest_syscalls",
        command=("sed -n -e 's/^#define __NR_\\([a-z0-9_]*\\)[[:space:]]*\\([0-9][0-9]*\\).*/"
                 "#define GUEST_SYS_\\1 \\2/p' < $in > $out"),
        description="MACOS GUEST SYSCALLS $out",
    )
    n.build(outputs=guest_syscalls_h, rule="macos_guest_syscalls", inputs=arch / "bits" / "syscall.h.in")

    guest_gl_c = gen_dir / "guest_gl.c"
    gl_imports = gen_dir / "gl_imports.list"
    n.rule(
        name="macos_gl_stubs",
        # (x86-64 passes six integer arguments in registers, not eight)
        command=(f"{python} tools/android_gl_stubs.py --integer-registers 6 {LINUX_DIR}/src/gl.h "
                 f"{gl_include}/GLES3/gl32.h {gl_include}/GLES2/gl2ext.h {guest_gl_c} {gl_imports}"),
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

    # the Android host's imports, and those only the macOS guest needs
    imports_s = gen_dir / "imports.s"
    host_table_c = host_gen / "host_import_table.c"
    import_lists = [ANDROID_DIR / "host_imports.list"]
    if (PORT_DIR / "host_imports_macos.list").is_file():
        import_lists.append(PORT_DIR / "host_imports_macos.list")
    n.rule(
        name="macos_imports",
        command=f"{python} tools/android_imports.py --arch x86_64 --host-table {host_table_c} {imports_s} $in",
        description="MACOS IMPORTS",
    )
    n.build(outputs=[imports_s, host_table_c], rule="macos_imports",
            inputs=[*import_lists, posix_imports, gl_imports],
            implicit=[Path("tools/android_imports.py")])

    generated_headers = [*xdk_headers(), alltypes, syscall_h, version_h, semantics_header, platform_semantics_header]

    # ---------- guest compilation: C -> ELF object, directly

    n.rule(
        name="macos_guest_cc",
        command=f"{compile_launcher(sln)}$macos_guest_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    n.rule(
        name="macos_guest_as",
        command="$macos_guest_cc --target=x86_64-linux-muslx32 -c $in -o $out",
        description="MACOS AS $out",
    )

    libc_includes = [
        f"-isystem {libc_include}", f"-isystem {arch}", f"-isystem {MUSL_DIR}/arch/generic",
        f"-isystem {MUSL_DIR}/include",
    ]
    guest_abi = " ".join(GUEST_ABI_FLAGS + (["-DHALO_RELEASE"] if getattr(sln, "port_release", False) else []))
    guest_code = " ".join(GUEST_CODE_FLAGS)
    tool_implicit: List[Path] = [*generated_headers]
    # profile-guided optimisation with the Linux build's profile, as Android
    # does: the game and platform code are the same, and functions that
    # differ simply go without
    profile = pgo_profile(sln, LINUX_PROFILE if pgo_mode(sln) == "train" else None, [LINUX_PROFILE], guest_cc)
    profile_flags = " ".join(profile_use_flags(profile))
    if profile:
        tool_implicit.append(profile)

    def guest_object(source: Path, cflags: str, prefix: str = "") -> Path:
        obj = obj_dir / prefix / Path(str(source).lstrip("/")).with_suffix(".o")
        if str(source).startswith(str(BUILD)):
            obj = obj_dir / prefix / source.relative_to(BUILD).with_suffix(".o")
        n.build(outputs=obj, rule="macos_guest_cc", inputs=source, implicit=tool_implicit,
                variables={"cflags": cflags})
        return obj

    # musl (ELF has aliases, so none of the Android guest's src_include)
    musl_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-w",
        f"-I{arch}", f"-I{MUSL_DIR}/arch/generic", f"-I{libc_internal}",
        f"-I{MUSL_DIR}/src/include", f"-I{MUSL_DIR}/src/internal", f"-I{libc_include}", f"-I{MUSL_DIR}/include",
    ])
    musl_objects = [guest_object(source, musl_cflags, "musl") for source in _musl_sources(MUSL_DIR)]
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
    game_flags = [
        "-std=gnu89", "-D__STRICT_ANSI__", "-w",
        "-Wno-error=incompatible-pointer-types",
        "-Wno-error=incompatible-function-pointer-types",
        "-Wno-error=int-conversion",
        "-Wno-error=implicit-function-declaration",
        "-Wno-error=implicit-int",
        "-Wno-error=return-type",
    ]
    game_cflags = " ".join([
        guest_abi, guest_code, " ".join(game_flags), profile_flags,
        f"-include {prefix_header}", f"-include {semantics_header}",
        f"-I{LINUX_DIR}/include",
        f"-iquote {Path(config['game_sources'])}",
        game_defines_and_includes(config), *libc_includes, f"-idirafter {XDK_INCLUDE}",
    ])
    for source in game_sources(config):
        cflags = game_cflags
        if source.as_posix() == "source/main/main.c":
            cflags += " " + updater_defines(getattr(sln, "port_release", False))
        # (x32 passes unprototyped variadic calls correctly, but the
        # prototypes cost nothing, and the two guests stay alike)
        if source.as_posix() in VARIADIC_PROTOTYPE_FILES:
            cflags += f" -include {ANDROID_DIR}/include/halo_android_variadic_prototypes.h"
        objects.append(guest_object(source, cflags))
    for source in sorted(Path(config["game_sources"]).glob("*.c")):
        objects.append(guest_object(source, game_cflags))

    # the platform layer shared with Linux, and the guest runtime
    platform_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE", "-DHALO_LINUX_PLATFORM_LAYER", "-w", profile_flags,
        f"-include {prefix_header}", f"-include {platform_semantics_header}",
        f"-I{LINUX_DIR}/src", f"-I{LINUX_DIR}/include", f"-I{runtime_dir}",
        f"-I{ANDROID_DIR}/include", f"-I{TOML_DIR}", f"-I{EXPAT_DIR}", f"-I{KCP_DIR}", f"-I{MONOCYPHER_DIR}",
        f"-I{ZLIB_DIR}", "-Isource -Isource/cseries",
        f"-I{SDL_DIR}/include", f"-I{gl_include}", *libc_includes, f"-idirafter {XDK_INCLUDE}",
    ])
    guest_host_only = {"memory_watch.c"}  # replaced by guest_memory_watch.c
    for source in sorted((LINUX_DIR / "src").glob("*.c")):
        if source.name.startswith("posix_") or source.name in guest_host_only:
            continue
        objects.append(guest_object(source, platform_cflags))
    # the high-res HUD's textures, the fonts, the menus and SMAA's files
    # (port/assets; port/linux/src/hud_hires.c)
    for source in hud_assets_build(n, "macos", gen_dir / "hud_hires_assets.c"):
        objects.append(guest_object(source, platform_cflags))
    objects.append(guest_object(TOML_DIR / "tomlc17.c", platform_cflags))
    for name in EXPAT_SOURCES:
        objects.append(guest_object(EXPAT_DIR / name, platform_cflags))
    objects.append(guest_object(KCP_DIR / "ikcp.c", platform_cflags))
    for name in ("monocypher.c", "monocypher-ed25519.c"):
        objects.append(guest_object(MONOCYPHER_DIR / name, platform_cflags))
    for name in ZLIB_SOURCES:
        objects.append(guest_object(ZLIB_DIR / name, " ".join([platform_cflags, *ZLIB_DEFINES])))
    # the game's sin, pow and the rest, the same on every port
    # (port/include/halo_math.h)
    musl_math_cflags = " ".join([
        guest_abi, "-std=gnu11", "-w", profile_flags, *libc_includes, f"-I{MUSL_MATH_DIR}/include",
        f"-include {MUSL_MATH_DIR}/include/libm.h",
    ])
    for source in musl_math_sources():
        objects.append(guest_object(source, musl_math_cflags))
    runtime_internal_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-D_GNU_SOURCE",
        f"-I{runtime_dir}", f"-I{ANDROID_DIR}/include",
        f"-I{arch}", f"-I{MUSL_DIR}/arch/generic", f"-I{libc_internal}",
        f"-I{MUSL_DIR}/src/include", f"-I{MUSL_DIR}/src/internal", f"-I{libc_include}", f"-I{MUSL_DIR}/include",
    ])
    runtime_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE",
        f"-I{runtime_dir}", f"-I{ANDROID_DIR}/include", f"-I{LINUX_DIR}/src",
        f"-I{SDL_DIR}/include", f"-I{gl_include}", *libc_includes,
    ])
    for source in sorted(runtime_dir.glob("*.c")):
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
        # (no compiler runtime: Homebrew's LLVM has none for x32, and the
        # image needs none)
        command=(f"$macos_lld -m elf32_x86_64 -static -nostdlib -T {linker_script} "
                 f"-Map $out.map -o $out @$out.rsp {libguestc}"),
        description="MACOS LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=image, rule="macos_guest_link", inputs=objects, implicit=[libguestc, linker_script])

    # ---------- SDL3, static, so that no library of it is mapped where the
    # guest's memory goes (the host claims the low 4 GB first thing)

    n.rule(
        name="macos_sdl3",
        # (without the user's compiler flags, which name Homebrew's arm64
        # libraries, and without pkg-config: SDL needs none on macOS, and a
        # Homebrew one would offer libraries for the wrong architecture)
        command=(f"env -u CPPFLAGS -u CFLAGS -u CXXFLAGS -u OBJCFLAGS -u LDFLAGS "
                 f"$macos_cmake -S {SDL_DIR} -B {sdl_build} -G Ninja -DCMAKE_BUILD_TYPE=Release "
                 f"-DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON "
                 f"-DCMAKE_C_COMPILER={host_cc} -DCMAKE_OBJC_COMPILER={host_cc} "
                 f"-DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET={MACOS_MINIMUM} "
                 f"-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF "
                 f"> {BUILD}/sdl3-configure.log && "
                 f"env -u CPPFLAGS -u CFLAGS -u CXXFLAGS -u OBJCFLAGS -u LDFLAGS "
                 f"$macos_cmake --build {sdl_build} > {BUILD}/sdl3-build.log"),
        description="MACOS SDL3",
        pool="console",
    )
    n.build(outputs=libsdl, rule="macos_sdl3", implicit=[SDL_DIR / "CMakeLists.txt"])

    # ---------- the host executable

    host_objects: List[Path] = []
    host_obj_dir = host_dir / "obj"
    host_arch = f"-arch x86_64 -mmacosx-version-min={MACOS_MINIMUM}"
    n.rule(
        name="macos_host_cc",
        command=f"$macos_host_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    host_cflags = " ".join([
        host_arch, "-O2", "-g", "-Wall", "-Wno-unused-function", "-D_DARWIN_C_SOURCE", "-DHALO_MACOS=1",
        f"-I{PORT_DIR}/host", f"-I{host_gen}", f"-I{ANDROID_DIR}/include", f"-I{SDL_DIR}/include",
        f"-I{LINUX_DIR}/src", f"-I{TOML_DIR}",
    ])
    host_sources = sorted(path for pattern in ("*.c", "*.m", "*.S") for path in (PORT_DIR / "host").glob(pattern))
    host_sources += [
        LINUX_DIR / "src" / "posix_files.c", LINUX_DIR / "src" / "posix_net.c",
        # the launcher's disc image reader (host_launcher.c): the game data
        # is imported before the guest starts
        LINUX_DIR / "src" / "xiso.c",
        # the app reads debug.sample_seconds from config.toml (host_main.c)
        TOML_DIR / "tomlc17.c",
    ]
    for source in host_sources:
        obj = host_obj_dir / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": host_cflags},
                order_only=[guest_syscalls_h, host_table_c])
        host_objects.append(obj)
    # internet play's UPnP (posix_upnp.c, with port/third_party/miniupnpc)
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
        # a 64 KB page zero instead of 4 GB, so that the low 4 GB the guest
        # lives in can be mapped at all
        command=(f"$macos_host_cc {host_arch} -Wl,-pagezero_size,0x10000 -o $out @$out.rsp {libsdl} "
                 + " ".join(f"-framework {framework}" for framework in HOST_FRAMEWORKS) + " "
                 + " ".join(f"-weak_framework {framework}" for framework in HOST_WEAK_FRAMEWORKS) + " "
                 + " ".join(f"-l{library}" for library in HOST_LIBRARIES)),
        description="MACOS HOST LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=executable, rule="macos_host_link", inputs=host_objects, implicit=[libsdl])

    # ---------- the app bundle

    contents = BUILD / APP_NAME / "Contents"
    bundled = [
        (executable, contents / "MacOS" / EXECUTABLE_NAME),
        (image, contents / "Resources" / "halo_guest.elf"),
        # internet play's MQTT brokers: the app writes them beside
        # config.toml at each start (host_main.c), as Android does
        (Path("port/assets/network/brokers.txt"), contents / "Resources" / "brokers.txt"),
        (PORT_DIR / "Info.plist", contents / "Info.plist"),
    ]
    signed = BUILD / (APP_NAME + ".signed")
    copies = " && ".join(f"mkdir -p {target.parent} && cp {source} {target}" for source, target in bundled)
    n.rule(
        name="macos_bundle",
        # copied and signed in one step: signing rewrites the executable,
        # which as an input of a separate step would never be up to date.
        # Ad hoc signing (no identity) is what Apple silicon requires of any
        # executable, an x86_64 one under Rosetta included
        command=f"{copies} && codesign --force --sign - --timestamp=none {BUILD / APP_NAME} && touch $out",
        description="MACOS BUNDLE " + str(BUILD / APP_NAME),
    )
    n.build(outputs=signed, rule="macos_bundle", inputs=[source for source, _ in bundled],
            implicit_outputs=[target for _, target in bundled])
    n.build(outputs="macos", rule="phony", inputs=[signed])
    n.newline()
