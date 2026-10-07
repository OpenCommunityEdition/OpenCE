"""Ninja rules for the guest image of the Android and macOS ports.

Both ports run the game as ILP32 AArch64 code (32-bit pointers, as the
game's data formats require) inside an ordinary 64-bit process
(port/android/README.md, port/macos/README.md). This graph builds that
code, the guest image: the game sources, the platform layer shared with the
Linux port (port/linux/src) and the guest runtime (port/android/guest) with
a subset of musl as its C library, all compiled by clang for
arm64_32-apple-watchos, converted to ELF assembly
(tools/android_asm_convert.py), assembled for AArch64 and linked at a fixed
address. The macOS port also rebases every memory access onto a base
register (tools/guest_asm_rebase.py), since macOS keeps the low 4 GB of a
process from it.

tools/android_build.py and tools/macos_build.py each describe their guest
with a GuestTarget and add their host around the image.
"""

import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional

from .linux_build import (LINUX_PROFILE, MUSL_MATH_DIR, XDK_INCLUDE, compile_launcher, game_defines_and_includes,
                          game_sources, musl_math_sources, pgo_mode, pgo_profile, profile_use_flags, xdk_headers)
from .embed_assets import hud_assets_build
from .ninja_syntax import Writer

ANDROID_DIR = Path("port/android")
LINUX_DIR = Path("port/linux")
# the TOML parser config.toml is read with (port/linux/src/port_config.c)
TOML_DIR = Path("port/third_party/tomlc17")
EXPAT_DIR = Path("port/third_party/expat")
EXPAT_SOURCES = ("xmlparse.c", "xmlrole.c", "xmltok.c")
KCP_DIR = Path("port/third_party/kcp")
MONOCYPHER_DIR = Path("port/third_party/monocypher")
# the port's zlib (port/third_party/zlib/zlib_prefixed.h): what inflates the
# maps, the menus' and the HUD's PNGs and the updates, data from anywhere,
# instead of the game's own 1.1.3 (its inflate only, its names prefixed z_)
ZLIB_DIR = Path("port/third_party/zlib")
ZLIB_SOURCES = ("adler32.c", "crc32.c", "inffast.c", "inflate.c", "inftrees.c", "uncompr.c", "zutil.c")
# (its names prefixed, and the one Z_PREFIX leaves, its error messages, which
# the game's zlib names the same)
ZLIB_DEFINES = ("-DZ_PREFIX", "-Dz_errmsg=z_port_errmsg")
MUSL_VERSION = "1.2.5"
MUSL_URL = f"https://musl.libc.org/releases/musl-{MUSL_VERSION}.tar.gz"
SDL_TAG = "release-3.4.16"
SDL_URL = "https://github.com/libsdl-org/SDL.git"

# The guest ABI: AArch64 code with 32-bit pointers (clang's only such target
# is Apple's arm64_32, whose Mach-O output is converted afterwards). The
# Darwin environment is hidden from the sources; the C library is musl.
GUEST_ABI_FLAGS = [
    "--target=arm64_32-apple-watchos",
    "-U__APPLE__",
    "-U__MACH__",
    "-fno-define-target-os-macros",
    "-D__linux__=1",
    "-D__unix__=1",
    # the guest's own code: ILP32 AArch64 (HALO_GUEST), drawing with the
    # OpenGL ES 3 subset (HALO_GLES)
    "-DHALO_GUEST=1",
    "-DHALO_GLES=1",
    # ARMv8.0: nothing the emulator's binary translation or an older
    # device could lack (Darwin targets otherwise assume pointer
    # authentication and FP16)
    "-mcpu=cortex-a53",
    "-nostdinc",
    "-fshort-wchar",
    "-fno-stack-protector",
    "-fno-unwind-tables",
    "-fno-asynchronous-unwind-tables",
    "-femulated-tls",
    "-mllvm",
    "-aarch64-neon-syntax=generic",
    # no fused multiply-add: the game was written for x87/SSE arithmetic,
    # and its debug assertions (colours within 0..1, unit vectors) trip on
    # the different rounding of fused operations
    "-ffp-contract=off",
    "-O2",
]

# as the Linux build (tools/linux_build.py), minus what only x86 needs
GUEST_CODE_FLAGS = [
    "-fms-extensions",
    "-fcommon",
    "-fno-strict-aliasing",
    "-fwrapv",
    "-fno-delete-null-pointer-checks",
    "-fno-omit-frame-pointer",
    *(f"-fno-builtin-{name}" for name in (
        "wcslen", "wcsnlen", "wcschr", "wcsrchr", "wcscmp", "wcsncmp", "wcscpy",
        "wcsncpy", "wcscat", "wcsncat", "wmemchr", "wmemcmp", "wmemcpy",
        "wmemmove", "wmemset",
    )),
]

MUSL_DIRECTORIES = [
    "conf", "ctype", "dirent", "env", "errno", "exit", "fcntl", "internal",
    "locale", "malloc", "malloc/mallocng", "math", "mman", "multibyte",
    "prng", "sched", "select", "signal", "stat", "stdio", "stdlib", "string",
    "time", "unistd",
]
MUSL_FILES = [
    "thread/__lock.c", "thread/__wait.c", "thread/__timedwait.c",
    "thread/__syscall_cp.c", "thread/vmlock.c", "thread/pthread_self.c",
    "thread/pthread_equal.c", "thread/pthread_once.c",
    "thread/pthread_setcancelstate.c", "thread/pthread_testcancel.c",
    "thread/default_attr.c", "thread/lock_ptc.c", "misc/getauxval.c",
    "linux/sysinfo.c", "misc/basename.c", "misc/dirname.c",
    "misc/realpath.c", "misc/uname.c", "misc/ioctl.c", "misc/getrlimit.c",
    "misc/syscall.c", "network/htonl.c", "network/htons.c", "network/ntohl.c",
    "network/ntohs.c", "network/inet_addr.c", "network/inet_aton.c",
    "network/inet_ntoa.c", "network/inet_pton.c", "network/inet_ntop.c",
]
MUSL_THREAD_PREFIXES = (
    "pthread_attr_", "pthread_cond", "pthread_mutex", "pthread_rwlock",
    "pthread_spin", "sem_",
)
# replaced by the guest runtime (port/android/guest/runtime)
MUSL_EXCLUDE = {
    "env/__stack_chk.c", "env/__init_tls.c", "env/__libc_start_main.c",
    "env/__reset_tls.c", "malloc/oldmalloc", "thread/pthread_create.c",
    # unused, and its compiler barrier is an inline assembly statement
    "string/explicit_bzero.c",
}
# game files that call variadic functions without a prototype in scope, which
# only works under x86's calling convention (tools/android_abi_check.py)
VARIADIC_PROTOTYPE_FILES = {
    "source/ai/action_uncover.c", "source/ai/ai.c", "source/ai/ai_debug.c",
    "source/bungie_net/common/public_key_crypt.c", "source/camera/editor_flying_camera.c",
    "source/game/cheats.c", "source/game/game_engine.c", "source/game/players.c",
    "source/hs/hs.c", "source/interface/hud_nav_points.c",
    "source/networking/telnet_console.c", "source/rasterizer/xbox/rasterizer_xbox_errors.c",
    "source/render/render.c",
}


@dataclass
class GuestTarget:
    """What differs between the ports' guests."""
    # rule names and descriptions start with it: "android" or "macos"
    name: str
    # build/android or build/macos
    build: Path
    # its fetched musl and SDL3 (fetch_third_party)
    third_party: Path
    guest_cc: str
    # the platform's defines (-DHALO_ANDROID=1, ...) and any further code
    # generation flags
    defines: List[str]
    # a folder holding the OpenGL ES headers: GLES2, GLES3 and KHR
    gl_headers: Path
    # the import lists beside the generated GL and posix ones
    import_lists: List[Path]
    # the host's name-to-function table, generated with the stubs
    host_table: Path
    # ld.lld, and llvm-ar to archive musl (None: musl's objects are given to
    # the linker between --start-lib and --end-lib)
    linker: str
    archiver: Optional[str]
    # appended to the link: compiler-rt's builtins, say
    link_libraries: str = ""
    # rebase memory accesses onto x28 (tools/guest_asm_rebase.py)
    rebase: bool = False
    # further guest runtime sources and include folders
    runtime_sources: List[Path] = field(default_factory=list)
    runtime_includes: List[Path] = field(default_factory=list)
    # the assembler's target
    assembler_target: str = "aarch64-linux-android"
    # the host's hostposix_ imports are wrappers of its own
    # (tools/android_imports.py --posix-wrappers)
    posix_wrappers: bool = False

    @property
    def musl(self) -> Path:
        return self.third_party / f"musl-{MUSL_VERSION}"

    @property
    def sdl(self) -> Path:
        return self.third_party / "SDL3"


def fetch_third_party(third_party: Path) -> None:
    """Download musl and SDL3 into third_party (configure time, once)."""
    third_party.mkdir(parents=True, exist_ok=True)
    musl = third_party / f"musl-{MUSL_VERSION}"
    sdl = third_party / "SDL3"
    if not musl.is_dir():
        print(f"Downloading {MUSL_URL}")
        archive = third_party / f"musl-{MUSL_VERSION}.tar.gz"
        subprocess.run(["curl", "-sSfL", "-o", str(archive), MUSL_URL], check=True)
        subprocess.run(["tar", "xzf", archive.name], cwd=third_party, check=True)
        archive.unlink()
    if not sdl.is_dir():
        print(f"Cloning SDL3 {SDL_TAG}")
        subprocess.run(["git", "clone", "-q", "--depth", "1", "--branch", SDL_TAG, SDL_URL, str(sdl)],
                       check=True)


def musl_sources(musl: Path) -> List[Path]:
    src = musl / "src"
    result = set()
    for directory in MUSL_DIRECTORIES:
        for path in (src / directory).glob("*.c"):
            result.add(path)
    for name in MUSL_FILES:
        result.add(src / name)
    for path in (src / "thread").glob("*.c"):
        if path.name.startswith(MUSL_THREAD_PREFIXES):
            result.add(path)
    sources = []
    for path in sorted(result):
        relative = path.relative_to(src).as_posix()
        if relative in MUSL_EXCLUDE or any(relative.startswith(e + "/") for e in MUSL_EXCLUDE):
            continue
        sources.append(path)
    return sources


def generate_guest(n: Writer, sln: Any, target: GuestTarget, config: Dict[str, Any]) -> Path:
    """The guest image's rules; returns the image."""
    name = target.name
    upper = name.upper()
    build = target.build
    musl = target.musl
    guest_dir = build / "guest"
    obj_dir = guest_dir / "obj"
    gen_dir = guest_dir / "gen"
    libc_include = guest_dir / "libc_include"
    libc_internal = guest_dir / "libc_internal"
    gl_include = guest_dir / "gl_include"
    arch = ANDROID_DIR / "guest" / "libc" / "arch" / "arm64_32"
    semantics_header = Path("build/linux/halo_msvc_semantics.h")
    platform_semantics_header = Path("build/linux/platform_msvc_semantics.h")
    prefix_header = LINUX_DIR / "include" / "halo_linux_prefix.h"
    image = build / "halo_guest.elf"
    python = "$python"
    guest_cc_variable = f"${name}_guest_cc"

    n.variable(f"{name}_guest_cc", target.guest_cc)

    # ---------- generated headers and sources

    alltypes = libc_include / "bits" / "alltypes.h"
    syscall_h = libc_include / "bits" / "syscall.h"
    version_h = libc_internal / "version.h"
    n.rule(
        name=f"{name}_alltypes",
        command=f"sed -f {musl}/tools/mkalltypes.sed $in > $out",
        description=f"{upper} MUSL $out",
    )
    n.build(outputs=alltypes, rule=f"{name}_alltypes",
            inputs=[arch / "bits" / "alltypes.h.in", musl / "include" / "alltypes.h.in"])
    n.rule(
        name=f"{name}_syscall_h",
        command="cp $in $out && sed -n -e s/__NR_/SYS_/p < $in >> $out",
        description=f"{upper} MUSL $out",
    )
    n.build(outputs=syscall_h, rule=f"{name}_syscall_h", inputs=arch / "bits" / "syscall.h.in")
    n.rule(
        name=f"{name}_version_h",
        command=f"echo '#define VERSION \"{MUSL_VERSION}\"' > $out",
        description=f"{upper} MUSL $out",
    )
    n.build(outputs=version_h, rule=f"{name}_version_h")

    # the OpenGL ES headers (C declarations only) for the guest
    gl_stamp = gl_include / "stamp"
    headers = target.gl_headers
    n.rule(
        name=f"{name}_gl_include",
        command=(f"mkdir -p {gl_include} && ln -sfn {headers}/GLES2 {gl_include}/GLES2 && "
                 f"ln -sfn {headers}/GLES3 {gl_include}/GLES3 && "
                 f"ln -sfn {headers}/KHR {gl_include}/KHR && touch $out"),
        description=f"{upper} GL HEADERS",
    )
    n.build(outputs=gl_stamp, rule=f"{name}_gl_include")

    guest_gl_c = gen_dir / "guest_gl.c"
    gl_imports = gen_dir / "gl_imports.list"
    n.rule(
        name=f"{name}_gl_stubs",
        command=(f"{python} tools/android_gl_stubs.py {LINUX_DIR}/src/gl.h {headers}/GLES3/gl32.h "
                 f"{headers}/GLES2/gl2ext.h {guest_gl_c} {gl_imports}"),
        description=f"{upper} GL STUBS",
    )
    n.build(outputs=[guest_gl_c, gl_imports], rule=f"{name}_gl_stubs",
            implicit=[Path("tools/android_gl_stubs.py"), LINUX_DIR / "src" / "gl.h"])

    guest_posix_c = gen_dir / "guest_posix.c"
    posix_imports = gen_dir / "posix_imports.list"
    n.rule(
        name=f"{name}_posix_stubs",
        command=f"{python} tools/android_posix_stubs.py {LINUX_DIR}/src/posix.h {guest_posix_c} {posix_imports}",
        description=f"{upper} POSIX STUBS",
    )
    n.build(outputs=[guest_posix_c, posix_imports], rule=f"{name}_posix_stubs",
            implicit=[Path("tools/android_posix_stubs.py"), LINUX_DIR / "src" / "posix.h"])

    imports_s = gen_dir / "imports.s"
    n.rule(
        name=f"{name}_imports",
        command=(f"{python} tools/android_imports.py --host-table {target.host_table}"
                 f"{' --posix-wrappers' if target.posix_wrappers else ''} {imports_s} $in"),
        description=f"{upper} IMPORTS",
    )
    n.build(outputs=[imports_s, target.host_table], rule=f"{name}_imports",
            inputs=[*target.import_lists, posix_imports, gl_imports],
            implicit=[Path("tools/android_imports.py")])

    generated_headers = [*xdk_headers(), alltypes, syscall_h, version_h, gl_stamp,
                         semantics_header, platform_semantics_header]

    # ---------- guest compilation: C -> Darwin assembly -> ELF assembly -> object

    convert = f"{python} tools/android_asm_convert.py $out.darwin.s $out.s"
    tools = [Path("tools/android_asm_convert.py")]
    if target.rebase:
        convert = (f"{python} tools/android_asm_convert.py $out.darwin.s $out.elf.s && "
                   f"{python} tools/guest_asm_rebase.py $out.elf.s $out.s")
        tools.append(Path("tools/guest_asm_rebase.py"))
    n.rule(
        name=f"{name}_guest_cc",
        command=(f"{compile_launcher(sln)}{guest_cc_variable} -MMD -MF $out.d $cflags -S $in -o $out.darwin.s && "
                 f"{convert} && "
                 f"{guest_cc_variable} --target={target.assembler_target} -c $out.s -o $out"),
        description=f"{upper} CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    n.rule(
        name=f"{name}_guest_as",
        command=f"{guest_cc_variable} --target={target.assembler_target} -c $in -o $out",
        description=f"{upper} AS $out",
    )

    libc_includes = [
        f"-isystem {libc_include}", f"-isystem {arch}", f"-isystem {musl}/arch/generic",
        f"-isystem {musl}/include",
    ]
    guest_abi = " ".join(GUEST_ABI_FLAGS + target.defines
                         + (["-DHALO_RELEASE"] if getattr(sln, "port_release", False) else []))
    guest_code = " ".join(GUEST_CODE_FLAGS)
    tool_implicit = [*tools, *generated_headers]
    # profile-guided optimisation with the Linux build's profile (committed,
    # or trained by the Linux build with --pgo=train): the game and platform
    # code are the same, and functions that differ simply go without
    profile = pgo_profile(sln, LINUX_PROFILE if pgo_mode(sln) == "train" else None, [LINUX_PROFILE],
                          target.guest_cc)
    profile_flags = " ".join(profile_use_flags(profile))
    if profile:
        tool_implicit.append(profile)

    def guest_object(source: Path, cflags: str, prefix: str = "") -> Path:
        obj = obj_dir / prefix / Path(str(source).lstrip("/")).with_suffix(".o")
        if str(source).startswith(str(build)):
            obj = obj_dir / prefix / source.relative_to(build).with_suffix(".o")
        n.build(outputs=obj, rule=f"{name}_guest_cc", inputs=source, implicit=tool_implicit,
                variables={"cflags": cflags})
        return obj

    # musl
    musl_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-w",
        f"-I{arch}", f"-I{musl}/arch/generic", f"-I{libc_internal}",
        f"-I{ANDROID_DIR}/guest/libc/src_include", f"-I{musl}/src/include",
        f"-I{musl}/src/internal", f"-I{libc_include}", f"-I{musl}/include",
    ])
    musl_objects = [guest_object(source, musl_cflags, "musl") for source in musl_sources(musl)]
    libguestc: Optional[Path] = None
    if target.archiver:
        libguestc = guest_dir / "libguestc.a"
        n.rule(
            name=f"{name}_ar",
            command=f"rm -f $out && {target.archiver} rcs $out @$out.rsp",
            description=f"{upper} AR $out",
            rspfile="$out.rsp",
            rspfile_content="$in_newline",
        )
        n.build(outputs=libguestc, rule=f"{name}_ar", inputs=musl_objects)

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
        # the headers of the port's own game units (port/linux/game), for the
        # game sources that call them
        f"-iquote {Path(config['game_sources'])}",
        game_defines_and_includes(config), *libc_includes, f"-idirafter {XDK_INCLUDE}",
    ])
    for source in game_sources(config):
        cflags = game_cflags
        if source.as_posix() in VARIADIC_PROTOTYPE_FILES:
            cflags += f" -include {ANDROID_DIR}/include/halo_android_variadic_prototypes.h"
        objects.append(guest_object(source, cflags))
    for source in sorted(Path(config["game_sources"]).glob("*.c")):
        objects.append(guest_object(source, game_cflags))

    # the platform layer shared with Linux, and the guest runtime
    runtime_includes = [ANDROID_DIR / "guest" / "runtime", ANDROID_DIR / "include", *target.runtime_includes]
    platform_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE", "-DHALO_LINUX_PLATFORM_LAYER", "-w", profile_flags,
        f"-include {prefix_header}", f"-include {platform_semantics_header}",
        f"-I{LINUX_DIR}/src", f"-I{LINUX_DIR}/include", *(f"-I{path}" for path in runtime_includes),
        f"-I{TOML_DIR}", f"-I{EXPAT_DIR}", f"-I{KCP_DIR}", f"-I{MONOCYPHER_DIR}",
        f"-I{ZLIB_DIR}", "-Isource -Isource/cseries",
        f"-I{target.sdl}/include", f"-I{gl_include}", *libc_includes, f"-idirafter {XDK_INCLUDE}",
    ])
    guest_host_only = {"memory_watch.c"}  # replaced by guest_memory_watch.c
    for source in sorted((LINUX_DIR / "src").glob("*.c")):
        if source.name.startswith("posix_") or source.name in guest_host_only:
            continue
        objects.append(guest_object(source, platform_cflags))
    # the high-res HUD's textures (port/assets/hud; port/linux/src/hud_hires.c)
    for source in hud_assets_build(n, name, gen_dir / "hud_hires_assets.c"):
        objects.append(guest_object(source, platform_cflags))
    # the settings file's parser (port/third_party/tomlc17)
    objects.append(guest_object(TOML_DIR / "tomlc17.c", platform_cflags))
    # the menus' XML parser (port/third_party/expat; menu_files.c)
    for source_name in EXPAT_SOURCES:
        objects.append(guest_object(EXPAT_DIR / source_name, platform_cflags))
    # internet play's reliable streams (port/third_party/kcp; p2p.c)
    objects.append(guest_object(KCP_DIR / "ikcp.c", platform_cflags))
    # internet play's signatures, for public games' listings
    # (port/third_party/monocypher; p2p_crypto.c)
    for source_name in ("monocypher.c", "monocypher-ed25519.c"):
        objects.append(guest_object(MONOCYPHER_DIR / source_name, platform_cflags))
    # the port's zlib
    for source_name in ZLIB_SOURCES:
        # (not the CPU's CRC32 instructions, which the guest's assembly step
        # is not told it may use)
        objects.append(guest_object(ZLIB_DIR / source_name, " ".join([platform_cflags, *ZLIB_DEFINES,
                                                                      "-U__ARM_FEATURE_CRC32"])))
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
        *(f"-I{path}" for path in runtime_includes),
        f"-I{arch}", f"-I{musl}/arch/generic", f"-I{libc_internal}",
        f"-I{ANDROID_DIR}/guest/libc/src_include", f"-I{musl}/src/include",
        f"-I{musl}/src/internal", f"-I{libc_include}", f"-I{musl}/include",
    ])
    runtime_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE",
        *(f"-I{path}" for path in runtime_includes), f"-I{LINUX_DIR}/src",
        f"-I{target.sdl}/include", f"-I{gl_include}", *libc_includes,
    ])
    runtime_dir = ANDROID_DIR / "guest" / "runtime"
    for source in sorted(runtime_dir.glob("*.c")) + list(target.runtime_sources):
        if source.name in ("guest_thread.c", "guest_start.c"):
            objects.append(guest_object(source, runtime_internal_cflags))
        elif source.name == "guest_memory_watch.c":
            objects.append(guest_object(source, platform_cflags))
        else:
            objects.append(guest_object(source, runtime_cflags))
    objects.append(guest_object(guest_gl_c, runtime_cflags))
    objects.append(guest_object(guest_posix_c, runtime_cflags))
    imports_o = obj_dir / "gen" / "imports.o"
    n.build(outputs=imports_o, rule=f"{name}_guest_as", inputs=imports_s)
    objects.append(imports_o)

    # ---------- the guest image

    linker_script = ANDROID_DIR / "guest" / "guest.ld"
    if libguestc:
        libraries = str(libguestc)
        implicit = [libguestc, linker_script]
    else:
        libraries = "--start-lib " + " ".join(str(obj) for obj in musl_objects) + " --end-lib"
        implicit = [*musl_objects, linker_script]
    n.rule(
        name=f"{name}_guest_link",
        command=(f"{target.linker} -m aarch64linux -static -nostdlib -T {linker_script} "
                 f"-Map $out.map -o $out @$out.rsp {libraries} {target.link_libraries}").rstrip(),
        description=f"{upper} LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.build(outputs=image, rule=f"{name}_guest_link", inputs=objects, implicit=implicit)
    return image
