import hashlib
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List

from .linux_build import (GAME_FLAGS, MINIUPNPC_DEFINES, MINIUPNPC_DIR, MUSL_MATH_DIR, STB_VORBIS_SOURCE,
                          XDK_INCLUDE, compile_launcher, configuration_defines, game_defines_and_includes,
                          game_sources, miniupnpc_sources, musl_math_sources, opus_cflags, opus_sources,
                          updater_defines, xdk_headers)
from .embed_assets import hud_assets_build, hud_configure_inputs
from .ninja_syntax import Writer

PORT_DIR = Path("port/macos")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/macos")
THIRD_PARTY = BUILD / "third_party"
RUNTIME_DIR = PORT_DIR / "guest" / "runtime"
LIBC_DIR = PORT_DIR / "guest" / "libc"
TOML_DIR = Path("port/third_party/tomlc17")
EXPAT_DIR = Path("port/third_party/expat")
EXPAT_SOURCES = ("xmlparse.c", "xmlrole.c", "xmltok.c")
KCP_DIR = Path("port/third_party/kcp")
MBEDTLS_DIR = Path("port/third_party/mbedtls")
MONOCYPHER_DIR = Path("port/third_party/monocypher")
ZLIB_DIR = Path("port/third_party/zlib")
ZLIB_SOURCES = ("adler32.c", "crc32.c", "inffast.c", "inflate.c", "inftrees.c", "uncompr.c", "zutil.c")
ZLIB_DEFINES = ("-DZ_PREFIX", "-Dz_errmsg=z_port_errmsg")
MUSL_VERSION = "1.2.5"
MUSL_DIR = THIRD_PARTY / f"musl-{MUSL_VERSION}"
MUSL_URL = f"https://musl.libc.org/releases/musl-{MUSL_VERSION}.tar.gz"
MUSL_SHA256 = "a9a118bbe84d8764da0ea0d28b3ab3fae8477fc7e4085d90102b8596fc7c75e4"
SDL_TAG = "release-3.4.16"
SDL_COMMIT = "fa2c02bb6e21974a89ea9824bc53c9932abe5f9c"
SDL_DIR = THIRD_PARTY / "SDL3"
SDL_URL = "https://github.com/libsdl-org/SDL.git"
ANGLE_URL = ("https://github.com/kivy/angle-builder/releases/download/chromium-7151_rev1/"
             "angle-macos-universal.tar.gz")
ANGLE_SHA256 = "b87f1d0144218a0699859340bfc9864949190d5f4b4d19d4265c3ab561fcd078"
ANGLE_DIR = THIRD_PARTY / "angle"
HOMEBREW = next((Path(prefix) for prefix in ("/opt/homebrew", "/usr/local") if Path(prefix, "opt/llvm@20").is_dir()),
                Path("/opt/homebrew"))
LLVM_BIN = HOMEBREW / "opt/llvm@20/bin"
LLD_BIN = HOMEBREW / "opt/lld@20/bin"
MACOS_MINIMUM = "11.0"
MACOS_FRAMEWORKS = ("Cocoa", "IOKit", "CoreAudio", "AudioToolbox", "CoreVideo", "Carbon", "ForceFeedback",
                    "GameController", "CoreHaptics", "Metal", "QuartzCore", "UniformTypeIdentifiers", "AVFoundation",
                    "CoreMedia", "Security")

GUEST_ABI_FLAGS = {
    "x86_64": [
        "--target=x86_64-linux-gnux32",
        "-DHALO_MACOS=1",
        "-DHALO_GLES=1",
        "-nostdinc",
        "-fshort-wchar",
        "-fno-pic",
        "-fno-stack-protector",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-femulated-tls",
        "-ffp-contract=off",
        "-O2",
    ],
    "arm64": [
        "--target=arm64_32-apple-watchos",
        "-U__APPLE__",
        "-U__MACH__",
        "-fno-define-target-os-macros",
        "-D__linux__=1",
        "-D__unix__=1",
        "-DHALO_ARM64_GUEST=1",
        "-DHALO_GLES=1",
        "-mcpu=cortex-a53",
        "-nostdinc",
        "-fshort-wchar",
        "-fno-stack-protector",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-femulated-tls",
        "-mllvm",
        "-aarch64-neon-syntax=generic",
        "-ffp-contract=off",
        "-O2",
        "-DHALO_MACOS=1",
    ],
}
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
MUSL_EXCLUDE = {
    "env/__stack_chk.c", "env/__init_tls.c", "env/__libc_start_main.c",
    "env/__reset_tls.c", "malloc/oldmalloc", "thread/pthread_create.c",
    "string/explicit_bzero.c", "string/memcmp.c",
}
VARIADIC_PROTOTYPE_FILES = {
    "source/ai/action_uncover.c", "source/ai/ai.c", "source/ai/ai_debug.c",
    "source/bungie_net/common/public_key_crypt.c", "source/camera/editor_flying_camera.c",
    "source/game/cheats.c", "source/game/game_engine.c", "source/game/players.c",
    "source/hs/hs.c", "source/interface/hud_nav_points.c",
    "source/networking/telnet_console.c", "source/rasterizer/xbox/rasterizer_xbox_errors.c",
    "source/render/render.c",
}
GUEST_ELF = {"x86_64": ("x86_64-linux-gnux32", "elf32_x86_64"), "arm64": ("aarch64-linux-gnu", "aarch64linux")}


def macos_configure_inputs() -> List[Path]:
    return [Path(__file__), PORT_DIR, RUNTIME_DIR, LINUX_DIR / "src", *hud_configure_inputs()]


def fetch_third_party() -> None:
    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    if not MUSL_DIR.is_dir():
        print(f"Downloading {MUSL_URL}")
        archive = THIRD_PARTY / f"musl-{MUSL_VERSION}.tar.gz"
        subprocess.run(["curl", "-sSfL", "-o", str(archive), MUSL_URL], check=True)
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        if digest != MUSL_SHA256:
            archive.unlink()
            raise SystemExit(f"{MUSL_URL}: SHA-256 {digest}, not the pinned {MUSL_SHA256}")
        subprocess.run(["tar", "xzf", archive.name], cwd=THIRD_PARTY, check=True)
        archive.unlink()
    if not SDL_DIR.is_dir():
        print(f"Cloning SDL3 {SDL_TAG}")
        subprocess.run(["git", "clone", "-q", "--depth", "1", "--branch", SDL_TAG, SDL_URL, str(SDL_DIR)],
                       check=True)
        commit = subprocess.run(["git", "-C", str(SDL_DIR), "rev-parse", "HEAD"], capture_output=True, text=True,
                                check=True).stdout.strip()
        if commit != SDL_COMMIT:
            shutil.rmtree(SDL_DIR)
            raise SystemExit(f"SDL3 {SDL_TAG} is {commit}, not the pinned {SDL_COMMIT}")


def musl_sources() -> List[Path]:
    src = MUSL_DIR / "src"
    result = set()
    for directory in MUSL_DIRECTORIES:
        result.update((src / directory).glob("*.c"))
    result.update(src / name for name in MUSL_FILES)
    result.update(path for path in (src / "thread").glob("*.c") if path.name.startswith(MUSL_THREAD_PREFIXES))
    sources = []
    for path in sorted(result):
        relative = path.relative_to(src).as_posix()
        if relative in MUSL_EXCLUDE or any(relative.startswith(e + "/") for e in MUSL_EXCLUDE):
            continue
        sources.append(path)
    return sources


def fetch_angle() -> None:
    if ANGLE_DIR.is_dir():
        return
    print(f"Downloading {ANGLE_URL}")
    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    archive = THIRD_PARTY / "angle.tar.gz"
    subprocess.run(["curl", "-sSfL", "-o", str(archive), ANGLE_URL], check=True)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != ANGLE_SHA256:
        archive.unlink()
        raise SystemExit(f"{ANGLE_URL}: SHA-256 {digest}, not the pinned {ANGLE_SHA256}")
    ANGLE_DIR.mkdir()
    subprocess.run(["tar", "xzf", str(archive.resolve())], cwd=ANGLE_DIR, check=True)
    archive.unlink()


def generate_macos_build(n: Writer, sln: Any) -> None:
    config_path = LINUX_DIR / "port.json"
    if not getattr(sln, "macos", False) or not config_path.is_file():
        return
    try:
        fetch_third_party()
        fetch_angle()
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"macOS build disabled: cannot fetch musl, SDL3 or ANGLE ({error})", file=sys.stderr)
        return
    import json
    config: Dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))

    n.comment("macOS build (ninja macos); see port/macos/README.md")
    n.variable("macos_guest_cc", LLVM_BIN / "clang")
    n.rule(name="macos_copy", command="cp $in $out", description="MACOS COPY $out")

    plugin = BUILD / "guest_rebase.dylib"
    llvm_flags = subprocess.run([str(LLVM_BIN / "llvm-config"), "--cxxflags"], capture_output=True, text=True,
                                check=True).stdout.strip()
    n.rule(name="macos_plugin",
           command=f"clang++ -shared -fPIC -O2 {llvm_flags} $in -o $out -Wl,-undefined,dynamic_lookup",
           description="MACOS PLUGIN $out")
    n.build(outputs=plugin, rule="macos_plugin", inputs=PORT_DIR / "compiler" / "guest_rebase.cpp")

    resource_dir = subprocess.run([str(LLVM_BIN / "clang"), "-print-resource-dir"], capture_output=True, text=True,
                                  check=True).stdout.strip()
    n.rule(name="macos_guest_cc",
           command=f"{compile_launcher(sln)}$macos_guest_cc -MMD -MF $out.d $cflags -c $in -o $out",
           description="MACOS CC $out", depfile="$out.d", deps="gcc")
    n.rule(name="macos_guest_cc_arm64",
           command=(f"{compile_launcher(sln)}$macos_guest_cc -MMD -MF $out.d $cflags -fpass-plugin={plugin} "
                    f"-S $in -o $out.darwin.s && $python tools/macos_asm_convert.py $out.darwin.s $out.s && "
                    "$macos_guest_cc --target=aarch64-linux-gnu -c $out.s -o $out"),
           description="MACOS CC $out", depfile="$out.d", deps="gcc")
    n.rule(name="macos_guest_as", command="$macos_guest_cc --target=$target -c $in -o $out",
           description="MACOS AS $out")
    n.rule(name="macos_ar", command=f"rm -f $out && {LLVM_BIN}/llvm-ar rcs $out @$out.rsp",
           description="MACOS AR $out", rspfile="$out.rsp", rspfile_content="$in_newline")
    n.rule(name="macos_guest_link",
           command=(f"{LLD_BIN}/ld.lld -m $emulation -static -nostdlib -T {PORT_DIR / 'guest' / 'guest.ld'} "
                    "--error-limit=0 -Map $out.map -o $out @$out.rsp $libc"),
           description="MACOS LINK $out", rspfile="$out.rsp", rspfile_content="$in_newline")
    n.rule(name="macos_alltypes", command=f"sed -f {MUSL_DIR}/tools/mkalltypes.sed $in > $out",
           description="MACOS MUSL $out")
    n.rule(name="macos_syscall_h", command="cp $in $out && sed -n -e s/__NR_/SYS_/p < $in >> $out",
           description="MACOS MUSL $out")
    n.rule(name="macos_version_h", command=f"echo '#define VERSION \"{MUSL_VERSION}\"' > $out",
           description="MACOS MUSL $out")
    n.rule(name="macos_gl_stubs",
           command=(f"$python tools/macos_gl_stubs.py $arch {LINUX_DIR}/src/gl.h {ANGLE_DIR}/include/GLES3/gl32.h "
                    f"{ANGLE_DIR}/include/GLES2/gl2ext.h $out"),
           description="MACOS GL STUBS")
    n.rule(name="macos_posix_stubs",
           command=f"$python tools/macos_posix_stubs.py {LINUX_DIR}/src/posix.h $out",
           description="MACOS POSIX STUBS")
    n.rule(name="macos_imports", command="$python tools/macos_imports.py $arch --host-table $out $in",
           description="MACOS IMPORTS")

    images = {}
    syscall_headers = {}
    for arch in GUEST_ABI_FLAGS:
        images[arch], syscall_headers[arch], host_table_c = guest_image(n, sln, config, arch, plugin, resource_dir)


    sdl_build = BUILD / "sdl3"
    libsdl = sdl_build / "libSDL3.a"
    n.rule(name="macos_sdl3",
           command=(f"cmake -S {SDL_DIR} -B {sdl_build} -G Ninja '-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64' "
                    f"-DCMAKE_OSX_DEPLOYMENT_TARGET={MACOS_MINIMUM} -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=OFF "
                    "-DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF "
                    f"> {BUILD}/sdl3-configure.log && ninja -C {sdl_build} > {BUILD}/sdl3-build.log"),
           description="MACOS SDL3", pool="console")
    n.build(outputs=libsdl, rule="macos_sdl3", implicit=[SDL_DIR / "CMakeLists.txt"])


    host_obj_dir = BUILD / "host" / "obj"
    n.rule(name="macos_host_cc", command="clang -MMD -MF $out.d $cflags -c $in -o $out",
           description="MACOS HOST CC $out", depfile="$out.d", deps="gcc")
    host_cflags = " ".join([
        "-arch x86_64 -arch arm64", f"-mmacosx-version-min={MACOS_MINIMUM}", "-O2", "-g", "-Wall",
        "-Wno-unused-function", f"-I{PORT_DIR}/host", f"-I{PORT_DIR}/include",
        f"-I{SDL_DIR}/include", f"-I{LINUX_DIR}/src", f"-isystem {ANGLE_DIR}/include",
        *(f"-Xarch_{arch} -iquote{header.parent.parent}" for arch, header in syscall_headers.items()),
    ])
    host_sources = [*sorted((PORT_DIR / "host").glob("*.c")), host_table_c, LINUX_DIR / "src" / "posix_files.c", LINUX_DIR / "src" / "posix_net.c"]
    host_objects = []
    for source in host_sources:
        obj = host_obj_dir / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": host_cflags},
                implicit=list(syscall_headers.values()) if source.name == "host_syscall.c" else [])
        host_objects.append(obj)
    miniupnpc_cflags = " ".join([host_cflags, f"-I{MINIUPNPC_DIR / 'include'}", f"-I{MINIUPNPC_DIR / 'src'}",
                                 *MINIUPNPC_DEFINES])
    for source in [LINUX_DIR / "src" / "posix_upnp.c", *miniupnpc_sources()]:
        obj = host_obj_dir / ("miniupnpc_" + source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source,
                variables={"cflags": miniupnpc_cflags + (" -w" if source.name != "posix_upnp.c" else "")})
        host_objects.append(obj)
    mbedtls_cflags = " ".join([host_cflags, f"-I{MBEDTLS_DIR / 'include'}",
                               f"-I{MBEDTLS_DIR / 'library'}", "-DMBEDTLS_THREADING_C",
                               "-DMBEDTLS_THREADING_PTHREAD"])
    for source in [LINUX_DIR / "src" / "posix_dtls.c", LINUX_DIR / "src" / "posix_update.c",
                   *sorted((MBEDTLS_DIR / "library").glob("*.c"))]:
        obj = host_obj_dir / ("mbedtls_" + source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source,
                variables={"cflags": mbedtls_cflags + (" -w" if source.parent.parent == MBEDTLS_DIR else "")})
        host_objects.append(obj)


    app = BUILD / "OpenCE.app" / "Contents"
    host = app / "MacOS" / "halo"
    frameworks = [app / "Frameworks" / name for name in ("libEGL.dylib", "libGLESv2.dylib")]
    n.rule(name="macos_host_link",
           command=(f"clang -arch x86_64 -arch arm64 -mmacosx-version-min={MACOS_MINIMUM} -o $out $in {libsdl} "
                    "-Xarch_x86_64 -Wl,-pagezero_size,0x1000 "
                    f"-L{app / 'Frameworks'} -lEGL -lGLESv2 -Wl,-rpath,@executable_path/../Frameworks "
                    + " ".join(f"-framework {name}" for name in MACOS_FRAMEWORKS)),
           description="MACOS HOST LINK $out")
    n.build(outputs=host, rule="macos_host_link", inputs=host_objects, implicit=[libsdl, *frameworks])
    for library in frameworks:
        n.build(outputs=library, rule="macos_copy", inputs=ANGLE_DIR / library.name)
    staged = [app / "Info.plist"]
    n.build(outputs=staged[0], rule="macos_copy", inputs=PORT_DIR / "Info.plist")
    for arch, image in images.items():
        staged.append(app / "Resources" / image.name)
        n.build(outputs=staged[-1], rule="macos_copy", inputs=image)
    n.build(outputs="macos", rule="phony", inputs=[host, *frameworks, *staged])
    n.newline()


def guest_image(n: Writer, sln: Any, config: Dict[str, Any], arch: str, plugin: Path, resource_dir: str):
    guest_dir = BUILD / f"guest-{arch}"
    obj_dir = guest_dir / "obj"
    gen_dir = guest_dir / "gen"
    libc_include = guest_dir / "libc_include"
    libc_internal = guest_dir / "libc_internal"
    runtime_dir = RUNTIME_DIR
    arm64_32 = LIBC_DIR / "arch" / "arm64_32"
    semantics_header = Path("build/linux/halo_msvc_semantics.h")
    platform_semantics_header = Path("build/linux/platform_msvc_semantics.h")
    prefix_header = LINUX_DIR / "include" / "halo_linux_prefix.h"
    image = BUILD / f"halo_guest_{arch}.elf"
    gl_include = ANGLE_DIR / "include"
    target, emulation = GUEST_ELF[arch]
    if arch == "x86_64":
        arch_dirs = [guest_dir / "arch", MUSL_DIR / "arch" / "x32"]
        syscall_source = MUSL_DIR / "arch" / "x32" / "bits" / "syscall.h.in"
        musl_extra = [MUSL_DIR / "src" / "fenv" / "x32" / "fenv.s",
                      *(source for source in sorted((MUSL_DIR / "src" / "fenv").glob("*.c"))
                        if source.name != "fenv.c")]
        src_include: List[str] = []
    else:
        arch_dirs = [arm64_32]
        syscall_source = arm64_32 / "bits" / "syscall.h.in"
        musl_extra = []
        src_include = [f"-I{LIBC_DIR}/src_include"]


    alltypes = libc_include / "bits" / "alltypes.h"
    syscall_h = libc_include / "bits" / "syscall.h"
    version_h = libc_internal / "version.h"
    n.build(outputs=alltypes, rule="macos_alltypes",
            inputs=[arm64_32 / "bits" / "alltypes.h.in", MUSL_DIR / "include" / "alltypes.h.in"])
    n.build(outputs=syscall_h, rule="macos_syscall_h", inputs=syscall_source)
    n.build(outputs=version_h, rule="macos_version_h")
    arch_headers = []
    if arch == "x86_64":
        for name in ("syscall_arch.h", "pthread_arch.h"):
            arch_headers.append(guest_dir / "arch" / name)
            n.build(outputs=arch_headers[-1], rule="macos_copy", inputs=arm64_32 / name)

    guest_gl_c = gen_dir / "guest_gl.c"
    gl_imports = gen_dir / "gl_imports.list"
    n.build(outputs=[guest_gl_c, gl_imports], rule="macos_gl_stubs", variables={"arch": arch},
            implicit=[Path("tools/macos_gl_stubs.py"), LINUX_DIR / "src" / "gl.h"])
    guest_posix_c = gen_dir / "guest_posix.c"
    posix_imports = gen_dir / "posix_imports.list"
    n.build(outputs=[guest_posix_c, posix_imports], rule="macos_posix_stubs",
            implicit=[Path("tools/macos_posix_stubs.py"), LINUX_DIR / "src" / "posix.h"])
    imports_s = gen_dir / "imports.s"
    host_table_c = gen_dir / "host_import_table.c"
    n.build(outputs=[host_table_c, imports_s], rule="macos_imports", variables={"arch": arch},
            inputs=[PORT_DIR / "host_imports.list", posix_imports, gl_imports],
            implicit=[Path("tools/macos_imports.py")])

    generated_headers = [*xdk_headers(), alltypes, syscall_h, version_h, *arch_headers,
                         semantics_header, platform_semantics_header]


    libc_includes = [f"-isystem {libc_include}", *(f"-isystem {directory}" for directory in arch_dirs),
                     f"-isystem {MUSL_DIR}/arch/generic", f"-isystem {MUSL_DIR}/include",
                     f"-idirafter {resource_dir}/include"]
    guest_abi = " ".join(GUEST_ABI_FLAGS[arch] + configuration_defines(sln))
    guest_code = " ".join(GUEST_CODE_FLAGS)
    rule = "macos_guest_cc_arm64" if arch == "arm64" else "macos_guest_cc"
    tool_implicit = [*generated_headers, *([plugin, Path("tools/macos_asm_convert.py")] if arch == "arm64" else [])]

    def guest_object(source: Path, cflags: str, prefix: str = "") -> Path:
        relative = source.relative_to(BUILD) if str(source).startswith(str(BUILD)) else Path(str(source).lstrip("/"))
        obj = obj_dir / prefix / relative.with_suffix(".o")
        n.build(outputs=obj, rule=rule, inputs=source, implicit=tool_implicit, variables={"cflags": cflags})
        return obj

    libc_internal_includes = [
        *(f"-I{directory}" for directory in arch_dirs), f"-I{MUSL_DIR}/arch/generic", f"-I{libc_internal}",
        *src_include, f"-I{MUSL_DIR}/src/include", f"-I{MUSL_DIR}/src/internal", f"-I{libc_include}",
        f"-I{MUSL_DIR}/include",
    ]
    musl_cflags = " ".join([guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-w",
                            *libc_internal_includes])
    musl_objects = [guest_object(source, musl_cflags, "musl") for source in musl_sources() + musl_extra]
    libguestc = guest_dir / "libguestc.a"
    n.build(outputs=libguestc, rule="macos_ar", inputs=musl_objects)

    objects: List[Path] = []
    game_cflags = " ".join([
        guest_abi, guest_code, " ".join(GAME_FLAGS),
        f"-include {prefix_header}", f"-include {semantics_header}",
        f"-I{LINUX_DIR}/include", f"-iquote {Path(config['game_sources'])}",
        game_defines_and_includes(config), *libc_includes, f"-idirafter {XDK_INCLUDE}",
    ])
    for source in game_sources(config):
        cflags = game_cflags
        if source.as_posix() == "source/main/main.c":
            cflags += " " + updater_defines(getattr(sln, "port_release", False))
        if arch == "arm64" and source.as_posix() in VARIADIC_PROTOTYPE_FILES:
            cflags += f" -include {PORT_DIR}/include/halo_variadic_prototypes.h"
        objects.append(guest_object(source, cflags))
    for source in sorted(Path(config["game_sources"]).glob("*.c")):
        objects.append(guest_object(source, game_cflags))
    objects.append(guest_object(STB_VORBIS_SOURCE, game_cflags))

    platform_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE", "-DHALO_LINUX_PLATFORM_LAYER", "-w",
        f"-include {prefix_header}", f"-include {platform_semantics_header}",
        f"-I{LINUX_DIR}/src", f"-I{LINUX_DIR}/include", f"-I{runtime_dir}", f"-I{PORT_DIR}/include",
        f"-I{TOML_DIR}", f"-I{EXPAT_DIR}", f"-I{KCP_DIR}", f"-I{MONOCYPHER_DIR}",
        f"-I{ZLIB_DIR}", "-Isource -Isource/cseries",
        f"-I{SDL_DIR}/include", f"-isystem {gl_include}", *libc_includes, f"-idirafter {XDK_INCLUDE}",
    ])
    for source in sorted((LINUX_DIR / "src").glob("*.c")):
        if source.name.startswith("posix_") or source.name == "memory_watch.c":
            continue
        objects.append(guest_object(source, platform_cflags))
    for source in hud_assets_build(n, f"macos_{arch}", gen_dir / "hud_hires_assets.c"):
        objects.append(guest_object(source, platform_cflags))
    objects.append(guest_object(TOML_DIR / "tomlc17.c", platform_cflags))
    for name in EXPAT_SOURCES:
        objects.append(guest_object(EXPAT_DIR / name, platform_cflags))
    objects.append(guest_object(KCP_DIR / "ikcp.c", platform_cflags))
    for source in opus_sources():
        objects.append(guest_object(source, " ".join([opus_cflags(guest_abi), *libc_includes])))
    for name in ("monocypher.c", "monocypher-ed25519.c"):
        objects.append(guest_object(MONOCYPHER_DIR / name, platform_cflags))
    for name in ZLIB_SOURCES:
        objects.append(guest_object(ZLIB_DIR / name, " ".join([platform_cflags, *ZLIB_DEFINES,
                                                                       "-U__ARM_FEATURE_CRC32"])))
    musl_math_cflags = " ".join([guest_abi, "-std=gnu11", "-w", *libc_includes, f"-I{MUSL_MATH_DIR}/include",
                                 f"-include {MUSL_MATH_DIR}/include/libm.h"])
    for source in musl_math_sources():
        objects.append(guest_object(source, musl_math_cflags))
    runtime_internal_cflags = " ".join([
        guest_abi, "-std=c99", "-ffreestanding", "-fno-common", "-D_XOPEN_SOURCE=700", "-D_GNU_SOURCE",
        f"-I{runtime_dir}", f"-I{PORT_DIR}/include", *libc_internal_includes,
    ])
    runtime_cflags = " ".join([
        guest_abi, guest_code, "-std=gnu11", "-D_GNU_SOURCE", f"-I{runtime_dir}", f"-I{PORT_DIR}/include",
        f"-I{LINUX_DIR}/src", f"-I{SDL_DIR}/include", f"-isystem {gl_include}", *libc_includes,
    ])
    for source in sorted(runtime_dir.glob("*.c")):
        if source.name in ("guest_thread.c", "guest_start.c"):
            objects.append(guest_object(source, runtime_internal_cflags))
        elif source.name == "guest_memory_watch.c":
            objects.append(guest_object(source, platform_cflags))
        else:
            objects.append(guest_object(source, runtime_cflags))
    for source in sorted((PORT_DIR / "guest").glob("*.c")):
        objects.append(guest_object(source, runtime_cflags))
    objects.append(guest_object(guest_gl_c, runtime_cflags))
    objects.append(guest_object(guest_posix_c, runtime_cflags))
    imports_o = obj_dir / "gen" / "imports.o"
    n.build(outputs=imports_o, rule="macos_guest_as", inputs=imports_s, variables={"target": target})
    objects.append(imports_o)


    n.build(outputs=image, rule="macos_guest_link", inputs=objects,
            implicit=[libguestc, PORT_DIR / "guest" / "guest.ld"],
            variables={"emulation": emulation, "libc": str(libguestc)})
    n.build(outputs=f"macos_guest_{arch}", rule="phony", inputs=image)
    return image, syscall_h, host_table_c
