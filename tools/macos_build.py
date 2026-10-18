"""Ninja rules for the macOS build (``ninja macos``).

The macOS host runs the game's guest image in a VM under Hypervisor.framework
(docs/macos_host.md). This graph builds

- the guest image, build/macos/halo_guest.elf, with the Android build's rules
  (tools/android_build.py's guest_image_build): the same sources and flags,
  but import stubs that exit to the host with an svc (tools/android_imports.py
  --svc);
- the host, build/macos/halo_host, a native arm64 executable signed ad hoc
  with the com.apple.security.hypervisor entitlement.

The tools come from the environment that `nix develop` sets (flake.nix):
HALO_MACOS_GUEST_CC (a clang with the arm64_32 target, not the Nix wrapper,
whose macOS flags would reach the guest), HALO_MACOS_LLD, HALO_MACOS_AR,
HALO_MACOS_GLES_INCLUDE (GLES2, GLES3 and KHR), HALO_MACOS_SDL_INCLUDE,
HALO_MACOS_SDL_LIB and HALO_MACOS_CODESIGN. Without the headers the graph is
left out.

For now the guest is the Android guest (HALO_ANDROID, and the SDL calls
port/android/guest/runtime/guest_sdl.c bridges): its configuration is the
one known to build. The desktop's paths (the window's modes, the mouse, the
audio devices) come with the macOS host's SDL bridge.
"""

import json
import os
import platform
import subprocess
import sys
from pathlib import Path
from typing import Any, List

from .android_build import MUSL_VERSION, fetch_musl, guest_image_build
from .ninja_syntax import Writer

PORT_DIR = Path("port/macos")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/macos")
THIRD_PARTY = BUILD / "third_party"
MUSL_DIR = THIRD_PARTY / f"musl-{MUSL_VERSION}"
ENTITLEMENTS = PORT_DIR / "host" / "halo_host.entitlements"
HOST_FRAMEWORKS = ["Hypervisor"]


def is_macos_host() -> bool:
    return sys.platform == "darwin" and platform.machine() == "arm64"


def macos_configure_inputs() -> List[Path]:
    return [Path(__file__), PORT_DIR / "host"]


def generate_macos_build(n: Writer, sln: Any) -> None:
    config_path = LINUX_DIR / "port.json"
    if not is_macos_host() or not config_path.is_file() or not (PORT_DIR / "host").is_dir():
        return
    environment = os.environ
    guest_cc = getattr(sln, "macos_guest_cc", None) or environment.get("HALO_MACOS_GUEST_CC") or "clang"
    gles_include = environment.get("HALO_MACOS_GLES_INCLUDE")
    sdl_include = environment.get("HALO_MACOS_SDL_INCLUDE")
    if not gles_include or not sdl_include:
        n.comment("macOS build: no OpenGL ES or SDL3 headers (run configure.py in `nix develop`)")
        return
    try:
        fetch_musl(THIRD_PARTY)
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"macOS build disabled: cannot fetch musl ({error})", file=sys.stderr)
        return
    config = json.loads(config_path.read_text(encoding="utf-8"))
    lld = environment.get("HALO_MACOS_LLD", "ld.lld")
    ar = environment.get("HALO_MACOS_AR", "llvm-ar")
    sdl_lib = environment.get("HALO_MACOS_SDL_LIB")
    codesign = environment.get("HALO_MACOS_CODESIGN", "codesign")
    host_cc = environment.get("CC", "cc")

    n.comment("macOS build (ninja macos); see docs/macos_host.md")
    n.variable("macos_guest_cc", guest_cc)
    n.variable("macos_host_cc", host_cc)

    host_dir = BUILD / "host"
    names_c = host_dir / "host_import_names.c"
    image = guest_image_build(n, sln, config, rule_prefix="macos", build=BUILD, guest_cc=guest_cc,
                              musl_dir=MUSL_DIR, sdl_include=Path(sdl_include), gl_headers=Path(gles_include),
                              ar=ar, lld=lld, builtins="", imports_options=f"--svc --host-names {names_c}",
                              imports_outputs=[names_c])

    # ---------- the host

    n.rule(
        name="macos_host_cc",
        command="$macos_host_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    host_cflags = " ".join(["-O2", "-g", "-Wall", "-Werror", "-std=gnu11", f"-I{PORT_DIR}/host"])
    host_objects: List[Path] = []
    for source in [*sorted((PORT_DIR / "host").glob("*.c")), names_c]:
        obj = host_dir / "obj" / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": host_cflags})
        host_objects.append(obj)
    host = BUILD / "halo_host"
    libraries = " ".join(f"-framework {name}" for name in HOST_FRAMEWORKS)
    if sdl_lib:
        libraries += f" -L{sdl_lib} -Wl,-rpath,{sdl_lib}"
    # signed as it is linked, so that no unsigned host is ever left: without
    # the entitlement it cannot create its VM
    n.rule(
        name="macos_host_link",
        command=(f"$macos_host_cc -o $out $in {libraries} && "
                 f"{codesign} -s - -f --entitlements {ENTITLEMENTS} $out"),
        description="MACOS HOST LINK $out",
    )
    n.build(outputs=host, rule="macos_host_link", inputs=host_objects, implicit=[ENTITLEMENTS])

    n.build(outputs="macos", rule="phony", inputs=[image, host])
    n.newline()
