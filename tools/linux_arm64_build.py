"""Ninja rules for the guest image of the native Linux arm64 port.

The native Linux arm64 port runs the game as the same ILP32 AArch64 guest
image as the Android port (tools/guest_build.py), but built and linked with
the machine's own clang, ld.lld and llvm-ar, and with Khronos's OpenGL ES
headers: no Android code, and no NDK. This graph builds
build/linux-arm64/halo_guest.elf; ``ninja linux_arm64_guest``.
"""

import json
import platform
import subprocess
import sys
from pathlib import Path
from typing import Any

from .guest_build import (
    LINUX_DIR,
    MUSL_VERSION,
    GuestTarget,
    fetch_musl,
    fetch_sdl,
    generate_guest,
)
from .ninja_syntax import Writer

BUILD = Path("build/linux-arm64")
THIRD_PARTY = BUILD / "third_party"
GL_HEADERS_DIR = THIRD_PARTY / "gl-headers"
# the TOML parser config.toml is read with, as for the Android host
TOML_DIR = Path("port/third_party/tomlc17")
LINUX_ARM64_HOST = Path("port/linux-arm64") / "host"
ANDROID_HOST = Path("port/android") / "host"

# The Khronos registries' commits the OpenGL ES headers are taken from, and
# the headers themselves
GL_REGISTRY = "https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/6af574a14089ccfee87efe230ebcdd8742859813/api"
EGL_REGISTRY = "https://raw.githubusercontent.com/KhronosGroup/EGL-Registry/e964211ff5f7c2bcada786954a16d5b813379395/api"
GL_HEADERS = [
    (GL_REGISTRY, "GLES2/gl2.h"),
    (GL_REGISTRY, "GLES2/gl2ext.h"),
    (GL_REGISTRY, "GLES2/gl2platform.h"),
    (GL_REGISTRY, "GLES3/gl3.h"),
    (GL_REGISTRY, "GLES3/gl31.h"),
    (GL_REGISTRY, "GLES3/gl32.h"),
    (GL_REGISTRY, "GLES3/gl3platform.h"),
    (EGL_REGISTRY, "KHR/khrplatform.h"),
]


def fetch_gl_headers() -> Path:
    """Download the OpenGL ES headers (configure time, once)."""
    for registry, name in GL_HEADERS:
        path = GL_HEADERS_DIR / name
        if path.is_file():
            continue
        url = f"{registry}/{name}"
        print(f"Downloading {url}")
        path.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["curl", "-sSfL", "-o", str(path), url], check=True)
    return GL_HEADERS_DIR


def linux_arm64_configure_inputs() -> list[Path]:
    return [Path(__file__)]


def linux_arm64_host_build(n: Writer, host_table_c: Path) -> None:
    """The host the guest runs in (``ninja linux_arm64_host``).

    The Android host's sources, with the Android-only parts guarded out in
    the sources themselves, built with the system's SDL3 through pkg-config.
    The host import table generate_guest returns is compiled as it is for the
    Android host, as is tomlc17, config.toml's parser (the command lines of
    tools/android_build.py, with the system's compiler).
    """
    n.comment("Native Linux arm64 host build (ninja linux_arm64_host)")
    n.variable("linux_arm64_host_cc", "cc")
    n.variable(
        "linux_arm64_host_cflags",
        " ".join(
            [
                "-O2",
                "-std=gnu11",
                f"-I{ANDROID_HOST.parent}/include",
                f"-I{ANDROID_HOST}",
                f"-I{TOML_DIR}",
                "$$(pkg-config --cflags sdl3)",
            ]
        ),
    )
    obj_dir = BUILD / "host" / "obj"
    n.rule(
        name="linux_arm64_host_cc",
        command="$linux_arm64_host_cc $linux_arm64_host_cflags -c $in -o $out",
        description="LINUX ARM64 HOST CC $out",
    )
    host_sources = [
        LINUX_ARM64_HOST / "host_linux_support.c",
        LINUX_ARM64_HOST / "host_linux_main.c",
        *[
            ANDROID_HOST / name
            for name in (
                "host_loader.c",
                "host_memory.c",
                "host_watch_hash.c",
                "host_syscall.c",
                "host_thread.c",
                "host_debug.c",
                "host_sdl.c",
                "host_gl.c",
                "host_https.c",
            )
        ],
        host_table_c,
        TOML_DIR / "tomlc17.c",
    ]
    host_objects: list[Path] = []
    for source in host_sources:
        obj = obj_dir / (source.name + ".o")
        n.build(outputs=obj, rule="linux_arm64_host_cc", inputs=source)
        host_objects.append(obj)
    n.rule(
        name="linux_arm64_host_link",
        command="$linux_arm64_host_cc $in -o $out $$(pkg-config --libs sdl3) -lm -lpthread -ldl",
        description="LINUX ARM64 HOST LINK $out",
    )
    n.build(
        outputs=BUILD / "halo_host", rule="linux_arm64_host_link", inputs=host_objects
    )
    n.build(outputs="linux_arm64_host", rule="phony", inputs=BUILD / "halo_host")


def generate_linux_arm64_build(n: Writer, sln: Any) -> None:
    lld_bin = getattr(sln, "guest_lld_bin", None)
    if not lld_bin:
        return
    if not all((Path(lld_bin) / name).is_file() for name in ("ld.lld", "llvm-ar")):
        n.comment(
            "Native Linux arm64 guest build: --guest-lld-bin, a directory with ld.lld and llvm-ar"
        )
        return
    config_path = LINUX_DIR / "port.json"
    if not config_path.is_file():
        return
    try:
        THIRD_PARTY.mkdir(parents=True, exist_ok=True)
        fetch_gl_headers()
        fetch_musl(THIRD_PARTY)
        fetch_sdl(THIRD_PARTY)
    except (subprocess.CalledProcessError, OSError) as error:
        print(
            f"Native Linux arm64 guest build disabled: cannot fetch musl/SDL3/GL headers ({error})",
            file=sys.stderr,
        )
        return
    config: dict[str, Any] = json.loads(config_path.read_text(encoding="utf-8"))

    guest_cc = getattr(sln, "android_guest_cc", None) or "clang"
    n.comment("Native Linux arm64 guest build (ninja linux_arm64_guest)")
    n.variable("linux_arm64_guest_cc", guest_cc)

    target = GuestTarget(
        name="linux_arm64",
        label="LINUX ARM64",
        port_dir=Path("port/android"),
        build=BUILD,
        third_party=THIRD_PARTY,
        musl_dir=THIRD_PARTY / f"musl-{MUSL_VERSION}",
        sdl_dir=THIRD_PARTY / "SDL3",
        gl_headers=GL_HEADERS_DIR,
        guest_cc=guest_cc,
        ar=str(Path(lld_bin) / "llvm-ar"),
        ld=str(Path(lld_bin) / "ld.lld"),
        libgcc="",
        asm_triple="aarch64-unknown-linux-gnu",
        host_imports_list=Path("port/android/host_imports.list"),
        host_table_c=BUILD / "host" / "host_import_table.c",
    )
    image, host_table_c = generate_guest(n, sln, config, target)
    n.build(outputs="linux_arm64_guest", rule="phony", inputs=image)
    # the host is built only on Linux arm64, with the system's SDL3
    if platform.system() == "Linux" and platform.machine().lower() in (
        "aarch64",
        "arm64",
    ):
        linux_arm64_host_build(n, host_table_c)
    n.newline()
