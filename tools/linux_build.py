"""Ninja rules for the native Linux build (``ninja linux``).

This is independent of the byte-matching graph: it compiles the same game
sources with clang for 32-bit x86 Linux, adds the platform layer in
``port/linux/src``, and links an ELF executable at ``build/linux/halo``.
Nothing here changes the MSVC objects, objdiff configuration or progress.
See port/linux/README.md for the design.
"""

import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Sequence, Tuple

from .ninja_syntax import Writer

PORT_DIR = Path("port/linux")
PORT_CONFIG = PORT_DIR / "port.json"
# the Xbox SDK declarations the game and the platform layer use, in place of
# the SDK's headers (port/include/xdk/README.md)
XDK_INCLUDE = Path("port/include/xdk")


def xdk_headers() -> List[Path]:
    return sorted(XDK_INCLUDE.glob("*.h"))


def compile_launcher(sln: Any) -> str:
    """what the native ports' compile commands start with: the
    --compiler-launcher (ccache, say) and a space, or nothing"""
    launcher = getattr(sln, "compiler_launcher", None)
    return f"{launcher} " if launcher else ""

# The optimisation level of every unit, and of link-time optimisation.
OPTIMISATION = "-O2"

# Flags shared by the game and the XDK-facing half of the platform layer.
# They reproduce the MSVC/Xbox ABI the source was written against:
#  - 16-bit wchar_t (UTF-16 strings in tag data and saved games),
#  - MSVC struct layout for 64-bit members (-malign-double),
#  - __declspec, __int64 and calling conventions,
#  - C89 with tentative definitions shared between units (-fcommon),
#  - small structures and unions returned in EAX:EDX, as Win32 does
#    (hs_runtime.c calls union-returning converters through pointers typed
#    as returning long),
#  - no optimisations that assume the absence of MSVC-tolerated UB.
LINUX_ABI_FLAGS = [
    "--target=i686-linux-gnu",
    "-m32",
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
    OPTIMISATION,
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

# the TOML parser the platform layer reads config.toml with (port_config.c)
TOML_DIR = Path("port/third_party/tomlc17")

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
    "-std=gnu11",
    "-D_GNU_SOURCE",
    "-D_FILE_OFFSET_BITS=64",
    "-fno-pic",
    OPTIMISATION,
    "-g",
    "-Wall",
]

# Units compiled with a profile (-fprofile-use) that does not quite match
# them: new or changed functions simply go without.
PROFILE_USE_FLAGS = [
    "-Wno-profile-instr-unprofiled",
    "-Wno-profile-instr-out-of-date",
    "-Wno-profile-instr-missing",
    "-Wno-backend-plugin",
]


def march_flag(sln: Any) -> str:
    """The instruction set of the native x86 builds: this machine's
    (-march=native, the default), or with configure.py --portable the
    baseline every x86-64 processor has (SSE2), for builds that run on other
    computers. The game stays 32-bit code either way."""
    return "-march=x86-64" if getattr(sln, "port_portable", False) else "-march=native"


def lto_mode(sln: Any) -> str:
    """full, thin or off (configure.py --lto)"""
    return getattr(sln, "port_lto", "full")


def lto_flags(sln: Any, cache_dir: Path) -> Tuple[List[str], List[str]]:
    """compiler and linker flags for link-time optimisation with lld"""
    mode = lto_mode(sln)
    if mode == "off":
        return [], []
    flag = "-flto=thin" if mode == "thin" else "-flto=full"
    ldflags = [flag, "-fuse-ld=lld", OPTIMISATION]
    if mode == "thin":
        ldflags.append(f"-Wl,--thinlto-cache-dir={_quote(cache_dir)}")
    return [flag], ldflags


# Profiles recorded by playing the game (tools/pgo_train.py), kept in the
# repository so that every build is optimised with them. They are in the
# format of this LLVM version, which older compilers cannot read.
PGO_DIR = Path("pgo")
LINUX_PROFILE = PGO_DIR / "halo_linux.profdata"
WINDOWS_PROFILE = PGO_DIR / "halo_windows.profdata"
PROFILE_LLVM_MAJOR = 22

_clang_majors: Dict[str, Optional[int]] = {}


def clang_major(cc: str) -> Optional[int]:
    """the major version of the clang named cc, or None if unknown"""
    if cc not in _clang_majors:
        try:
            output = subprocess.run([cc, "--version"], capture_output=True, text=True, check=False).stdout
            match = re.search(r"clang version (\d+)", output)
            _clang_majors[cc] = int(match.group(1)) if match else None
        except OSError:
            _clang_majors[cc] = None
    return _clang_majors[cc]


def pgo_mode(sln: Any) -> str:
    """use, train or off (configure.py --pgo)"""
    return getattr(sln, "port_pgo", "use")


def pgo_profile(sln: Any, own: Optional[Path], others: Sequence[Path], cc: str) -> Optional[Path]:
    """The profile a native build is optimised with: --pgo-profile's; with
    --pgo=train, the build's own profile (trained if it is missing);
    otherwise the first committed profile there is, its own first. None with
    --pgo=off, or when cc is too old to read the profiles."""
    explicit = getattr(sln, "port_pgo_profile", None)
    if explicit:
        return explicit
    mode = pgo_mode(sln)
    if mode == "off":
        return None
    major = clang_major(cc)
    if major is not None and major < PROFILE_LLVM_MAJOR:
        print(f"{cc} is clang {major}; the profiles in {PGO_DIR} need clang {PROFILE_LLVM_MAJOR}: "
              "building without profile-guided optimisation", file=sys.stderr)
        return None
    if mode == "train" and own is not None:
        return own
    for profile in ([own] if own else []) + list(others):
        if profile.is_file():
            return profile
    return None


def profile_use_flags(profile: Any) -> List[str]:
    return [f"-fprofile-use={_quote(profile)}", *PROFILE_USE_FLAGS] if profile else []


def _load_port_config() -> Dict[str, Any]:
    with open(PORT_CONFIG, "r", encoding="utf-8") as f:
        return json.load(f)


def linux_configure_inputs() -> List[Path]:
    """Files whose change must re-run configure.py."""
    if not PORT_CONFIG.is_file():
        return [Path(__file__)]
    return [PORT_CONFIG, Path(__file__), PORT_DIR / "src", PORT_DIR / "game", XDK_INCLUDE]


def _quote(path: Any) -> str:
    text = str(path).replace(os.sep, "/")
    return f'"{text}"' if " " in text else text


def generate_linux_build(n: Writer, sln: Any) -> None:
    if not PORT_CONFIG.is_file():
        # a checkout without the port (or a test fixture): nothing to emit
        return
    config = _load_port_config()
    build_dir: Path = sln.build_dir / "linux"
    obj_dir = build_dir / "obj"
    output = build_dir / "halo"
    cc = sln.linux_cc or "clang"
    prefix_header = PORT_DIR / "include" / "halo_linux_prefix.h"
    semantics_header = build_dir / "halo_msvc_semantics.h"
    platform_semantics_header = build_dir / "platform_msvc_semantics.h"

    n.comment("Native Linux build (ninja linux)")
    n.variable("linux_cc", cc)
    n.rule(
        name="linux_msvc_semantics",
        command="$python tools/linux_msvc_semantics.py --output $out $scan",
        description="LINUX MSVC SEMANTICS $out",
        restat=True,
    )
    game_headers = sorted(
        p for p in Path("source").rglob("*") if p.suffix in (".c", ".h")
    )
    # The game sees its own tags and inline functions and those of the SDK
    # declarations (port/include/xdk); the platform layer only includes the
    # SDK declarations and so only needs their inline functions.
    n.build(
        outputs=semantics_header,
        rule="linux_msvc_semantics",
        implicit=[Path("tools/linux_msvc_semantics.py"), *xdk_headers(), *game_headers],
        variables={
            "scan": f"--all-inlines --tags source --inlines source --inlines {XDK_INCLUDE}"
        },
    )
    n.build(
        outputs=platform_semantics_header,
        rule="linux_msvc_semantics",
        implicit=[Path("tools/linux_msvc_semantics.py"), *xdk_headers()],
        variables={"scan": f"--inlines {XDK_INCLUDE}"},
    )
    n.rule(
        name="linux_cc",
        command=f"{compile_launcher(sln)}$linux_cc -MMD -MF $out.d $cflags -c $in -o $out",
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

    n.rule(
        name="linux_pgo_train",
        command="$python tools/pgo_train.py --binary $binary --work $work --output $out",
        description="LINUX PGO TRAINING: playing levels in the instrumented build",
        pool="console",
    )

    abi = " ".join(LINUX_ABI_FLAGS + [march_flag(sln)] + (["-DHALO_RELEASE"] if getattr(sln, "port_release", False) else []))
    port_include = PORT_DIR / "include"
    sdk_flags = f"-idirafter {XDK_INCLUDE}"
    excluded = set(config.get("exclude_sources", []))
    libs = " ".join(f"-l{lib}" for lib in config.get("libraries", []))

    def emit(obj_dir: Path, output: Path, extra_cflags: List[str], extra_ldflags: List[str],
             implicit_inputs: List[Path]) -> None:
        """the objects and the executable, with the given extra flags"""
        extra = " ".join(extra_cflags)
        # the posix_* units have glibc's 32-bit wchar_t, and LLVM will not
        # optimise them together with code that has a 16-bit one: they stay
        # native objects
        posix_extra = " ".join(flag for flag in extra_cflags if not flag.startswith("-flto"))
        objects: List[Path] = []

        def add_object(source: Path, cflags: str, posix: bool = False) -> None:
            obj = obj_dir / source.with_suffix(".o")
            objects.append(obj)
            n.build(
                outputs=obj,
                rule="linux_cc",
                inputs=source,
                # (the SDK declarations are system headers, which the depfile
                # leaves out)
                implicit=[*xdk_headers(), prefix_header, semantics_header, platform_semantics_header,
                          *implicit_inputs],
                variables={"cflags": f"{cflags} {posix_extra if posix else extra}"},
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
            f"-I{TOML_DIR}",
            "-Isource -Isource/cseries",
            sdk_flags,
        ])
        posix_cflags = " ".join(POSIX_FLAGS + [march_flag(sln), f"-I{platform_dir}"])
        for source in sorted(platform_dir.glob("*.c")):
            if source.name.startswith("posix_"):
                add_object(source, posix_cflags, posix=True)
            else:
                add_object(source, platform_cflags)
        # the settings file's parser (port/third_party/tomlc17), with the
        # platform layer's ABI (its structs hold doubles) and nothing else
        add_object(TOML_DIR / "tomlc17.c", " ".join([abi, "-std=gnu11", "-w"]))

        n.build(
            outputs=output,
            rule="linux_link",
            inputs=objects,
            variables={
                "ldflags": " ".join(["--target=i686-linux-gnu", "-m32", "-no-pie", "-g", *extra_ldflags]),
                "libs": libs,
            },
            implicit=[Path("tools/linux_link_check.py")],
        )

    # Profile-guided optimisation: with the committed profile, or with
    # --pgo=train one that an instrumented build records while playing
    # (tools/pgo_train.py). A profile is trained once: code changed since
    # simply goes without, and deleting it trains a new one.
    profile = pgo_profile(sln, LINUX_PROFILE, [], cc)
    if pgo_mode(sln) == "train" and profile == LINUX_PROFILE:
        instrumented = build_dir / "pgo-generate" / "halo"
        emit(build_dir / "pgo-generate" / "obj", instrumented, ["-fprofile-generate"], ["-fprofile-generate"], [])
        n.build(
            outputs=profile,
            rule="linux_pgo_train",
            order_only=[instrumented],
            variables={"binary": str(instrumented), "work": str(sln.build_dir / "pgo" / "linux")},
        )

    # Link-time optimisation: the objects are LLVM bitcode, optimised and
    # compiled together when lld links them.
    cflags, ldflags = lto_flags(sln, build_dir / "thinlto-cache")
    cflags += profile_use_flags(profile)
    emit(obj_dir, output, cflags, ldflags, [profile] if profile else [])
    n.build(outputs="linux", rule="phony", inputs=output)
    n.newline()
