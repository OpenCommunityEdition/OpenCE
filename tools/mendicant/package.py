#!/usr/bin/env python3
"""Packages a MENDICANT AI test build: a built game folder made into one zip to hand a playtester.
Fork-only (not part of the upstream pull request).

    python3 tools/mendicant/package.py --game build/linux \\
        --models build/release/mods/mendicant_ai/tags/mendicant/models.aibw --out build/release
    python3 tools/mendicant/package.py --game build/linux_x86_64 --models ... --out build/release
    python3 tools/mendicant/package.py --game halo-windows-release.zip --models ... --out build/release

--game is one of:
  - a build folder (build/linux, build/linux_x86_64, build/windows): the files tools/ci_build.py puts in dist/ are taken
    from it and from the repository, as that script takes them (keep the two lists alike);
  - a release folder or zip (the workflow's halo-<platform>-release artifact, dist/halo-...): its
    files as they are, less anything a player's run leaves (settings, maps, saves, logs).

The platform is the executable's: halo.exe a 32-bit program is the Windows build ("windows"), a
64-bit one the Windows x64 build ("windows-x64": the 64-bit Linux build's guest image inside a
64-bit Windows host, tools/windows_x64_build.py); halo a 32-bit ELF file is the x86 Linux build
("linux"), a 64-bit one the 64-bit x86 Linux build ("linux-x86_64": the game as an x32 guest
inside the 64-bit halo, tools/linux_guest_build.py, whose guest image, halo_guest.elf, is built
into the executable and so not shipped beside it).

The zip holds, at its top (unzipped into a new folder, as the platform's release zip is):
  the game's files; mods/mendicant_ai/ (mod.toml and tags/mendicant/models.aibw, where the game
  looks up ai.mendicant.weights, "mendicant/models.aibw"); README_MENDICANT.txt; LICENSE.txt
  (the fork's LICENSE, the AGPL 3.0) and SOURCE.txt (where the fork's source is, AGPL section 13).

No game assets, player data or personal paths go in: maps, custom_maps, config.toml, saves,
profiles, logs and disc images are left out, debug information is stripped from ELF files (the
full copies are kept beside the zip, <zip stem>-symbols/, for symbolizing a crash), and a path of
this machine's (the repository's folder, the home folder) left in a binary's strings is overwritten
with one of the same length; the zip is refused if a text file still names one.
"""

import argparse
import io
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import time
import zipfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parent.parent.parent
MOD_FOLDER = "mods/mendicant_ai"
MOD_NAME = "MENDICANT AI"
MOD_VERSION = "0.1.0-test"
MOD_DESCRIPTION = ("The campaign's Marines and Covenant fight as units led by an Elite or a sergeant "
                   "(bounding, flanking, suppressing, falling back), with trained models choosing the "
                   "drills and each soldier's way of fighting. Campaign and co-op host only.")
UPSTREAM = "https://github.com/OpenCommunityEdition/OpenCE"

# tools/ci_build.py's OUTPUTS and the files its main() adds to dist/ (source, name in the release)
BUILD_OUTPUTS = {
    "linux": ["halo", "libSDL3.so.0", "SDL3-LICENSE.txt"],
    "linux-x86_64": ["halo", "libSDL3.so.0", "SDL3-LICENSE.txt"],
    "windows": ["halo.exe", "SDL3.dll"],
    "windows-x64": ["halo.exe", "SDL3.dll"],
}
REPOSITORY_FILES = [
    ("LICENSE", "LICENSE.txt"),
    ("port/third_party/mbedtls/LICENSE", "mbedtls-LICENSE.txt"),
    ("port/third_party/miniupnpc/LICENSE", "miniupnpc-LICENSE.txt"),
    ("port/assets/fonts/Overpass-OFL.txt", "Overpass-OFL.txt"),
    ("port/third_party/expat/COPYING", "expat-COPYING.txt"),
    ("port/third_party/opus/COPYING", "opus-COPYING.txt"),
    ("port/third_party/jsmn/LICENSE", "jsmn-LICENSE.txt"),
    ("port/assets/icons/lucide/LICENSE", "lucide-LICENSE.txt"),
    ("port/assets/network/brokers.txt", "brokers.txt"),
    ("port/assets/network/masters.txt", "masters.txt"),
    ("deploy/init.txt.example", "init.txt.example"),
]
REPOSITORY_FOLDERS = [("deploy/gametypes", "gametypes")]
LINUX_ONLY_FILES = [("deploy/halo-server", "halo-server")]
LINUX_ONLY_FOLDERS = [("deploy", "deploy")]
# (tools/ci_build.py gives the dedicated server's examples to the x86 Linux and the 32-bit Windows
# builds only)
SERVER_EXAMPLES = {"init.txt.example", "gametypes"}

# what a player's run leaves in the game's folder, never packaged from a release folder
LEFT_OUT = re.compile(r"(?i)^(maps|custom_maps|mods|saves|profiles|crashes|update\.partial|logs)(/|$)|"
                      r"(^|/)(config\.toml|debug\.txt|halo\.log|bans\.txt|cheaters\.txt|gamestate\.txt|"
                      r"init\.txt|load_order\.toml|downloaded_maps\.txt|SHA256SUMS(\.sig)?)$|"
                      r"\.(iso|xiso|map|pdb|log|dmp)$|(^|/)__pycache__/|\.zip$")
EXECUTABLE_NAMES = {"halo", "halo-server"}


def readme(platform):
    windows = platform.startswith("windows")
    executable = "halo.exe" if windows else "./halo"
    build = {"windows": "Windows", "windows-x64": "64-bit Windows", "linux": "32-bit x86 Linux",
             "linux-x86_64": "64-bit x86 Linux"}[platform]
    requirements = {
        "windows": "",
        "windows-x64": ("  This build is a 64-bit program (the game runs inside it as a 32-bit guest, the same one\n"
                        "  as the linux-x86_64 package's). It is new: if it does not start, use the windows\n"
                        "  package instead, and send halo.log.\n"),
        "linux": ("  This build needs the distribution's 32-bit OpenGL libraries; a computer without them can\n"
                  "  use the linux-x86_64 package instead.\n"),
        "linux-x86_64": ("  This build is a 64-bit program (the game runs inside it as a 32-bit guest): it needs\n"
                         "  no 32-bit libraries, only the 64-bit OpenGL libraries of GLVND (libOpenGL.so.0,\n"
                         "  libEGL.so.1), which Mesa and NVIDIA's driver use.\n"),
    }[platform]
    log = ("halo.log, beside halo.exe" if windows
           else "the terminal's output (run ./halo from a terminal: ./halo 2>&1 | tee halo.log)")
    return f"""MENDICANT AI {MOD_VERSION} (test build, {build})
==================================

What it is
  A test build of OpenCE (Halo: Combat Evolved on PC) with MENDICANT AI: in the campaign the
  Marines and the Covenant fight as units, an Elite or a sergeant leading each through drills
  (bounding, flanking, suppressing, falling back), and trained models choose the drills and each
  soldier's way of fighting. It is on by default in this build.

Install
  1. Unzip everything into a new, empty folder (not over another OpenCE or Halo install).
  2. Start {executable}.
  3. On the first start the game asks for your own Halo: Combat Evolved Xbox disc image
     (.iso or .xiso); choose it and the game extracts its maps beside the executable. A disc
     image placed next to the executable is used without asking. This build includes no game
     data.
  This build does not update itself and sends no crash reports anywhere.
{requirements}
Campaign AI settings
  Settings > Player Profile > Video Setup > CAMPAIGN AI (also from a campaign's pause menu,
  SETTINGS). MENDICANT AI: ON/OFF turns it on or off (OFF is the stock AI); the other rows set
  the trained models, individuals, drill variety, discipline, threat targeting, unit radius and
  the AIM AND FIRE page. Switching it on or off takes effect at the next map start.

Co-op
  Only the host's AI matters: the host runs every actor, so the host's MENDICANT AI setting (and
  mods/mendicant_ai) decides how the AI plays for everyone. Clients need nothing special.

Sending logs
  After a problem, send:
    - {log}
    - debug.txt (beside the maps folder; a crash writes its report there)
  and say which level, difficulty and roughly when it happened.

Known issues
  - Test build: balance and drills are still being tuned; some fights may be harder or easier
    than stock.
  - The AI runs only on a campaign or co-op host, never in multiplayer games.
  - Changing CAMPAIGN AI on or off in the middle of a level applies from the next map start
    (the other settings apply at once).
  - mods/mendicant_ai holds the trained models; without it the drills are chosen by a scripted
    rule instead.

Licence
  OpenCE and this fork are under the GNU Affero General Public License 3.0 or later
  (LICENSE.txt). SOURCE.txt says where the source of this build is.
"""


def source_notice(revision, contact):
    return f"""Source code of this build
=========================

This build is a modified version of OpenCE ({UPSTREAM}), licensed under the
GNU Affero General Public License, version 3 or later (LICENSE.txt). It adds MENDICANT AI
(port/linux/game/mendicant/ in the source).

The complete corresponding source code of this build (the fork, branch "mendicant") is
available from {contact}, at no charge, for as long as this build is offered.

Build revision: {revision}

Under the AGPL, section 13: if you modify this program and let people interact with it over a
network (for example a dedicated server built from it), you must offer those people the
corresponding source of your version too. The licences of the third-party libraries are in the
*-LICENSE.txt, *-COPYING.txt and *-OFL.txt files beside it.
"""


def mod_toml():
    return (f'name = "{MOD_NAME}"\n'
            f'id = "mendicant_ai"\n'
            f'version = "{MOD_VERSION}"\n'
            f'description = "{MOD_DESCRIPTION}"\n')


def revision_text():
    """the fork's commit, and whether the tree had changes not committed"""
    try:
        head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True, text=True,
                              timeout=10, check=True).stdout.strip()
        dirty = subprocess.run(["git", "status", "--porcelain", "--untracked-files=no"], cwd=ROOT,
                               capture_output=True, text=True, timeout=10, check=True).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return "unknown"
    return head + (" with changes not yet committed" if dirty else "")


def personal_markers():
    """the strings of this machine's paths that must not be shipped, longest first"""
    markers = {str(ROOT), str(Path.home())}
    return sorted((m.encode() for m in markers if len(m) > 1), key=len, reverse=True)


def neutral(marker):
    """a path of the same length as marker that names nothing of this machine"""
    tail = b"/opence"
    return b"/" * (len(marker) - len(tail)) + tail if len(marker) > len(tail) else b"/" * len(marker)


def is_elf(data):
    return data[:4] == b"\x7fELF"


def strip_debug(data, name, symbols):
    """an ELF file without its debug information; the full copy saved in symbols/"""
    if b".debug_info" not in data and b".debug_line" not in data:
        return data
    symbols.mkdir(parents=True, exist_ok=True)
    (symbols / name).write_bytes(data)
    with tempfile.TemporaryDirectory(dir=symbols) as scratch:
        source, target = Path(scratch) / "in", Path(scratch) / "out"
        source.write_bytes(data)
        subprocess.run(["objcopy", "--strip-debug", str(source), str(target)], check=True)
        return target.read_bytes()


def scrub(data, path, symbols):
    """the file's bytes made shippable: debug information stripped, this machine's paths
    overwritten (binaries) or refused (text)"""
    if is_elf(data):
        data = strip_debug(data, PurePosixPath(path).name, symbols)
    binary = b"\0" in data[:8192] or is_elf(data) or data[:2] == b"MZ"
    for marker in personal_markers():
        if marker in data:
            if not binary:
                raise SystemExit(f"{path}: names {marker.decode()}; refusing to package it")
            print(f"{path}: {data.count(marker)} copies of a local path overwritten", flush=True)
            data = data.replace(marker, neutral(marker))
    return data


class Package:
    def __init__(self, platform):
        self.platform = platform
        self.files = {}  # name in the zip -> (bytes, executable)

    def add(self, name, data, executable=False):
        name = PurePosixPath(name).as_posix()
        if name in self.files:
            raise SystemExit(f"{name}: added twice")
        self.files[name] = (data, executable)

    def add_tree(self, source, prefix):
        """a repository folder, as tools/ci_build.py copies it (less zips and __pycache__)"""
        for path in sorted(source.rglob("*")):
            relative = path.relative_to(source).as_posix()
            if path.is_file() and not re.search(r"(^|/)__pycache__/|\.zip$", relative):
                self.add(f"{prefix}/{relative}", path.read_bytes(), os.access(path, os.X_OK))


def pe_machine(data):
    """a Windows program's machine (0x14c 32-bit x86, 0x8664 x86-64), or None"""
    if data[:2] != b"MZ" or len(data) < 0x40:
        return None
    offset = int.from_bytes(data[0x3c:0x40], "little")
    if data[offset:offset + 4] != b"PE\0\0":
        return None
    return int.from_bytes(data[offset + 4:offset + 6], "little")


def platform_of(names, halo=None, exe=None):
    """the package's platform from the game's file names, and halo's first bytes (an ELF file's
    class: 1 32-bit, 2 64-bit) or halo.exe's headers (its machine)"""
    if "halo.exe" in names:
        return "windows-x64" if exe is not None and pe_machine(exe) == 0x8664 else "windows"
    if "halo" in names:
        if halo is None or not is_elf(halo):
            raise SystemExit("halo in --game is not an ELF file")
        return "linux-x86_64" if halo[4] == 2 else "linux"
    raise SystemExit("no halo or halo.exe in --game: not a game folder")


def from_build_folder(game, package):
    """the files of tools/ci_build.py's dist/ for this platform"""
    for name in BUILD_OUTPUTS[package.platform]:
        path = game / name
        if not path.is_file():
            raise SystemExit(f"{path}: missing (built with ninja {package.platform}?)")
        package.add(name, path.read_bytes(), name in EXECUTABLE_NAMES)
    files, folders = list(REPOSITORY_FILES), list(REPOSITORY_FOLDERS)
    if package.platform in ("linux-x86_64", "windows-x64"):
        files = [(source, name) for source, name in files if name not in SERVER_EXAMPLES]
        folders = [(source, name) for source, name in folders if name not in SERVER_EXAMPLES]
    if package.platform == "linux":
        files += LINUX_ONLY_FILES
        folders += LINUX_ONLY_FOLDERS
    for source, name in files:
        if (ROOT / source).is_file():
            package.add(name, (ROOT / source).read_bytes(), name in EXECUTABLE_NAMES)
    for source, name in folders:
        package.add_tree(ROOT / source, name)


def from_release(entries, package):
    """a release's files as they are (entries: name -> bytes, executable)"""
    for name in sorted(entries):
        if LEFT_OUT.search(name):
            continue
        data, executable = entries[name]
        package.add(name, data, executable or PurePosixPath(name).name in EXECUTABLE_NAMES)


def release_entries(game):
    """name -> (bytes, executable) of a release folder or zip, from its top (a zip whose files are
    all in one folder is read from inside it)"""
    entries = {}
    if game.is_file():
        with zipfile.ZipFile(game) as archive:
            for info in archive.infolist():
                if not info.is_dir():
                    mode = info.external_attr >> 16
                    entries[info.filename] = (archive.read(info), bool(mode & 0o111))
        tops = {PurePosixPath(name).parts[0] for name in entries}
        if len(tops) == 1 and not any(len(PurePosixPath(n).parts) == 1 for n in entries):
            top = tops.pop()
            entries = {PurePosixPath(n).relative_to(top).as_posix(): v for n, v in entries.items()}
    else:
        for path in sorted(game.rglob("*")):
            if path.is_file():
                entries[path.relative_to(game).as_posix()] = (path.read_bytes(), os.access(path, os.X_OK))
    for name in entries:
        if PurePosixPath(name).is_absolute() or ".." in PurePosixPath(name).parts:
            raise SystemExit(f"{game}: unsafe name {name!r}")
    return entries


def write_zip(package, out):
    stamp = time.localtime()[:6]
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in sorted(package.files):
            data, executable = package.files[name]
            info = zipfile.ZipInfo(name, stamp)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = ((stat.S_IFREG | (0o755 if executable else 0o644)) << 16)
            archive.writestr(info, data)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game", required=True, type=Path,
                        help="build/linux, build/linux_x86_64, build/windows, or a release folder or zip")
    parser.add_argument("--models", required=True, type=Path,
                        help="models.aibw (tools/mendicant/export_models.py)")
    parser.add_argument("--out", required=True, type=Path, help="the folder the zip goes in")
    parser.add_argument("--contact", default="the author of this build (the person who sent it to you)",
                        help="who the source is available from (SOURCE.txt)")
    args = parser.parse_args()

    game = args.game.resolve()
    models = args.models.read_bytes()
    if models[:4] != b"AIBW":
        raise SystemExit(f"{args.models}: not a models file (no AIBW magic)")
    if game.is_dir() and (game / "build.ninja").exists():
        raise SystemExit(f"{game}: the repository, not a game folder")
    build_folder = game.is_dir() and not (game / "LICENSE.txt").exists()
    if build_folder:
        names = {path.name for path in game.iterdir()}
        package = Package(platform_of(names, (game / "halo").read_bytes()[:8] if "halo" in names else None,
                                      (game / "halo.exe").read_bytes()[:4096] if "halo.exe" in names else None))
        from_build_folder(game, package)
    else:
        entries = release_entries(game)
        package = Package(platform_of(set(entries), entries["halo"][0][:8] if "halo" in entries else None,
                                      entries["halo.exe"][0][:4096] if "halo.exe" in entries else None))
        from_release(entries, package)
        if "LICENSE.txt" in package.files:
            del package.files["LICENSE.txt"]
        package.add("LICENSE.txt", (ROOT / "LICENSE").read_bytes())

    stem = f"mendicant-ai-{MOD_VERSION}-{package.platform}"
    args.out.mkdir(parents=True, exist_ok=True)
    symbols = args.out / f"{stem}-symbols"
    if symbols.exists():
        shutil.rmtree(symbols)
    package.files = {name: (scrub(data, name, symbols), executable)
                     for name, (data, executable) in package.files.items()}
    package.add(f"{MOD_FOLDER}/mod.toml", mod_toml().encode())
    package.add(f"{MOD_FOLDER}/tags/mendicant/models.aibw", models)
    newline = "\r\n" if package.platform.startswith("windows") else "\n"
    package.add("README_MENDICANT.txt", readme(package.platform).replace("\n", newline).encode())
    package.add("SOURCE.txt", scrub(source_notice(revision_text(), args.contact).replace("\n", newline)
                                    .encode(), "SOURCE.txt", symbols))

    out = args.out / f"{stem}.zip"
    write_zip(package, out)
    total = sum(len(data) for data, _ in package.files.values())
    print(f"{out}: {len(package.files)} files, {total} bytes unpacked, {out.stat().st_size} bytes zipped",
          flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
