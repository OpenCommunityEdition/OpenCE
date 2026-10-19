"""Cross-compiling the Windows build from Linux (configure.py --windows-cross).

tools/windows_build.py generates the same build on Windows and on Linux;
what a cross build needs in addition lives here:

- finding the MSVC toolchain's directories (configure.py's
  --windows-vctools-dir and --windows-sdk-dir, or VCToolsInstallDir and
  WindowsSdkDir), whatever their case on disk is
- bridging the file names' cases: neither clang nor lld-link looks names up
  case-insensitively as Windows does, so on Linux's case-sensitive file
  systems `<windows.h>` does not find `Windows.h`, `-lws2_32` does not find
  `WS2_32.Lib`, and the SDK's own headers name one another in a third case
  (`<DriverSpecs.h>` for driverspecs.h). case_bridge() links the names in
  build/windows/case_bridge. Importantly, the toolchain itself is not touched, so a
  mounted Windows install serves as well as one downloaded with something like msvc-wine
- Clang receives the directories in INCLUDE and LIB, just like what vcvars*.bat
  sets on VS and clang's MSVC toolchain expects (unfortunately /winsdkdir and
  /vctoolsdir are clang-cl only).

See port/windows/README.md.
"""

import os
import re
import shutil
import sys
from pathlib import Path
from typing import Any, Dict, List, Tuple

LINUX_DIR = Path("port/linux")
BUILD = Path("build/windows")

# Very crude regex to match includes
INCLUDE_SCAN = re.compile(r"^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]", re.M)


def _ci_child(path: Path, name: str) -> Path:
    """Finds the child element named `name` under path, found with whatever its case on disk is.
    This is useful because a mounted Windows install has `Include` and `Lib`, where one fixed by msvc-wine
    has `include` and `lib`."""
    exact = path / name
    if not exact.is_dir() and path.is_dir():
        lowered = name.lower()
        for child in path.iterdir():
            if child.is_dir() and child.name.lower() == lowered:
                return child
    return exact


def case_bridge(sources: List[Path], kind: str) -> List[Path]:
    """Lowercase views of the toolchain's directories, for the case-sensitive
    file system a *NIX cross compiler runs on. This function mirrors headers under
    build/windows/case_bridge in which every file is a link of its lowercased
    name, so that the compiler and lld find the names the sources and the
    default-library directives use (<windows.h> for Windows.h, -lws2_32 and
    uuid.lib for WS2_32.Lib and Uuid.Lib). This is because neither clang nor lld-link looks
    names up case-insensitively; the crafted directory takes priority, delegating to the real directories
    for names spelled as on disk. Made once and remade only when the toolchain's directories change.
    This function uses a few heuristics that overall work fine on the current SDK (10.0.26100.0) and
    MSVC (14.51.36231)"""
    root = BUILD / "case_bridge" / kind
    stamp = BUILD / "case_bridge" / f"{kind}.stamp"
    resolved = [source.resolve() for source in sources]
    text = "\n".join(str(p) for p in resolved)
    if stamp.is_file() and stamp.read_text(encoding="utf-8") == text:
        return sorted(p for p in root.iterdir() if p.is_dir())
    print(f"Cross toolchain: linking the {kind} directories' names in lower case, in {root}")
    shutil.rmtree(root, ignore_errors=True)
    mirrors: List[Path] = []
    by_lowercase: Dict[str, Tuple[int, str]] = {}
    for index, source in enumerate(resolved):
        mirror = root / str(index)
        mirrors.append(mirror)
        for dirpath, _dirnames, filenames in os.walk(source):
            rel = Path(dirpath).relative_to(source)
            target_dir = mirror.joinpath(*(part.lower() for part in rel.parts)) if rel.parts else mirror
            target_dir.mkdir(parents=True, exist_ok=True)
            for name in filenames:
                link = target_dir / name.lower()
                # of two names that differ only in case, either is the file
                # a case-insensitive system would have found
                if not os.path.lexists(link):
                    os.symlink(Path(dirpath) / name, link)
                rel_posix = (rel / name).as_posix() if rel.parts else name
                by_lowercase.setdefault(rel_posix.lower(), (index, rel_posix))
    if kind == "include":
        # The SDK's own headers require some extra shenanigans due to stuff like "driverspecs.h" being
        # referred to as <DriverSpecs.h> by <Windows.h>. This loop makes a link of each such spelling,
        # found by reading every header's includes, beside the lowercased ones.
        for source in resolved:
            # Walk over all headers and find include preprocessor statements, creating symlinks if they
            # don't respect the actual file case
            for dirpath, _dirnames, filenames in os.walk(source):
                for name in filenames:
                    try:
                        text_of = (Path(dirpath) / name).read_text(encoding="utf-8", errors="replace")
                    except OSError:
                        continue
                    for spelled in INCLUDE_SCAN.findall(text_of):
                        spelled = spelled.replace("\\", "/")
                        entry = by_lowercase.get(spelled.lower())
                        if entry is None or spelled == spelled.lower():
                            continue
                        dir_index, actual = entry
                        if spelled == actual:
                            continue
                        link = mirrors[dir_index] / spelled
                        if not os.path.lexists(link):
                            link.parent.mkdir(parents=True, exist_ok=True)
                            os.symlink(resolved[dir_index] / actual, link)
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.write_text(text, encoding="utf-8")
    return mirrors


def cross_toolchain_prefix(sln: Any) -> str:
    """Sets up a cross build targeting Windows from a *NIX system (see configure.py
    --windows-cross). This injects all MSVC toolchain's directories in INCLUDE
    and LIB, i.e. the same variables vcvars*.bat sets and clang reads (no -winsdkdir and
    -vctoolsdir unfortunately, they're clang-cl only). The toolchain's directories are set via
    configure.py's --windows-vctools-dir (path to the versioned folder under VC/Tools/MSVC, with `include` and `lib`)
    and --windows-sdk-dir (path to the Windows SDK folder with `Include` and `Lib`), or through the environment via
    VCToolsInstallDir and WindowsSdkDir. This function can either use a Windows install mounted (read-only is fine),
    or a copy of MSVC downloaded with something like msvc-wine. This file bridges the casing using some magic (see
    case_bridge). Returns just an empty string on Windows, where no discovery is necessary thanks to clang doing that
    for us"""
    if sys.platform == "win32":
        return ""
    vctools = getattr(sln, "windows_vctools_dir", None) or os.environ.get("VCToolsInstallDir")
    sdk = getattr(sln, "windows_sdk_dir", None) or os.environ.get("WindowsSdkDir")
    if vctools and sdk:
        vctools_path, sdk_path = Path(vctools), Path(sdk)
        sdk_include = _ci_child(sdk_path, "include")
        # the newest of the SDK's versions, as clang picks on Windows
        sdk_versions = sorted(
            (p.name for p in sdk_include.glob("*") if p.is_dir()),
            key=lambda name: [int(part) if part.isdigit() else 0 for part in name.split(".")],
        )
        sdk_version = sdk_versions[-1] if sdk_versions else ""
    if not vctools or not sdk or not _ci_child(vctools_path, "include").is_dir() or not sdk_version:
        sys.exit("--windows-cross needs both the MSVC and the Windows SDK directories to be set, either via command" \
                 " line flags (--windows-vctools-dir and --windows-sdk-dir) or environment variables (VCToolsInstallDir and WindowsSdkDir)."
                 " The correct MSVC dir is the versioned one with `include` and `lib` (no matter the casing). For Windows SDK, set a path" \
                 " containing both Include and Lib (usually under 'Windows Kits/10')")
    vctools, sdk = vctools_path, sdk_path
    includes = [
        _ci_child(vctools, "include"),
        _ci_child(_ci_child(sdk, "include") / sdk_version, "ucrt"),
        _ci_child(_ci_child(sdk, "include") / sdk_version, "um"),
        _ci_child(_ci_child(sdk, "include") / sdk_version, "shared"),
        _ci_child(_ci_child(sdk, "include") / sdk_version, "winrt"),
    ]
    # link to 32-bit (x86) libraries
    libraries = [
        _ci_child(_ci_child(vctools, "lib"), "x86"),
        _ci_child(_ci_child(_ci_child(sdk, "lib") / sdk_version, "ucrt"), "x86"),
        _ci_child(_ci_child(_ci_child(sdk, "lib") / sdk_version, "um"), "x86"),
    ]
    # The game includes <StdDef.h>, which has the wrong casing on a case-sensitive file system.
    # The Linux build already has a shim (port/linux/include/StdDef.h), so we can use it for this too
    overlay = BUILD / "include_case"
    overlay.mkdir(parents=True, exist_ok=True)
    shim = overlay / "StdDef.h"
    if os.path.lexists(shim):
        shim.unlink()
    os.symlink((LINUX_DIR / "include" / "StdDef.h").resolve(), shim)
    # Very important, make fixed case-sensitive symlinks for all includes and libs, which often are referred to with
    # the wrong case (something NTFS hides on Windows)
    includes = [overlay, *case_bridge(includes, "include"), *includes]
    libraries = [*case_bridge(libraries, "lib"), *libraries]
    def shq(text: str) -> str:
        return "'" + text.replace("'", "'\\''") + "'"

    # Craft the same env vars vcvars*.bat defines on Windows and that Clang in MSVC mode expects to find
    return "env " + " ".join([
        f"INCLUDE={shq(';'.join(str(p) for p in includes))}",
        f"LIB={shq(';'.join(str(p) for p in libraries))}",
    ]) + " "
