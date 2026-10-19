"""Ninja rules for the macOS build (``ninja macos``).

The macOS host runs the game's guest image in a VM under Hypervisor.framework
(docs/macos_host.md). This graph builds

- the guest image, build/macos/halo_guest.elf, with the Android build's rules
  (tools/android_build.py's guest_image_build): the same sources and flags,
  but import stubs that exit to the host with an svc (tools/android_imports.py
  --svc);
- the host, build/macos/halo_host, a native arm64 executable signed ad hoc
  with the com.apple.security.hypervisor entitlement;
- the host's runtime, the Rust crate port/macos/runtime: its static library
  (build/macos/cargo/release/libopence_runtime.a) and the C header cbindgen
  writes for it (build/macos/runtime/opence_runtime.h).

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

from .android_build import MBEDTLS_DIR, MUSL_VERSION, fetch_musl, guest_image_build
from .linux_build import MINIUPNPC_DEFINES, MINIUPNPC_DIR, miniupnpc_sources
from .ninja_syntax import Writer

PORT_DIR = Path("port/macos")
ANDROID_DIR = Path("port/android")
LINUX_DIR = Path("port/linux")
BUILD = Path("build/macos")
THIRD_PARTY = BUILD / "third_party"
MUSL_DIR = THIRD_PARTY / f"musl-{MUSL_VERSION}"
ENTITLEMENTS = PORT_DIR / "host" / "halo_host.entitlements"
HOST_FRAMEWORKS = ["Hypervisor"]
# the host's oldest macOS (conductor/tech-stack.md: macOS 26; os_sync_wait_on_address needs 14.4)
MACOS_MINIMUM = "26.0"
APP_DIR = PORT_DIR / "app"
APP_VERSION = "0.1.0"


def is_macos_host() -> bool:
    return sys.platform == "darwin" and platform.machine() == "arm64"


def macos_configure_inputs() -> List[Path]:
    return [Path(__file__), PORT_DIR / "host", PORT_DIR / "cli", APP_DIR / "Sources" / "OpenCE",
            APP_DIR / "Sources" / "OpenCEShell", APP_DIR / "Sources" / "OpenCEWindow",
            APP_DIR / "Sources" / "OpenCEAudio", APP_DIR / "Tests",
            PORT_DIR / "runtime" / "src", Path("tools/macos_dispatch.py")]


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
                              imports_outputs=[names_c],
                              # the guest's code paths for the VM (the thread
                              # pointer in TPIDR_EL0, port/android/guest/runtime)
                              defines=["-DHALO_GUEST_VM=1"])

    # ---------- the host

    n.rule(
        name="macos_host_cc",
        command="$macos_host_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="MACOS HOST CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    # the runtime crate (below), whose header the host includes
    runtime_dir = PORT_DIR / "runtime"
    runtime_library = BUILD / "cargo" / "release" / "libopence_runtime.a"
    runtime_header = BUILD / "runtime" / "opence_runtime.h"

    # the import dispatcher, from the stubs' lists and the imports' prototypes
    dispatch_c = host_dir / "host_dispatch.c"
    gen_dir = BUILD / "guest" / "gen"
    import_lists = [ANDROID_DIR / "host_imports.list", gen_dir / "posix_imports.list", gen_dir / "gl_imports.list"]
    prototype_headers = [ANDROID_DIR / "guest" / "runtime" / "guest_host.h", LINUX_DIR / "src" / "posix.h"]
    n.rule(
        name="macos_dispatch",
        command=("$python tools/macos_dispatch.py "
                 + " ".join(f"--prototypes {header}" for header in prototype_headers)
                 + " $out $in"),
        description="MACOS DISPATCH $out",
    )
    n.build(outputs=dispatch_c, rule="macos_dispatch", inputs=import_lists,
            implicit=[Path("tools/macos_dispatch.py"), *prototype_headers])

    host_cflags = " ".join([
        "-O2", "-g", "-Wall", "-Werror", "-std=gnu11", "-D_DARWIN_C_SOURCE", f"-mmacosx-version-min={MACOS_MINIMUM}",
        f"-I{PORT_DIR}/host", f"-I{runtime_header.parent}", f"-I{ANDROID_DIR}/include",
        f"-I{ANDROID_DIR}/guest/runtime", f"-I{ANDROID_DIR}/guest/libc/arch/arm64_32", f"-I{LINUX_DIR}/src",
        f"-I{sdl_include}",
    ])
    # the files the Android host shares: its SDL bridge, and the platform
    # layer's POSIX helpers (with UPnP and DTLS, as there)
    shared_cflags = host_cflags.replace("-Werror", "") + " -Wno-unused-function"
    host_objects: List[Path] = []
    for source in [*sorted((PORT_DIR / "host").glob("*.c")), names_c, dispatch_c]:
        obj = host_dir / "obj" / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": host_cflags},
                order_only=[runtime_header])
        host_objects.append(obj)
    for source in [ANDROID_DIR / "host" / "host_sdl.c", LINUX_DIR / "src" / "posix_files.c",
                   LINUX_DIR / "src" / "posix_net.c"]:
        obj = host_dir / "obj" / (source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source, variables={"cflags": shared_cflags},
                order_only=[runtime_header])
        host_objects.append(obj)
    miniupnpc_cflags = " ".join([shared_cflags, f"-I{MINIUPNPC_DIR / 'include'}", f"-I{MINIUPNPC_DIR / 'src'}",
                                 *MINIUPNPC_DEFINES])
    for source in [LINUX_DIR / "src" / "posix_upnp.c", *miniupnpc_sources()]:
        obj = host_dir / "obj" / ("miniupnpc_" + source.name + ".o" if source.parent.parent == MINIUPNPC_DIR
                                  else source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source,
                variables={"cflags": miniupnpc_cflags + (" -w" if source.name != "posix_upnp.c" else "")})
        host_objects.append(obj)
    mbedtls_cflags = " ".join([shared_cflags, f"-I{MBEDTLS_DIR / 'include'}", f"-I{MBEDTLS_DIR / 'library'}"])
    for source in [LINUX_DIR / "src" / "posix_dtls.c", *sorted((MBEDTLS_DIR / "library").glob("*.c"))]:
        obj = host_dir / "obj" / ("mbedtls_" + source.name + ".o" if source.parent.parent == MBEDTLS_DIR
                                  else source.name + ".o")
        n.build(outputs=obj, rule="macos_host_cc", inputs=source,
                variables={"cflags": mbedtls_cflags + (" -w" if source.name != "posix_dtls.c" else "")})
        host_objects.append(obj)
    # the host as a library, which the command-line host and the app link
    host_library = BUILD / "libhalo_host.a"
    n.rule(
        name="macos_host_archive",
        command=f"rm -f $out && {ar} --format=darwin rcs $out $in",
        description="MACOS HOST AR $out",
    )
    n.build(outputs=host_library, rule="macos_host_archive", inputs=host_objects)
    cli_object = BUILD / "cli" / "obj" / "main.c.o"
    n.build(outputs=cli_object, rule="macos_host_cc", inputs=PORT_DIR / "cli" / "main.c",
            variables={"cflags": host_cflags})

    host = BUILD / "halo_host"
    libraries = " ".join(f"-framework {name}" for name in HOST_FRAMEWORKS)
    libraries += f" {runtime_library}"
    if sdl_lib:
        libraries += f" -L{sdl_lib} -Wl,-rpath,{sdl_lib}"
    libraries += " -lSDL3"
    # signed as it is linked, so that no unsigned host is ever left: without
    # the entitlement it cannot create its VM
    n.rule(
        name="macos_host_link",
        command=(f"$macos_host_cc -mmacosx-version-min={MACOS_MINIMUM} -o $out $in {libraries} && "
                 f"{codesign} -s - -f --entitlements {ENTITLEMENTS} $out"),
        description="MACOS HOST LINK $out",
    )
    n.build(outputs=host, rule="macos_host_link", inputs=[cli_object, host_library],
            implicit=[ENTITLEMENTS, runtime_library])

    # ---------- the app

    # Swift is Xcode's (conductor/tech-stack.md), run outside the Nix
    # toolchain: the environment's SDKROOT and compilers are the Nix SDK's,
    # which Xcode's Swift cannot link against. SwiftPM keeps its caches in
    # the build folder (a Nix build has no home), and the libraries' paths
    # are absolute because the linker runs in the package's folder.
    swift_build = BUILD / "swift"
    swift = environment.get("HALO_MACOS_SWIFT", "/usr/bin/xcrun swift")
    app_codesign = environment.get("HALO_MACOS_APP_CODESIGN", "/usr/bin/codesign")
    app = BUILD / "OpenCE.app"
    app_executable = app / "Contents" / "MacOS" / "OpenCE"
    linker_flags = f'-Xlinker "$$PWD/{host_library}" -Xlinker "$$PWD/{runtime_library}"'
    for name in HOST_FRAMEWORKS:
        linker_flags += f" -Xlinker -framework -Xlinker {name}"
    if sdl_lib:
        linker_flags += f" -Xlinker -L{sdl_lib} -Xlinker -rpath -Xlinker {sdl_lib}"
    linker_flags += " -Xlinker -lSDL3"
    swift_package = (f"env -u SDKROOT -u DEVELOPER_DIR -u MACOSX_DEPLOYMENT_TARGET -u CC -u CXX -u LD "
                     f"{swift} %s --quiet --disable-sandbox --package-path {APP_DIR} "
                     f"--scratch-path {swift_build} --cache-path {swift_build}/cache "
                     f"--config-path {swift_build}/config --security-path {swift_build}/security {linker_flags}")
    # (SwiftPM does not count the libraries given to its linker as inputs: its
    # product is removed first, so that a changed host library is linked)
    n.rule(
        name="macos_app",
        command=(f"rm -f {swift_build}/release/OpenCE && {swift_package % 'build -c release'} && "
                 f"$python tools/macos_app.py {swift_build}/release/OpenCE {image} {ENTITLEMENTS} {app_codesign} "
                 f"{APP_VERSION} {app}"),
        description="MACOS APP $out",
    )
    # The runtime's Swift bindings, generated from its library (whose
    # metadata they must match) into the package's ignored folders.
    # uniffi-bindgen is built in a target folder of its own: its features
    # would otherwise rebuild the library the hosts link.
    bindings_dir = BUILD / "swift_bindings"
    bindings = [APP_DIR / "Sources" / "OpenCERuntime" / "opence_runtime.swift",
                APP_DIR / "Sources" / "opence_runtimeFFI" / "opence_runtimeFFI.h",
                APP_DIR / "Sources" / "opence_runtimeFFI" / "module.modulemap"]
    n.rule(
        name="macos_swift_bindings",
        command=(f"cargo run --quiet --release --features bindgen --bin uniffi-bindgen "
                 f"--manifest-path {runtime_dir}/Cargo.toml --target-dir {BUILD / 'cargo_bindgen'} -- "
                 f"generate --library {runtime_library} --language swift --out-dir {bindings_dir} && "
                 f"mkdir -p {bindings[0].parent} {bindings[1].parent} && "
                 f"cp {bindings_dir}/opence_runtime.swift {bindings[0]} && "
                 f"cp {bindings_dir}/opence_runtimeFFI.h {bindings[1]} && "
                 f"cp {bindings_dir}/opence_runtimeFFI.modulemap {bindings[2]}"),
        description="MACOS UNIFFI $out",
    )
    n.build(outputs=bindings, rule="macos_swift_bindings", inputs=[runtime_library],
            implicit=[runtime_dir / "src" / "bin" / "uniffi-bindgen.rs"])
    swift_sources = [APP_DIR / "Package.swift", *sorted((APP_DIR / "Sources" / "OpenCE").glob("*.swift")),
                     *sorted((APP_DIR / "Sources" / "OpenCEShell").glob("*.swift")),
                     *sorted((APP_DIR / "Sources" / "OpenCEWindow").glob("*.swift")),
                     *sorted((APP_DIR / "Sources" / "OpenCEAudio").glob("*.swift")),
                     APP_DIR / "Sources" / "CHaloHost" / "module.modulemap", PORT_DIR / "host" / "halo_host.h",
                     *bindings]
    n.build(outputs=app_executable, rule="macos_app",
            inputs=[*swift_sources, host_library, runtime_library, image, ENTITLEMENTS],
            implicit=[Path("tools/macos_app.py")])
    n.build(outputs="macos_app", rule="phony", inputs=[app_executable])
    # the package's tests (port/macos/app/Tests), run each time they are asked for
    swift_tests = sorted((APP_DIR / "Tests").rglob("*.swift"))
    n.rule(
        name="macos_app_test",
        command=swift_package % "test",
        description="MACOS SWIFT TEST",
        pool="console",
    )
    n.build(outputs="macos_app_test", rule="macos_app_test",
            inputs=[*swift_sources, *swift_tests, host_library, runtime_library])

    # ---------- the runtime crate

    runtime_inputs = [runtime_dir / "Cargo.toml", runtime_dir / "Cargo.lock",
                      *sorted((runtime_dir / "src").glob("*.rs"))]
    n.rule(
        name="macos_cargo",
        # (cargo reads .cargo/config.toml from the folder it runs in, not the
        # crate's, so the target folder is named here too)
        command=f"cargo build --quiet --release --manifest-path {runtime_dir}/Cargo.toml --target-dir {BUILD / 'cargo'}",
        description="MACOS CARGO $out",
        restat=True,
    )
    n.build(outputs=runtime_library, rule="macos_cargo", implicit=runtime_inputs)
    n.rule(
        name="macos_cbindgen",
        command=f"cbindgen --quiet --config {runtime_dir}/cbindgen.toml --output $out {runtime_dir}",
        description="MACOS CBINDGEN $out",
    )
    n.build(outputs=runtime_header, rule="macos_cbindgen", implicit=[*runtime_inputs, runtime_dir / "cbindgen.toml"])

    n.build(outputs="macos", rule="phony", inputs=[image, host_library, host, runtime_library, runtime_header])
    n.newline()
