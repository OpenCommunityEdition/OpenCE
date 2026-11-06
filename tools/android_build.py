"""Ninja rules for the Android build (``ninja android``).

The Android port (port/android/README.md) runs the game as ILP32 AArch64
code - 32-bit pointers, as the game's data formats require - inside an
ordinary 64-bit Android app. This graph builds

- the guest image, build/android/halo_guest.elf: the game sources, the
  platform layer shared with the Linux port (port/linux/src) and the guest
  runtime (port/guest) with a subset of musl as its C library, all
  compiled by clang for arm64_32-apple-watchos, converted to ELF assembly
  (tools/guest_asm_convert.py), assembled for AArch64 and linked at a fixed
  address below 4 GB;
- the host library, build/android/jniLibs/arm64-v8a/libmain.so, with the
  Android NDK: the loader and the services the guest calls, over SDL3;
- SDL3 itself, with the NDK's CMake toolchain file;

and stages both, with the SDL3 Java sources, for the Gradle project in
port/android/app, which ``ninja android_apk`` then assembles.
"""

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

from .embed_assets import hud_configure_inputs
from .guest_build import (
    MUSL_VERSION,
    GuestTarget,
    fetch_musl,
    fetch_sdl,
    generate_guest,
    host_network_objects,
)
from .ninja_syntax import Writer

PORT_DIR = Path("port/android")
GUEST_DIR = Path("port/guest")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/android")
THIRD_PARTY = BUILD / "third_party"
# the TOML parser config.toml is read with (port/linux/src/port_config.c)
TOML_DIR = Path("port/third_party/tomlc17")
MUSL_DIR = THIRD_PARTY / f"musl-{MUSL_VERSION}"
SDL_DIR = THIRD_PARTY / "SDL3"
SDL_ANDROID_MOUSE_PATCH = Path("port/android/patches/sdl-relative-mouse.patch")
SDL_ANDROID_MOUSE_LISTENER = "android-project/app/src/main/java/org/libsdl/app/SDLControllerManager.java"
ANDROID_API = 28

HOST_LIBRARIES = ["SDL3", "GLESv3", "EGL", "log", "android", "m", "dl"]


def _quote(path: Any) -> str:
    text = str(path).replace(os.sep, "/")
    return f'"{text}"' if " " in text else text


def _find_ndk() -> Path | None:
    for variable in ("ANDROID_NDK_HOME", "ANDROID_NDK_ROOT", "ANDROID_NDK"):
        if os.environ.get(variable) and Path(os.environ[variable]).is_dir():
            return Path(os.environ[variable])
    for variable in ("ANDROID_HOME", "ANDROID_SDK_ROOT"):
        sdk = os.environ.get(variable)
        if sdk and (Path(sdk) / "ndk").is_dir():
            versions = sorted((Path(sdk) / "ndk").iterdir())
            if versions:
                return versions[-1]
    for sdk in (Path.home() / "Android/Sdk", Path("/opt/android-sdk")):
        if (sdk / "ndk").is_dir():
            versions = sorted((sdk / "ndk").iterdir())
            if versions:
                return versions[-1]
    return None


def fetch_third_party() -> None:
    """Download musl and SDL3 (configure time, once)."""
    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    fetch_musl(THIRD_PARTY)
    fetch_sdl(THIRD_PARTY)
    # SDL 3.4.16's generic mouse listener drops captured relative motion and
    # button transitions unless they are forwarded from captured pointer events.
    # A tree patched by another version of the patch (an older checkout, or
    # CI's cached one) is put back as SDL has it before this one is applied.
    reverse = subprocess.run(
        ["git", "-C", str(SDL_DIR), "apply", "--reverse", "--check", str(SDL_ANDROID_MOUSE_PATCH.resolve())],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False,
    )
    if reverse.returncode != 0:
        subprocess.run(["git", "-C", str(SDL_DIR), "checkout", "--", SDL_ANDROID_MOUSE_LISTENER], check=True)
        subprocess.run(["git", "-C", str(SDL_DIR), "apply", str(SDL_ANDROID_MOUSE_PATCH.resolve())], check=True)


def android_configure_inputs() -> list[Path]:
    return [Path(__file__), SDL_ANDROID_MOUSE_PATCH, GUEST_DIR / "runtime", PORT_DIR / "host",
            LINUX_DIR / "src", *hud_configure_inputs()]


def generate_android_build(n: Writer, sln: Any) -> None:
    config_path = LINUX_DIR / "port.json"
    if not config_path.is_file() or not (PORT_DIR / "host").is_dir():
        return
    ndk = Path(sln.android_ndk) if getattr(sln, "android_ndk", None) else _find_ndk()
    if not ndk or not ndk.is_dir():
        n.comment("Android build: no NDK found (set ANDROID_NDK_HOME or pass --android-ndk)")
        return
    try:
        fetch_third_party()
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"Android build disabled: cannot fetch musl/SDL3 ({error})", file=sys.stderr)
        return
    config: dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))

    prebuilt = ndk / "toolchains" / "llvm" / "prebuilt"
    _host_tag = os.environ.get("ANDROID_NDK_HOST_TAG", "")
    if not _host_tag:
        if sys.platform == "darwin":
            _host_tag = "darwin-x86_64"
        elif os.name == "nt":
            _host_tag = "windows-x86_64"
        else:
            _host_tag = "linux-x86_64"
    if not (prebuilt / _host_tag).is_dir():
        # an NDK that names its host folder differently: take the one there is
        _tags = sorted(entry.name for entry in prebuilt.iterdir() if entry.is_dir()) if prebuilt.is_dir() else []
        if not _tags:
            n.comment("Android build: the NDK has no LLVM toolchain")
            return
        _host_tag = _tags[0]
    toolchain = prebuilt / _host_tag
    sysroot_include = toolchain / "sysroot" / "usr" / "include"
    host_cc = toolchain / "bin" / f"aarch64-linux-android{ANDROID_API}-clang"
    ndk_bin = toolchain / "bin"
    guest_cc = getattr(sln, "android_guest_cc", None) or "clang"

    n.comment("Android build (ninja android); see port/android/README.md")
    n.variable("android_guest_cc", guest_cc)
    n.variable("android_host_cc", str(host_cc))
    n.variable("android_ndk_bin", str(ndk_bin))

    # the guest image: arm64_32 code with musl, assembled for AArch64
    target = GuestTarget(
        name="android",
        label="ANDROID",
        port_dir=GUEST_DIR,
        build=BUILD,
        third_party=THIRD_PARTY,
        musl_dir=MUSL_DIR,
        sdl_dir=SDL_DIR,
        gl_headers=sysroot_include,
        guest_cc=guest_cc,
        ar="$android_ndk_bin/llvm-ar",
        ld="$android_ndk_bin/ld.lld",
        libgcc="$$($android_host_cc -print-libgcc-file-name)",
        asm_triple="aarch64-linux-android",
        host_imports_list=GUEST_DIR / "host_imports.list",
        host_table_c=BUILD / "host" / "host_import_table.c",
    )
    image, host_table_c = generate_guest(n, sln, config, target)

    # the paths of what the guest image needs and of what the sections after
    # it use
    sdl_build = BUILD / "sdl3-build"
    libsdl = sdl_build / "libSDL3.so"
    jni_dir = BUILD / "jniLibs" / "arm64-v8a"
    libmain = jni_dir / "libmain.so"
    assets_dir = BUILD / "assets"

    # ---------- SDL3

    n.rule(
        name="android_sdl3",
        command=(f"cmake -S {SDL_DIR} -B {sdl_build} -G Ninja "
                 f"-DCMAKE_TOOLCHAIN_FILE={ndk}/build/cmake/android.toolchain.cmake "
                 f"-DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-{ANDROID_API} -DCMAKE_BUILD_TYPE=Release "
                 f"-DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF "
                 # 16 KB pages (Android 15 and later), as the host library
                 f"-DCMAKE_SHARED_LINKER_FLAGS=-Wl,-z,max-page-size=16384 "
                 f"> {BUILD}/sdl3-configure.log && ninja -C {sdl_build} > {BUILD}/sdl3-build.log"),
        description="ANDROID SDL3",
        pool="console",
    )
    n.build(outputs=libsdl, rule="android_sdl3", implicit=[SDL_DIR / "CMakeLists.txt"])

    # ---------- the host library

    host_objects: list[Path] = []
    host_obj_dir = BUILD / "host" / "obj"
    n.rule(
        name="android_host_cc",
        command="$android_host_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="ANDROID HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    host_cflags = " ".join([
        "-O2", "-g", "-fPIC", "-Wall", "-Wno-unused-function", "-D_GNU_SOURCE",
        f"-I{GUEST_DIR}/include", f"-I{PORT_DIR}/host", f"-I{SDL_DIR}/include", f"-I{LINUX_DIR}/src",
        f"-I{TOML_DIR}",
    ])
    host_sources = [*sorted((PORT_DIR / "host").glob("*.c")),
        LINUX_DIR / "src" / "posix_files.c", LINUX_DIR / "src" / "posix_net.c",
        # the app reads debug.sample_seconds from config.toml (host_main.c)
        TOML_DIR / "tomlc17.c",
    ]
    for source in host_sources:
        obj = host_obj_dir / (source.name + ".o")
        n.build(outputs=obj, rule="android_host_cc", inputs=source, variables={"cflags": host_cflags})
        host_objects.append(obj)
    # internet play's UPnP (posix_upnp.c, with port/third_party/miniupnpc),
    # as the other posix_*.c in the host
    host_objects += host_network_objects(n, "android_host_cc", host_obj_dir, host_cflags)
    table_obj = host_obj_dir / "host_import_table.c.o"
    n.build(outputs=table_obj, rule="android_host_cc", inputs=host_table_c, variables={"cflags": host_cflags})
    host_objects.append(table_obj)
    n.rule(
        name="android_host_link",
        command=(f"$android_host_cc -shared -o $out $in -L{sdl_build} "
                 + " ".join(f"-l{lib}" for lib in HOST_LIBRARIES)
                 + " -Wl,-z,max-page-size=16384 -Wl,--no-undefined"),
        description="ANDROID HOST LINK $out",
    )
    n.build(outputs=libmain, rule="android_host_link", inputs=host_objects, implicit=[libsdl])

    # ---------- staging for Gradle

    staged_sdl = jni_dir / "libSDL3.so"
    staged_image = assets_dir / "halo_guest.elf"
    n.rule(name="android_copy", command="cp $in $out", description="ANDROID STAGE $out")
    n.build(outputs=staged_sdl, rule="android_copy", inputs=libsdl)
    n.build(outputs=staged_image, rule="android_copy", inputs=image)
    # internet play's MQTT brokers, in the APK: the app writes them beside
    # config.toml (port/android/host/host_main.c)
    staged_brokers = assets_dir / "brokers.txt"
    n.build(outputs=staged_brokers, rule="android_copy", inputs=Path("port/assets/network/brokers.txt"))
    n.build(outputs="android", rule="phony", inputs=[libmain, staged_sdl, staged_image, staged_brokers])

    apk = PORT_DIR / "app" / "build" / "outputs" / "apk" / "debug" / "app-debug.apk"
    sdl_android_mouse_listener = SDL_DIR / SDL_ANDROID_MOUSE_LISTENER
    n.rule(
        name="android_gradle",
        # Gradle leaves the APK alone when its contents would not change
        command=(f"cd {PORT_DIR} && ./gradlew --console=plain -q assembleDebug && "
                 "touch app/build/outputs/apk/debug/app-debug.apk"),
        description="ANDROID GRADLE $out",
        pool="console",
    )
    n.build(outputs=apk, rule="android_gradle", inputs=[libmain, staged_sdl, staged_image, staged_brokers],
            implicit=[sdl_android_mouse_listener])
    n.build(outputs="android_apk", rule="phony", inputs=apk)
    n.newline()
