#!/usr/bin/env python3
"""Lays out the Linux release (tools/ci_build.py), and zips it:

    python tools/linux_package.py tree <folder>    (the tree of build/linux's build)
    python tools/linux_package.py zip dist/halo-linux-i686-release halo-linux-i686-release.zip

The release is a tree laid out as a Unix prefix is, which a package manager
can copy to /usr (or /opt/<name>) as it is, and which a player unpacks
anywhere and starts with bin/opence:

    bin/opence                                 the game
    lib/opence/libSDL3.so.0                    its SDL (the rpath is $ORIGIN:$ORIGIN/../lib/opence)
    share/opence/brokers.txt                   internet play's brokers (network.brokers_file)
    share/applications/opence.desktop          the desktop entry
    share/icons/hicolor/<n>x<n>/apps/opence.png  its icon
    share/doc/opence/                          the READMEs
    share/licenses/opence/                     the project's licence and those of what it carries

The game finds its files from where it is (port/linux/src/install_layout.c).
The releases before the tree were flat: everything in one folder, the game
named halo. Their self-updaters, which cannot change, unpack only a zip's
top-level files, and ask for halo-linux-<config>.zip; so the release keeps
that zip, the same build laid out flat (stage_flat), for them. The game
that build installs updates to the tree (port/linux/src/updater.c).

tree lays out build/linux's build in a folder, for a package built from the
sources: its contents go into the package's /usr. A build made with
configure.py --portable brings its SDL (libSDL3.so.0 and SDL3-LICENSE.txt);
another has none, and uses the system's. zip writes the
folder's files in name order, with their Unix modes
(executables by their place, as GitHub's artifacts lose the modes), for
unzip and the self-updater.
"""

import argparse
import shutil
import stat
import sys
import zipfile
from pathlib import Path
from typing import Iterable, List, Optional, Tuple

ROOT = Path(__file__).resolve().parent.parent
NAME = "opence"  # install_layout.h's INSTALL_NAME

DESKTOP_ENTRY = ROOT / "port/assets/linux/opence.desktop"
ICONS = ROOT / "port/assets/icon/hicolor"  # opence-<size>.png (tools/app_icon.py)
BROKERS = ROOT / "port/assets/network/brokers.txt"
DOCUMENTS = [(ROOT / "README.md", "README.md"), (ROOT / "port/linux/README.md", "README.linux.md")]
PROJECT_LICENSE = ROOT / "LICENSE.md"


def notices(platform: str) -> List[Tuple[Path, str]]:
    """the licences of what a platform's build carries, which its release
    has beside it ((source, name in the release) pairs; tools/ci_build.py's
    for every platform; the Linux build's SDL3-LICENSE.txt is apart)"""
    pairs = [
        # the disc image readers (port/linux/src/xiso.c, and the Android
        # app's XisoExtractor.java) follow extract-xiso, whose license asks
        # binaries to carry its notice
        (ROOT / "port/third_party/extract-xiso/LICENSE.TXT", "extract-xiso-LICENSE.txt"),
    ]
    if platform != "web":
        # internet play's DTLS with browsers, and the Linux self-updater's
        # TLS (port/third_party/mbedtls), whose Apache license asks the same
        pairs.append((ROOT / "port/third_party/mbedtls/LICENSE", "mbedtls-LICENSE.txt"))
    pairs += [
        # internet play's UPnP (port/third_party/miniupnpc), in every build,
        # whose BSD license asks binaries to carry its notice
        (ROOT / "port/third_party/miniupnpc/LICENSE", "miniupnpc-LICENSE.txt"),
        # the text's fonts (port/assets/fonts), embedded in every build, whose
        # SIL Open Font License asks each copy to carry it
        (ROOT / "port/assets/fonts/Overpass-OFL.txt", "Overpass-OFL.txt"),
        # the menus' XML parser (port/third_party/expat), in every build,
        # whose MIT license asks copies to carry its notice
        (ROOT / "port/third_party/expat/COPYING", "expat-COPYING.txt"),
        # voice chat's codec (port/third_party/opus), in every build, whose
        # BSD license asks binaries to carry its notice
        (ROOT / "port/third_party/opus/COPYING", "opus-COPYING.txt"),
        # voice chat's speaker icons (port/assets/icons/lucide, drawn into
        # the menus' bitmaps), whose ISC license asks copies to carry its
        # notice
        (ROOT / "port/assets/icons/lucide/LICENSE", "lucide-LICENSE.txt"),
    ]
    return pairs


def _copy(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def stage_tree(dist: Path, executable: Path, library: Optional[Path], notices: Iterable[Tuple[Path, str]]) -> None:
    """The release's tree in dist: the executable as bin/opence, SDL in
    lib/opence (none: the system's), and notices ((source, name) pairs: the
    licences of what the build carries) in share/licenses/opence."""
    _copy(executable, dist / "bin" / NAME)
    if library:
        _copy(library, dist / "lib" / NAME / library.name)
    _copy(BROKERS, dist / "share" / NAME / BROKERS.name)
    _copy(DESKTOP_ENTRY, dist / "share/applications" / f"{NAME}.desktop")
    for icon in sorted(ICONS.glob(f"{NAME}-*.png")):
        size = icon.stem.rsplit("-", 1)[1]
        _copy(icon, dist / "share/icons/hicolor" / f"{size}x{size}" / "apps" / f"{NAME}.png")
    for source, name in DOCUMENTS:
        _copy(source, dist / "share/doc" / NAME / name)
    _copy(PROJECT_LICENSE, dist / "share/licenses" / NAME / PROJECT_LICENSE.name)
    for source, name in notices:
        _copy(source, dist / "share/licenses" / NAME / name)


def stage_flat(dist: Path, executable: Path, library: Path, notices: Iterable[Tuple[Path, str]]) -> None:
    """The flat release in dist, as the releases before the tree were: halo,
    libSDL3.so.0, brokers.txt and the notices in one folder (for their
    self-updaters)."""
    _copy(executable, dist / "halo")
    _copy(library, dist / library.name)
    _copy(BROKERS, dist / BROKERS.name)
    for source, name in notices:
        _copy(source, dist / name)


def mode(name: str) -> int:
    """a file's mode in the zip, from its place: the executables and the
    shared libraries of the tree and of the flat release are 0755"""
    parts = name.split("/")
    if parts[0] == "bin" and len(parts) == 2:
        return 0o755
    if parts == ["halo"] or (".so" in parts[-1] and (len(parts) == 1 or parts[0] == "lib")):
        return 0o755
    return 0o644


def write_zip(folder: Path, output: Path) -> None:
    """folder's files (and its folders) into the zip output, in name order"""
    entries = sorted(path for path in folder.rglob("*"))
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in entries:
            name = path.relative_to(folder).as_posix()
            if path.is_dir():
                info = zipfile.ZipInfo(name + "/", date_time=(1980, 1, 1, 0, 0, 0))
                info.create_system = 3
                info.external_attr = (stat.S_IFDIR | 0o755) << 16 | 0x10
                archive.writestr(info, b"")
                continue
            info = zipfile.ZipInfo.from_file(path, name)
            info.create_system = 3
            info.external_attr = (stat.S_IFREG | mode(name)) << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            with open(path, "rb") as file:
                archive.writestr(info, file.read(), compresslevel=9)
    print(f"{folder} -> {output}", flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    tree_command = commands.add_parser("tree", help="lay out build/linux's portable build as the release's tree")
    tree_command.add_argument("folder", type=Path)
    tree_command.add_argument("--build", type=Path, default=ROOT / "build/linux",
                              help="the build's folder (default: build/linux)")
    zip_command = commands.add_parser("zip", help="zip a staged release with its files' modes")
    zip_command.add_argument("folder", type=Path)
    zip_command.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "tree":
        library = args.build / "libSDL3.so.0"
        sdl_license = args.build / "SDL3-LICENSE.txt"
        if not (args.build / "halo").exists():
            parser.error(f"{args.build} has no build: enter ninja linux first")
        if library.exists() and sdl_license.exists():
            stage_tree(args.folder, args.build / "halo", library,
                       notices("linux") + [(sdl_license, sdl_license.name)])
        else:
            stage_tree(args.folder, args.build / "halo", None, notices("linux"))
        print(f"{args.build} -> {args.folder}", flush=True)
    elif args.command == "zip":
        write_zip(args.folder, args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
