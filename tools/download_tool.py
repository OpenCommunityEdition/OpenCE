#!/usr/bin/env python3

###
# Downloads various tools from GitHub releases.
#
# Usage:
#   python3 tools/download_tool.py wibo build/tools/wibo --tag 1.0.0
#
# If changes are made, please submit a PR to
# https://github.com/encounter/dtk-template
###

import argparse
import io
import os
import platform
import shutil
import stat
import urllib.request
import zipfile
from typing import Callable, Dict
from pathlib import Path


def binutils_url(tag):
    uname = platform.uname()
    system = uname.system.lower()
    arch = uname.machine.lower()
    if system == "darwin":
        system = "macos"
        arch = "universal"
    elif arch == "amd64":
        arch = "x86_64"

    repo = "https://github.com/encounter/gc-wii-binutils"
    return f"{repo}/releases/download/{tag}/{system}-{arch}.zip"


def compilers_url(tag: str) -> str:
    return f"https://files.decomp.dev/compilers_{tag}.zip"


def dtk_url(tag: str) -> str:
    uname = platform.uname()
    suffix = ""
    system = uname.system.lower()
    if system == "darwin":
        system = "macos"
    elif system == "windows":
        suffix = ".exe"
    arch = uname.machine.lower()
    if arch == "amd64":
        arch = "x86_64"

    repo = "https://github.com/encounter/decomp-toolkit"
    return f"{repo}/releases/download/{tag}/dtk-{system}-{arch}{suffix}"


def objdiff_cli_url(tag: str) -> str:
    uname = platform.uname()
    suffix = ""
    system = uname.system.lower()
    if system == "darwin":
        system = "macos"
    elif system == "windows":
        suffix = ".exe"
    arch = uname.machine.lower()
    if arch == "amd64":
        arch = "x86_64"

    repo = "https://github.com/encounter/objdiff"
    return f"{repo}/releases/download/{tag}/objdiff-cli-{system}-{arch}{suffix}"


def sjiswrap_url(tag: str) -> str:
    repo = "https://github.com/encounter/sjiswrap"
    return f"{repo}/releases/download/{tag}/sjiswrap-windows-x86.exe"


def wibo_url(tag: str) -> str:
    uname = platform.uname()
    arch = uname.machine.lower()
    system = uname.system.lower()
    if system == "darwin":
        arch = "macos"

    repo = "https://github.com/decompals/wibo"
    return f"{repo}/releases/download/{tag}/wibo-{arch}"


def csplit_url(tag: str) -> str:
    uname = platform.uname()
    suffix = ""
    system = uname.system.lower()
    if system == "darwin":
        system = "macos"
    elif system == "windows":
        suffix = ".exe"
    arch = uname.machine.lower()
    if arch == "amd64":
        arch = "x86_64"

    repo = "https://github.com/punpckhdq/csplit"
    return f"{repo}/releases/download/{tag}/csplit-{system}-{arch}{suffix}"


def uasm_url(tag: str) -> str:
    # UASM, an open-source MASM-compatible assembler, publishes a Linux
    # x86-64 build only. Release "v2.57r" ships "uasm257_linux64.zip".
    version = tag.lstrip("v").rstrip("r").replace(".", "")
    repo = "https://github.com/Terraspace/UASM"
    return f"{repo}/releases/download/{tag}/uasm{version}_linux64.zip"


TOOLS: Dict[str, Callable[[str], str]] = {
    "binutils": binutils_url,
    "compilers": compilers_url,
    "dtk": dtk_url,
    "objdiff-cli": objdiff_cli_url,
    "sjiswrap": sjiswrap_url,
    "wibo": wibo_url,
    "csplit": csplit_url,
    "uasm": uasm_url,
}


def download(url, response, output) -> None:
    if url.endswith(".zip"):
        data = io.BytesIO(response.read())
        with zipfile.ZipFile(data) as f:
            f.extractall(output)
        # Make all files executable
        for root, _, files in os.walk(output):
            for name in files:
                os.chmod(os.path.join(root, name), 0o755)
        output.touch(mode=0o755)  # Update dir modtime
    else:
        with open(output, "wb") as f:
            shutil.copyfileobj(response, f)
        st = os.stat(output)
        os.chmod(output, st.st_mode | stat.S_IEXEC)


CSPLIT_REPOSITORY = "https://github.com/punpckhdq/csplit"


def build_csplit_from_source(commit: str, output: Path) -> None:
    """Build csplit for the host from a pinned source commit.

    The v0.0.2 Linux release writes placeholder ("PHONY") objects with a
    stray, uninitialised relocation block ahead of the symbol table, which
    objdiff rejects. Upstream fixed it after the release ("Fix uninit phony
    section"); until a release carries the fix, build that commit.
    """
    import subprocess
    import tarfile
    import tempfile

    url = f"{CSPLIT_REPOSITORY}/archive/{commit}.tar.gz"
    print(f"Building csplit {commit[:12]} from {url} into {output}")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with tempfile.TemporaryDirectory() as temporary:
        with urllib.request.urlopen(req) as response:
            with tarfile.open(fileobj=io.BytesIO(response.read()), mode="r:gz") as archive:
                archive.extractall(temporary, filter="data")
        source = next(Path(temporary).glob("*/csplit"))
        compiler = os.environ.get("CC", "cc")
        subprocess.run(
            [compiler, "-O2", "-DCJSON_HIDE_SYMBOLS", "-o", str(output),
             str(source / "main.c"), str(source / "cJSON.c"), "-lm"],
            check=True,
        )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("tool", help="Tool name")
    parser.add_argument("output", type=Path, help="output file path")
    parser.add_argument("--tag", help="GitHub tag", required=True)
    args = parser.parse_args()

    output = Path(args.output)
    if args.tool == "csplit-source":
        build_csplit_from_source(args.tag, output)
        return

    url = TOOLS[args.tool](args.tag)

    print(f"Downloading {url} to {output}")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    try:
        with urllib.request.urlopen(req) as response:
            download(url, response, output)
    except urllib.error.URLError as e:
        if str(e).find("CERTIFICATE_VERIFY_FAILED") == -1:
            raise e
        try:
            import certifi
            import ssl
        except ImportError:
            print(
                '"certifi" module not found. Please install it using "python -m pip install certifi".'
            )
            return

        with urllib.request.urlopen(
            req, context=ssl.create_default_context(cafile=certifi.where())
        ) as response:
            download(url, response, output)


if __name__ == "__main__":
    main()
