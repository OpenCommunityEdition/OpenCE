"""Ninja rules for the native Windows build (``ninja windows``).

Like the Linux build (tools/linux_build.py), this is independent of the
byte-matching graph: it compiles the same game sources with clang for 32-bit
x86 Windows (i686-pc-windows-msvc), adds the platform layer shared with
Linux (``port/linux/src``) and the Windows parts in ``port/windows``, and
links ``build/windows/halo.exe`` with lld. It is generated only when
configure.py runs on Windows. See port/windows/README.md for the design.
"""

import json
import os
import re
import shutil
import sys
import urllib.request
import zipfile
from pathlib import Path
from typing import Any, Dict, List, Optional

from .ninja_syntax import Writer

LINUX_DIR = Path("port/linux")
PORT_DIR = Path("port/windows")
PORT_CONFIG = PORT_DIR / "port.json"
BUILD = Path("build/windows")

SDL_VERSION = "3.4.16"
SDL_URL = (
    f"https://github.com/libsdl-org/SDL/releases/download/release-{SDL_VERSION}/"
    f"SDL3-devel-{SDL_VERSION}-VC.zip"
)
THIRD_PARTY = BUILD / "third_party"
SDL_DIR = THIRD_PARTY / f"SDL3-{SDL_VERSION}"

# Flags shared by every unit. The Microsoft target gives the game the ABI it
# was written against natively: 16-bit wchar_t, MSVC structure layout,
# __asm blocks, __declspec, calling conventions and COMDAT inline functions.
#  - 32-bit time_t, as in the Xbox's C runtime (the same on both sides of the
#    platform layer, which share struct timespec),
#  - tentative definitions shared between units (-fcommon),
#  - no optimisations that assume the absence of MSVC-tolerated UB,
#  - EBP frames (MSVC /Oy-): get_return_eip and the stack walker follow the
#    frame chain.
WINDOWS_ABI_FLAGS = [
    "--target=i686-pc-windows-msvc",
    "-march=pentium3",
    "-fms-extensions",
    "-fcommon",
    "-fno-strict-aliasing",
    "-fwrapv",
    "-fno-delete-null-pointer-checks",
    "-fno-omit-frame-pointer",
    "-O2",
    "-g",
    "-gcodeview",
    "-D_USE_32BIT_TIME_T",
    "-D_CRT_SECURE_NO_WARNINGS",
    "-D_CRT_NONSTDC_NO_WARNINGS",
    "-D_WINSOCK_DEPRECATED_NO_WARNINGS",
    # the MSVC 7 signatures the game uses: swprintf without a buffer size,
    # two-argument wcstok
    "-D_CRT_NON_CONFORMING_SWPRINTFS",
    "-D_CRT_NON_CONFORMING_WCSTOK",
]

GAME_FLAGS = [
    "-std=gnu89",
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
    "-Wno-microsoft-enum-forward-reference",
    "-Wno-language-extension-token",
]

# port/windows/src/win32_*.c talk to Windows only: they see the Windows SDK
# instead of the Xbox SDK (the two define the same names differently).
WIN32_FLAGS = [
    "-std=gnu11",
    "-DWIN32_LEAN_AND_MEAN",
    "-DNOMINMAX",
    "-D_WIN32_WINNT=0x0A00",
    "-Wall",
]


def _load_config() -> Dict[str, Any]:
    with open(PORT_CONFIG, "r", encoding="utf-8") as f:
        return json.load(f)


def windows_configure_inputs() -> List[Path]:
    """Files whose change must re-run configure.py."""
    return [Path(__file__), PORT_CONFIG, PORT_DIR / "src", LINUX_DIR / "src", LINUX_DIR / "game"]


def _quote(path: Any) -> str:
    text = str(path).replace(os.sep, "/")
    return f'"{text}"' if " " in text else text


def fetch_sdl() -> None:
    """Downloads SDL3's Visual C++ development package (headers, import
    library and DLL) once."""
    if (SDL_DIR / "lib" / "x86" / "SDL3.lib").is_file():
        return
    THIRD_PARTY.mkdir(parents=True, exist_ok=True)
    archive = THIRD_PARTY / f"SDL3-devel-{SDL_VERSION}-VC.zip"
    print(f"Downloading {SDL_URL}")
    with urllib.request.urlopen(SDL_URL) as response, open(archive, "wb") as f:
        shutil.copyfileobj(response, f)
    with zipfile.ZipFile(archive) as z:
        z.extractall(THIRD_PARTY)
    archive.unlink()


# a C __inline function with external linkage, defined in a .c file
EXPORTED_INLINE = re.compile(
    r"^(?!\s*static\b)[^;{}()\n]*\b(?:__inline|_inline|__forceinline)\b[^;{}()]*?"
    r"\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{",
    re.M,
)
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def inline_export_wrapper(source: Path) -> Path:
    """MSVC emits a C __inline function with external linkage wherever a
    call to it is not inlined, and other units may call it through a
    prototype; clang can inline every call in the defining unit, leaving no
    copy. A unit that defines such a function is compiled through a
    generated wrapper that includes it and takes the functions' addresses,
    which makes clang emit them (as COMDATs). Returns the file to compile."""
    if source.suffix != ".c" or not str(source).replace(os.sep, "/").startswith("source/"):
        return source
    names = EXPORTED_INLINE.findall(COMMENT.sub("", source.read_text(encoding="utf-8", errors="replace")))
    if not names:
        return source
    wrapper = BUILD / "inline_exports" / source
    text = (
        "/* generated by tools/windows_build.py: see inline_export_wrapper */\n"
        f'#include "{source.resolve().as_posix()}"\n'
        "static void *const halo_windows_inline_exports[] __attribute__((used)) = {\n"
        + "".join(f"\t(void *){name},\n" for name in names)
        + "};\n"
    )
    wrapper.parent.mkdir(parents=True, exist_ok=True)
    if not wrapper.is_file() or wrapper.read_text(encoding="utf-8") != text:
        wrapper.write_text(text, encoding="utf-8")
    return wrapper


def generate_windows_build(n: Writer, sln: Any) -> None:
    if sys.platform != "win32" or not PORT_CONFIG.is_file():
        return
    try:
        fetch_sdl()
    except OSError as error:
        print(f"Windows build disabled: cannot fetch SDL3 ({error})", file=sys.stderr)
        return
    linux_config: Dict[str, Any] = json.loads((LINUX_DIR / "port.json").read_text(encoding="utf-8"))
    config = _load_config()

    overlay_dir = BUILD / "sdk_include"
    overlay_stamp = BUILD / "sdk_include.stamp"
    tags_header = BUILD / "halo_msvc_tags.h"
    obj_dir = BUILD / "obj"
    output = BUILD / "halo.exe"
    sdl_dll = BUILD / "SDL3.dll"
    cc = getattr(sln, "windows_cc", None) or "clang"
    prefix_header = PORT_DIR / "include" / "halo_windows_prefix.h"
    crt_include = PORT_DIR / "include" / "crt"
    posix_include = PORT_DIR / "include" / "posix"

    n.comment("Native Windows build (ninja windows)")
    n.variable("windows_cc", cc)
    n.rule(
        name="windows_sdk_overlay",
        command=f"$python tools/windows_sdk_overlay.py --output {overlay_dir} --stamp $out",
        description="WINDOWS SDK HEADERS",
    )
    n.build(outputs=overlay_stamp, rule="windows_sdk_overlay",
            implicit=[Path("tools/windows_sdk_overlay.py"), Path("tools/linux_sdk_overlay.py")])
    # MSVC gives struct tags first named in a prototype file scope; clang
    # does not (the Linux build's generator also writes these declarations)
    n.rule(
        name="windows_msvc_tags",
        command="$python tools/linux_msvc_semantics.py --output $out --tags source",
        description="WINDOWS MSVC TAGS $out",
        restat=True,
    )
    game_headers = sorted(p for p in Path("source").rglob("*") if p.suffix in (".c", ".h"))
    n.build(outputs=tags_header, rule="windows_msvc_tags",
            implicit=[Path("tools/linux_msvc_semantics.py"), *game_headers])
    n.rule(
        name="windows_cc",
        command="$windows_cc -MMD -MF $out.d $cflags -c $in -o $out",
        description="WINDOWS CC $out",
        depfile="$out.d",
        deps="gcc",
    )
    n.rule(
        name="windows_link",
        command="$windows_cc $ldflags -o $out --rsp-quoting=windows @$out.rsp $libs",
        description="WINDOWS LINK $out",
        rspfile="$out.rsp",
        rspfile_content="$in_newline",
    )
    n.rule(
        name="windows_copy",
        command="$python -c \"import shutil,sys; shutil.copyfile(sys.argv[1], sys.argv[2])\" $in $out",
        description="WINDOWS COPY $out",
    )

    abi = " ".join(WINDOWS_ABI_FLAGS + (["-DHALO_RELEASE"] if getattr(sln, "port_release", False) else []))
    sdl_include = SDL_DIR / "include"
    excluded = set(linux_config.get("exclude_sources", []))
    objects: List[Path] = []

    def add_object(source: Path, cflags: str) -> None:
        obj = obj_dir / source.with_suffix(".o")
        objects.append(obj)
        compiled = inline_export_wrapper(source)
        n.build(
            outputs=obj,
            rule="windows_cc",
            inputs=compiled,
            implicit=[overlay_stamp, prefix_header, tags_header, source],
            variables={"cflags": cflags},
        )

    for proj in sln.projects:
        if proj.name not in linux_config["projects"]:
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
            f"-include {tags_header}",
            defines,
            f"-I{crt_include}",
            f"-I{PORT_DIR / 'include'}",
            includes,
            # the Xbox SDK comes before the Windows SDK, which has headers of
            # the same names; the overlay leaves out the SDK's C runtime
            f"-I{overlay_dir}",
        ])
        for obj in proj.objects:
            name = str(obj.file_path).replace(os.sep, "/")
            if obj.status.name == "Missing" or name in excluded:
                continue
            if obj.file_path.suffix.lower() != ".c":
                continue
            add_object(obj.file_path, game_cflags)
        for source in sorted(Path(linux_config["game_sources"]).glob("*.c")):
            add_object(source, game_cflags)

    linux_platform = Path(linux_config["platform_sources"])
    platform_cflags = " ".join([
        abi,
        " ".join(PLATFORM_FLAGS),
        f"-include {prefix_header}",
        f"-I{posix_include}",
        f"-I{crt_include}",
        f"-I{linux_platform}",
        f"-I{PORT_DIR / 'include'}",
        # halo_linux_winsock_names.h, but not the Linux build's C runtime
        # wrappers next to it
        f"-iquote {LINUX_DIR / 'include'}",
        "-Isource -Isource/cseries",
        f"-I{_quote(sdl_include)}",
        f"-I{overlay_dir}",
    ])
    win32_cflags = " ".join([
        abi,
        " ".join(WIN32_FLAGS),
        f"-I{posix_include}",
        f"-I{linux_platform}",
        f"-I{_quote(sdl_include)}",
    ])
    replaced = set(config.get("replaced_platform_sources", []))
    for source in sorted(linux_platform.glob("*.c")):
        if source.name in replaced:
            continue
        add_object(source, platform_cflags)
    for source in sorted((PORT_DIR / "src").glob("*.c")):
        add_object(source, win32_cflags if source.name.startswith("win32_") else platform_cflags)

    libs = " ".join(
        [_quote(SDL_DIR / "lib" / "x86" / "SDL3.lib")]
        + [f"-l{lib}" for lib in config.get("libraries", [])]
    )
    n.build(
        outputs=output,
        rule="windows_link",
        inputs=objects,
        variables={
            "ldflags": " ".join([
                "--target=i686-pc-windows-msvc",
                "-fuse-ld=lld",
                "-g",
                # the Xbox memory window is at 0x80000000, in the upper half
                # of the 32-bit address space
                "-Wl,/LARGEADDRESSAWARE",
                "-Wl,/STACK:0x800000",
                "-Wl,/SUBSYSTEM:CONSOLE",
            ]),
            "libs": libs,
        },
    )
    n.build(outputs=sdl_dll, rule="windows_copy", inputs=SDL_DIR / "lib" / "x86" / "SDL3.dll")
    n.build(outputs="windows", rule="phony", inputs=[output, sdl_dll])
    n.newline()
