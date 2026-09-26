"""Ninja rules for the native Linux build (``ninja linux``).

This is independent of the byte-matching graph: it compiles the same game
sources with clang for 32-bit x86 Linux, adds the platform layer in
``port/linux/src``, and links an ELF executable at ``build/linux/halo``.
Nothing here changes the MSVC objects, objdiff configuration or progress.
See port/linux/README.md for the design.
"""

import json
import os
from pathlib import Path
from typing import Any, Dict, List

from .ninja_syntax import Writer

PORT_DIR = Path("port/linux")
PORT_CONFIG = PORT_DIR / "port.json"

# Flags shared by the game and the XDK-facing half of the platform layer.
# They reproduce the MSVC/Xbox ABI the source was written against:
#  - 16-bit wchar_t (UTF-16 strings in tag data and saved games),
#  - MSVC struct layout for 64-bit members (-malign-double),
#  - MSVC-style __asm blocks, __declspec, __int64 and calling conventions,
#  - C89 with tentative definitions shared between units (-fcommon),
#  - small structures and unions returned in EAX:EDX, as Win32 does
#    (hs_runtime.c calls union-returning converters through pointers typed
#    as returning long),
#  - no optimisations that assume the absence of MSVC-tolerated UB.
LINUX_ABI_FLAGS = [
    "--target=i686-linux-gnu",
    "-m32",
    "-march=pentium3",
    "-fms-extensions",
    "-fshort-wchar",
    "-malign-double",
    "-fcommon",
    "-fno-pic",
    "-fno-strict-aliasing",
    "-fwrapv",
    "-fno-delete-null-pointer-checks",
    "-freg-struct-return",
    # the game keeps EBP frames (MSVC /Oy-): get_return_eip and the stack
    # walker follow the frame chain
    "-fno-omit-frame-pointer",
    "-O2",
    "-g",
    # glibc's wide string functions assume a 32-bit wchar_t; stop clang from
    # turning loops into calls to them (it rewrites a counting loop as
    # wcslen even under -fshort-wchar)
    *(f"-fno-builtin-{name}" for name in (
        "wcslen", "wcsnlen", "wcschr", "wcsrchr", "wcscmp", "wcsncmp", "wcscpy",
        "wcsncpy", "wcscat", "wcsncat", "wmemchr", "wmemcmp", "wmemcpy",
        "wmemmove", "wmemset",
    )),
]

# The game is compiled with glibc restricted to ISO C so that POSIX names
# (random, strnlen, ...) cannot collide with the game's own declarations.
GAME_FLAGS = [
    "-std=gnu89",
    "-D__STRICT_ANSI__",
    "-w",
    "-Wno-error=incompatible-pointer-types",
    "-Wno-error=incompatible-function-pointer-types",
    "-Wno-error=int-conversion",
    "-Wno-error=implicit-function-declaration",
    "-Wno-error=implicit-int",
    "-Wno-error=return-type",
]

PLATFORM_FLAGS = [
    "-std=gnu11",
    "-D_GNU_SOURCE",
    "-DHALO_LINUX_PLATFORM_LAYER",
    "-Wall",
    "-Wno-unused-function",
    "-Wno-unknown-pragmas",
    "-Wno-microsoft-anon-tag",
    "-Wno-pragma-pack",
    "-Wno-ignored-attributes",
    "-Wno-duplicate-decl-specifier",
    "-Wno-missing-braces",
    "-Wno-unused-variable",
    "-Wno-ignored-pragmas",
]

# Platform files named posix_*.c talk to glibc only. They are built with the
# host's native ABI (no -malign-double, no 16-bit wchar_t, no XDK headers) so
# glibc structures such as struct stat have their real layout.
POSIX_FLAGS = [
    "--target=i686-linux-gnu",
    "-m32",
    "-march=pentium3",
    "-std=gnu11",
    "-D_GNU_SOURCE",
    "-D_FILE_OFFSET_BITS=64",
    "-fno-pic",
    "-O2",
    "-g",
    "-Wall",
]


def _load_port_config() -> Dict[str, Any]:
    with open(PORT_CONFIG, "r", encoding="utf-8") as f:
        return json.load(f)


def linux_configure_inputs() -> List[Path]:
    """Files whose change must re-run configure.py."""
    if not PORT_CONFIG.is_file():
        return [Path(__file__)]
    return [PORT_CONFIG, Path(__file__), PORT_DIR / "src", PORT_DIR / "game"]


def _quote(path: Any) -> str:
    text = str(path).replace(os.sep, "/")
    return f'"{text}"' if " " in text else text


def generate_linux_build(n: Writer, sln: Any) -> None:
    if not PORT_CONFIG.is_file():
        # a checkout without the port (or a test fixture): nothing to emit
        return
    config = _load_port_config()
    build_dir: Path = sln.build_dir / "linux"
    overlay_dir = build_dir / "sdk_include"
    overlay_stamp = build_dir / "sdk_include.stamp"
    obj_dir = build_dir / "obj"
    output = build_dir / "halo"
    cc = sln.linux_cc or "clang"
    prefix_header = PORT_DIR / "include" / "halo_linux_prefix.h"
    semantics_header = build_dir / "halo_msvc_semantics.h"
    platform_semantics_header = build_dir / "platform_msvc_semantics.h"

    n.comment("Native Linux build (ninja linux)")
    n.variable("linux_cc", cc)
    n.rule(
        name="linux_sdk_overlay",
        command=f"$python tools/linux_sdk_overlay.py --output {overlay_dir} --stamp $out",
        description="LINUX SDK HEADERS",
    )
    n.build(
        outputs=overlay_stamp,
        rule="linux_sdk_overlay",
        implicit=[Path("tools/linux_sdk_overlay.py")],
    )
    n.rule(
        name="linux_msvc_semantics",
        command="$python tools/linux_msvc_semantics.py --output $out $scan",
        description="LINUX MSVC SEMANTICS $out",
        restat=True,
    )
    game_headers = sorted(
        p for p in Path("source").rglob("*") if p.suffix in (".c", ".h")
    )
    # The game sees its own tags and inline functions plus the XDK's; the
    # platform layer only includes XDK headers (the overlay, which omits the
    # SDK's C runtime headers) and so only needs the XDK's inline functions.
    n.build(
        outputs=semantics_header,
        rule="linux_msvc_semantics",
        implicit=[Path("tools/linux_msvc_semantics.py"), overlay_stamp, *game_headers],
        variables={
            "scan": f"--all-inlines --tags source --inlines source --inlines {overlay_dir}"
        },
    )
    n.build(
        outputs=platform_semantics_header,
        rule="linux_msvc_semantics",
        implicit=[Path("tools/linux_msvc_semantics.py"), overlay_stamp],
        variables={"scan": f"--inlines {overlay_dir}"},
    )
    n.rule(
        name="linux_cc",
        command="$linux_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="LINUX CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    n.rule(
        name="linux_link",
        command=(
            "$linux_cc $ldflags -o $out @$out.rsp $libs"
            " && $python tools/linux_link_check.py $out.rsp $out"
        ),
        description="LINUX LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )

    abi = " ".join(LINUX_ABI_FLAGS + (["-DHALO_RELEASE"] if getattr(sln, "port_release", False) else []))
    port_include = PORT_DIR / "include"
    sdk_flags = f"-idirafter {overlay_dir}"
    excluded = set(config.get("exclude_sources", []))
    objects: List[Path] = []

    def add_object(source: Path, cflags: str) -> None:
        obj = obj_dir / source.with_suffix(".o")
        objects.append(obj)
        n.build(
            outputs=obj,
            rule="linux_cc",
            inputs=source,
            implicit=[overlay_stamp, prefix_header, semantics_header, platform_semantics_header],
            variables={"cflags": cflags},
        )

    for proj in sln.projects:
        if proj.name not in config["projects"]:
            continue
        options = proj.options
        defines = " ".join(f"-D{d}" for d in options.get("defines") or [])
        includes = " ".join(
            f"-I{_quote(d)}"
            for d in options.get("include_dirs") or []
            if Path(d) != Path("xbox/include")
        )
        game_cflags = " ".join([
            abi,
            " ".join(GAME_FLAGS),
            f"-include {prefix_header}",
            f"-include {semantics_header}",
            defines,
            f"-I{port_include}",
            includes,
            sdk_flags,
        ])
        for obj in proj.objects:
            name = str(obj.file_path).replace(os.sep, "/")
            if obj.status.name == "Missing" or name in excluded:
                continue
            if obj.file_path.suffix.lower() not in (".c",):
                continue
            add_object(obj.file_path, game_cflags)
        # Port-specific units that must see the game exactly as its own
        # sources do (port/linux/game).
        for source in sorted(Path(config["game_sources"]).glob("*.c")):
            add_object(source, game_cflags)

    platform_dir = Path(config["platform_sources"])
    platform_cflags = " ".join([
        abi,
        " ".join(PLATFORM_FLAGS),
        f"-include {prefix_header}",
        f"-include {platform_semantics_header}",
        f"-I{platform_dir}",
        f"-I{port_include}",
        "-Isource -Isource/cseries",
        sdk_flags,
    ])
    posix_cflags = " ".join(POSIX_FLAGS + [f"-I{platform_dir}"])
    for source in sorted(platform_dir.glob("*.c")):
        if source.name.startswith("posix_"):
            add_object(source, posix_cflags)
        else:
            add_object(source, platform_cflags)

    libs = " ".join(f"-l{lib}" for lib in config.get("libraries", []))
    n.build(
        outputs=output,
        rule="linux_link",
        inputs=objects,
        variables={
            "ldflags": "--target=i686-linux-gnu -m32 -no-pie -g",
            "libs": libs,
        },
        implicit=[Path("tools/linux_link_check.py")],
    )
    n.build(outputs="linux", rule="phony", inputs=output)
    n.newline()
