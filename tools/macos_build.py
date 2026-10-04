"""Ninja rules for the macOS build (``ninja macos``).

The macOS port (port/macos) runs the Android guest image in a
Hypervisor.framework virtual machine: the game, the platform layer shared
with Linux and a musl runtime, compiled as ILP32 AArch64 code (32-bit
pointers, as the game's data formats require) and executed at native speed
at EL1, with the host's stage-2 tables identity-mapping the guest's
addresses. See port/macos/host/host.h for the design and
port/android/README.md for the guest image's own.

This graph builds

- the guest image, build/macos/halo_guest.elf: exactly the Android build's
  guest (the same generated sources, the same musl, the same layout), with
  import stubs that doorbell (tools/macos_imports.py) instead of jumping
  through the Android loader's table;
- the host, build/macos/halo: a macOS executable (SDL3, Hypervisor) whose
  import dispatch (tools/macos_import_dispatch.py,
  tools/macos_gl_dispatch.py) serves the guest's doorbells.

and stages both, with the entitlements the hypervisor needs, into dist/
through tools/ci_build.py.
"""

import os
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List

from .linux_build import (MINIUPNPC_DEFINES, MINIUPNPC_DIR, MUSL_MATH_DIR,
                          compile_launcher, game_defines_and_includes, game_sources,
                          miniupnpc_sources, musl_math_sources)
from .android_build import (GUEST_ABI_FLAGS, GUEST_CODE_FLAGS, MUSL_DIRECTORIES,
                            MUSL_EXCLUDE, MUSL_FILES, MUSL_THREAD_PREFIXES,
                            VARIADIC_PROTOTYPE_FILES, _musl_sources, fetch_third_party)
from .embed_assets import hud_assets_build, hud_configure_inputs
from .ninja_syntax import Writer

PORT_DIR = Path("port/macos")
ANDROID_DIR = Path("port/android")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/macos")
THIRD_PARTY = BUILD / "third_party"
SDL_TAG = "release-3.4.16"
SDL_DIR = THIRD_PARTY / "SDL3"
SDL_URL = "https://github.com/libsdl-org/SDL.git"
TOML_DIR = Path("port/third_party/tomlc17")
EXPAT_DIR = Path("port/third_party/expat")
EXPAT_SOURCES = ("xmlparse.c", "xmlrole.c", "xmltok.c")
KCP_DIR = Path("port/third_party/kcp")
MONOCYPHER_DIR = Path("port/third_party/monocypher")


MUSL_VERSION = "1.2.5"
MUSL_DIR = THIRD_PARTY / f"musl-{MUSL_VERSION}"
MUSL_URL = f"https://musl.libc.org/releases/musl-{MUSL_VERSION}.tar.gz"


def fetch_musl() -> None:
    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    if not MUSL_DIR.is_dir():
        print(f"Downloading {MUSL_URL}")
        archive = THIRD_PARTY / f"musl-{MUSL_VERSION}.tar.gz"
        subprocess.run(["curl", "-sSfL", "-o", str(archive), MUSL_URL], check=True)
        subprocess.run(["tar", "xzf", archive.name], cwd=THIRD_PARTY, check=True)
        archive.unlink()


def fetch_sdl() -> None:
    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    if not SDL_DIR.is_dir():
        print(f"Cloning SDL3 {SDL_TAG}")
        subprocess.run(["git", "clone", "-q", "--depth", "1", "--branch", SDL_TAG,
                        SDL_URL, str(SDL_DIR)], check=True)


def macos_configure_inputs() -> List[Path]:
    return [Path(__file__), PORT_DIR / "host", ANDROID_DIR / "guest" / "runtime",
            LINUX_DIR / "src", *hud_configure_inputs()]


def generate_macos_build(n: Writer, sln: Any) -> None:
    if sys.platform != "darwin":
        n.comment("macOS build: only on macOS")
        return
    config_path = LINUX_DIR / "port.json"
    if not config_path.is_file() or not (PORT_DIR / "host").is_dir():
        return
    try:
        fetch_musl()
        fetch_sdl()
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"macOS build disabled: cannot fetch SDL3 ({error})", file=sys.stderr)
        return
    import json
    config: Dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))

    guest_cc = os.environ.get("HALO_MACOS_GUEST_CC", "clang")
    host_cc = os.environ.get("HALO_MACOS_HOST_CC", "clang")
    lld = os.environ.get("HALO_MACOS_LLD", "ld.lld")
    python = "$python"

    guest_dir = BUILD / "guest"
    obj_dir = guest_dir / "obj"
    gen_dir = guest_dir / "gen"
    libc_include = guest_dir / "libc_include"
    libc_internal = guest_dir / "libc_internal"
    arch = ANDROID_DIR / "guest" / "libc" / "arch" / "arm64_32"
    semantics_header = Path("build/linux/halo_msvc_semantics.h")
    platform_semantics_header = Path("build/linux/platform_msvc_semantics.h")
    prefix_header = LINUX_DIR / "include" / "halo_linux_prefix.h"
    image = BUILD / "halo_guest.elf"
    host_binary = BUILD / "halo"
    host_obj = BUILD / "host"

    n.comment("macOS build (ninja macos); see port/macos/host/host.h")
    n.variable("macos_guest_cc", guest_cc)
    n.variable("macos_host_cc", host_cc)
    n.variable("macos_llvm_ar", os.environ.get("HALO_MACOS_LLVM_AR", "/opt/homebrew/opt/llvm/bin/llvm-ar"))

    # ---------- generated headers and sources (the Android build's, with
    # this port's doorbell stubs)

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
        command="echo '#define VERSION \"1.2.5\"' > $out",
        description="MACOS MUSL $out",
    )
    n.build(outputs=version_h, rule="macos_version_h")

    # musl's headers, once (the Android build extracts them from its
    # downloaded musl; the same archive)
    musl_include_stamp = libc_include / "stamp"
    n.rule(
        name="macos_musl_include",
        command=f"mkdir -p {libc_include}/bits {libc_internal} && touch $out",
        description="MACOS MUSL HEADERS",
    )
    n.build(outputs=musl_include_stamp, rule="macos_musl_include")

    # the GLES headers of the guest (declarations only; this machine's
    # SDK has them under its own names)
    gl_include = gen_dir / "gl_include"
    gl_stamp = gl_include / "stamp"
    n.rule(
        name="macos_gl_include",
        command=(f"mkdir -p {gl_include} && ln -sfn ../../../../../port/macos/gles/GLES2 {gl_include}/GLES2 && "
                 f"ln -sfn ../../../../../port/macos/gles/GLES3 {gl_include}/GLES3 && "
                 f"ln -sfn ../../../../../port/macos/gles/KHR {gl_include}/KHR && touch $out"),
        description="MACOS GL HEADERS",
    )
    n.build(outputs=gl_stamp, rule="macos_gl_include",
            implicit=[Path("port/macos/gles/GLES3/gl32.h"), Path("port/macos/gles/GLES2/gl2ext.h"),
                      Path("port/macos/gles/KHR/khrplatform.h")])

    guest_gl_c = gen_dir / "guest_gl.c"
    gl_imports = gen_dir / "gl_imports.list"
    n.rule(
        name="macos_gl_stubs",
        command=(f"{python} tools/android_gl_stubs.py {LINUX_DIR}/src/gl.h "
                 f"port/macos/gles/GLES3/gl32.h port/macos/gles/GLES2/gl2ext.h "
                 f"{guest_gl_c} {gl_imports}"),
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
    n.rule(
        name="macos_imports",
        command=f"{python} tools/macos_imports.py {imports_s} $in",
        description="MACOS IMPORTS",
    )
    n.build(outputs=imports_s, rule="macos_imports",
            inputs=[ANDROID_DIR / "host_imports.list", posix_imports, gl_imports],
            implicit=[Path("tools/macos_imports.py")])

    # the host's dispatch (typed wrappers) and GL trampolines
    host_dispatch_c = gen_dir / "host_dispatch.c"
    host_gl_dispatch_c = gen_dir / "host_gl_dispatch.c"
    n.rule(
        name="macos_host_dispatch",
        command=(f"{python} tools/macos_import_dispatch.py "
                 f"--header {PORT_DIR}/host/host.h --header {LINUX_DIR}/src/posix.h "
                 f"--list {ANDROID_DIR}/host_imports.list --list {posix_imports} "
                 f"--list {gl_imports} {host_dispatch_c}"),
        description="MACOS HOST DISPATCH",
    )
    n.build(outputs=host_dispatch_c, rule="macos_host_dispatch",
            inputs=[ANDROID_DIR / "host_imports.list", posix_imports, gl_imports],
            implicit=[Path("tools/macos_import_dispatch.py"), PORT_DIR / "host" / "host.h"])
    n.rule(
        name="macos_gl_dispatch",
        command=(f"{python} tools/macos_gl_dispatch.py --list {gl_imports} "
                 f"--protos port/macos/gles/GLES3/gl32.h,port/macos/gles/GLES2/gl2ext.h "
                 f"{host_gl_dispatch_c}"),
        description="MACOS GL DISPATCH",
    )
    n.build(outputs=host_gl_dispatch_c, rule="macos_gl_dispatch",
            inputs=[gl_imports], implicit=[Path("tools/macos_gl_dispatch.py")])

    generated_headers = [*semantics_header.parents] and [semantics_header, platform_semantics_header]

    # ---------- guest compilation: C -> Darwin assembly -> ELF assembly -> object

    n.rule(
        name="macos_guest_cc",
        command=(f"{compile_launcher(sln)}$macos_guest_cc -MMD -MF $out.d $cflags -S $in -o $out.darwin.s && "
                 f"{python} tools/android_asm_convert.py $out.darwin.s $out.s && "
                 f"$macos_guest_cc --target=aarch64-none-elf -c $out.s -o $out"),
        description="MACOS CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    n.rule(
        name="macos_guest_as",
        command="$macos_guest_cc --target=aarch64-none-elf -c $in -o $out",
        description="MACOS AS $out",
    )

    libc_includes = [
        f"-isystem {libc_include}", f"-isystem {arch}", f"-isystem {THIRD_PARTY}/musl-1.2.5/arch/generic",
        f"-isystem {THIRD_PARTY}/musl-1.2.5/include",
    ]
    guest_abi = " ".join(GUEST_ABI_FLAGS + (["-DHALO_RELEASE"] if getattr(sln, "port_release", False) else []))
    guest_code = " ".join(GUEST_CODE_FLAGS)
    tool_implicit = [Path("tools/android_asm_convert.py"), semantics_header, platform_semantics_header]

    def guest_object(source: Path, cflags: str, prefix: str = "") -> Path:
        obj = obj_dir / prefix / Path(str(source).lstrip("/")).with_suffix(".o")
        if str(source).startswith(str(BUILD)):
            obj = obj_dir / prefix / source.relative_to(BUILD).with_suffix(".o")
        n.build(outputs=obj, rule="macos_guest_cc", inputs=source,
                implicit=tool_implicit, order_only=[musl_include_stamp, gl_stamp, syscall_h, alltypes, version_h],
                variables={"cflags": cflags})
        return obj

    # musl
    musl_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-w",
        f"-I{arch}", f"-I{THIRD_PARTY}/musl-1.2.5/arch/generic", f"-I{libc_internal}",
        f"-I{ANDROID_DIR}/guest/libc/src_include", f"-I{THIRD_PARTY}/musl-1.2.5/src/include",
        f"-I{THIRD_PARTY}/musl-1.2.5/src/internal", f"-I{libc_include}", f"-I{THIRD_PARTY}/musl-1.2.5/include",
    ])
    musl_objects = [guest_object(source, musl_cflags, "musl") for source in _musl_sources(MUSL_DIR)]
    libguestc = guest_dir / "libguestc.a"
    n.rule(
        name="macos_ar",
        command=f"rm -f $out && $macos_llvm_ar rcs $out $in",
        description="MACOS AR $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=libguestc, rule="macos_ar", inputs=musl_objects)

    # the game
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
        guest_abi, guest_code, " ".join(game_flags),
        f"-include {prefix_header}", f"-include {semantics_header}",
        f"-I{LINUX_DIR}/include", game_defines_and_includes(config), *libc_includes,
        f"-idirafter port/include/xdk",
    ])
    objects: List[Path] = []
    for source in game_sources(config):
        cflags = game_cflags
        if source.as_posix() in VARIADIC_PROTOTYPE_FILES:
            cflags += f" -include {ANDROID_DIR}/include/halo_android_variadic_prototypes.h"
        objects.append(guest_object(source, cflags))
    for source in sorted(Path(config["game_sources"]).glob("*.c")):
        objects.append(guest_object(source, game_cflags))

    # the platform layer shared with Linux, and the guest runtime
    platform_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE", "-DHALO_LINUX_PLATFORM_LAYER", "-w",
        f"-include {prefix_header}", f"-include {platform_semantics_header}",
        f"-I{LINUX_DIR}/src", f"-I{LINUX_DIR}/include", f"-I{ANDROID_DIR}/guest/runtime",
        f"-I{ANDROID_DIR}/include", f"-I{TOML_DIR}", f"-I{EXPAT_DIR}", f"-I{KCP_DIR}", f"-I{MONOCYPHER_DIR}",
        "-Isource -Isource/cseries",
        f"-I{SDL_DIR}/include", f"-I{gl_include}", *libc_includes, f"-idirafter port/include/xdk",
    ])
    guest_host_only = {"memory_watch.c"}
    for source in sorted((LINUX_DIR / "src").glob("*.c")):
        if source.name.startswith("posix_") or source.name in guest_host_only:
            continue
        objects.append(guest_object(source, platform_cflags))
    for source in hud_assets_build(n, "macos", gen_dir / "hud_hires_assets.c"):
        objects.append(guest_object(source, platform_cflags))
    objects.append(guest_object(TOML_DIR / "tomlc17.c", platform_cflags))
    for name in EXPAT_SOURCES:
        objects.append(guest_object(EXPAT_DIR / name, platform_cflags))
    objects.append(guest_object(KCP_DIR / "ikcp.c", platform_cflags))
    for name in ("monocypher.c", "monocypher-ed25519.c"):
        objects.append(guest_object(MONOCYPHER_DIR / name, platform_cflags))
    musl_math_cflags = " ".join([
        guest_abi, "-std=gnu11", "-w", *libc_includes, f"-I{MUSL_MATH_DIR}/include",
        f"-include {MUSL_MATH_DIR}/include/libm.h",
    ])
    for source in musl_math_sources():
        objects.append(guest_object(source, musl_math_cflags))
    runtime_internal_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-D_GNU_SOURCE",
        f"-I{ANDROID_DIR}/guest/runtime", f"-I{ANDROID_DIR}/include",
        f"-I{arch}", f"-I{THIRD_PARTY}/musl-1.2.5/arch/generic", f"-I{libc_internal}",
        f"-I{ANDROID_DIR}/guest/libc/src_include", f"-I{THIRD_PARTY}/musl-1.2.5/src/include",
        f"-I{THIRD_PARTY}/musl-1.2.5/src/internal", f"-I{libc_include}", f"-I{THIRD_PARTY}/musl-1.2.5/include",
    ])
    runtime_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE",
        f"-I{ANDROID_DIR}/guest/runtime", f"-I{ANDROID_DIR}/include", f"-I{LINUX_DIR}/src",
        f"-I{SDL_DIR}/include", f"-I{gl_include}", *libc_includes,
    ])
    runtime_dir = ANDROID_DIR / "guest" / "runtime"
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

    # ---------- the guest image (the Android layout; ld.lld from brew)

    linker_script = ANDROID_DIR / "guest" / "guest.ld"
    n.rule(
        name="macos_guest_link",
        command=(f"{lld} -m aarch64linux -static -nostdlib -T {linker_script} "
                 f"-Map $out.map -o $out @$out.rsp {libguestc}"),
        description="MACOS LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=image, rule="macos_guest_link", inputs=objects, implicit=[libguestc, linker_script])

    # ---------- SDL3 for the host

    sdl_build = BUILD / "sdl3-build"
    n.rule(
        name="macos_sdl3",
        command=(f"cmake -S {SDL_DIR} -B {sdl_build} -G Ninja -DCMAKE_BUILD_TYPE=Release "
                 f"-DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF "
                 f"-DSDL_EXAMPLES=OFF > {BUILD}/sdl3-configure.log && "
                 f"ninja -C {sdl_build} > {BUILD}/sdl3-build.log"),
        description="MACOS SDL3",
        pool="console",
    )
    libsdl = sdl_build / "libSDL3.0.dylib"
    n.build(outputs=libsdl, rule="macos_sdl3", implicit=[SDL_DIR / "CMakeLists.txt"])

    # ---------- the host

    host_objects: List[Path] = []
    n.rule(
        name="macos_host_cc",
        command="$macos_host_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    host_cflags = " ".join([
        "-O2", "-g", "-Wall", "-Wno-unused-function",
        "-DBUILDING_HALO_MACOS_HOST",
        f"-I{PORT_DIR}/host", f"-I{PORT_DIR}/gles", f"-I{ANDROID_DIR}/include", f"-I{SDL_DIR}/include", f"-I{LINUX_DIR}/src",
        f"-I{TOML_DIR}", f"-I{ANDROID_DIR}/guest/runtime",
    ])
    host_sources = sorted((PORT_DIR / "host").glob("*.c")) + [host_dispatch_c, host_gl_dispatch_c,
        # the settings file's parser (the host reads debug.sample_seconds)
        TOML_DIR / "tomlc17.c"]
    for source in host_sources:
        obj = host_obj / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": host_cflags})
        host_objects.append(obj)
    # the platform's file and socket helpers, with the Darwin spellings of
    # struct stat's times (port/macos/host/darwin_stat_shim.h)
    posix_host_cflags = " ".join([host_cflags, f"-include {PORT_DIR}/host/darwin_stat_shim.h"])
    for source in [LINUX_DIR / "src" / "posix_files.c", LINUX_DIR / "src" / "posix_net.c"]:
        obj = host_obj / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": posix_host_cflags})
        host_objects.append(obj)
    # internet play's UPnP (posix_upnp.c, with port/third_party/miniupnpc)
    miniupnpc_cflags = " ".join([host_cflags, f"-I{MINIUPNPC_DIR / 'include'}", f"-I{MINIUPNPC_DIR / 'src'}",
                                 *MINIUPNPC_DEFINES])
    for source in [LINUX_DIR / "src" / "posix_upnp.c", *miniupnpc_sources()]:
        obj = host_obj / ("miniupnpc_" + source.name + ".o" if source.parent.parent == MINIUPNPC_DIR
                          else source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source,
                variables={"cflags": miniupnpc_cflags + (" -w" if source.name != "posix_upnp.c" else "")})
        host_objects.append(obj)

    n.rule(
        name="macos_host_link",
        command=(f"$macos_host_cc -o $out $in -L{sdl_build} -lSDL3 "
                 f"-framework Hypervisor -lobjc "
                 f"-Wl,-rpath,{sdl_build} -Wl,-rpath,@executable_path "
                 f"-Wl,-sectcreate,__TEXT,__entitlements,{PORT_DIR}/host/halo.entitlements.plist"),
        description="MACOS HOST LINK $out",
    )
    n.build(outputs=host_binary, rule="macos_host_link", inputs=host_objects, implicit=[libsdl])

    # re-sign (the section-embedded entitlements need the signature)
    n.rule(
        name="macos_codesign",
        command=f"codesign -f -s - $in && touch $out",
        description="MACOS CODESIGN $out",
    )
    n.build(outputs=str(host_binary) + ".signed", rule="macos_codesign", inputs=host_binary)
    n.build(outputs="macos", rule="phony", inputs=[host_binary, image])
    n.newline()