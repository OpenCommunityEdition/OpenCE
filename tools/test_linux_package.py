"""The Linux release's layout (tools/linux_package.py): the tree that packages copy to /usr, the flat release the
updaters of older builds read, the zips' modes, and that the game (install_layout.c, updater.c) and the desktop
entry agree with them. Needs no build: the files are stand-ins.

    python -m pytest -q tools/test_linux_package.py
"""

import re
import stat
import sys
import zipfile
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import linux_package  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent


@pytest.fixture
def build(tmp_path):
    """stand-ins for the build's executable, SDL and SDL's licence"""
    folder = tmp_path / "build"
    folder.mkdir()
    for name in ("halo", "libSDL3.so.0", "SDL3-LICENSE.txt"):
        (folder / name).write_bytes(name.encode())
    return folder


def notices(build):
    return linux_package.notices("linux") + [(build / "SDL3-LICENSE.txt", "SDL3-LICENSE.txt")]


def files(folder):
    return sorted(path.relative_to(folder).as_posix() for path in folder.rglob("*") if path.is_file())


def test_tree(build, tmp_path):
    tree = tmp_path / "tree"
    linux_package.stage_tree(tree, build / "halo", build / "libSDL3.so.0", notices(build))
    names = files(tree)
    assert (tree / "bin/opence").read_bytes() == b"halo"
    assert "lib/opence/libSDL3.so.0" in names
    assert "share/opence/brokers.txt" in names
    assert "share/applications/opence.desktop" in names
    assert "share/icons/hicolor/48x48/apps/opence.png" in names
    assert "share/licenses/opence/LICENSE.md" in names
    assert "share/licenses/opence/SDL3-LICENSE.txt" in names
    assert "share/doc/opence/README.md" in names
    # (nothing but the prefix's folders at its top)
    assert {name.split("/")[0] for name in names} == {"bin", "lib", "share"}


def test_tree_without_sdl(build, tmp_path):
    tree = tmp_path / "tree"
    linux_package.stage_tree(tree, build / "halo", None, linux_package.notices("linux"))
    assert not (tree / "lib").exists()


def test_flat(build, tmp_path):
    """what the updaters of the releases before the tree install: files at the zip's top, at most 32
    (their MAXIMUM_UPDATE_FILES), the executable named halo"""
    flat = tmp_path / "flat"
    linux_package.stage_flat(flat, build / "halo", build / "libSDL3.so.0", notices(build))
    names = files(flat)
    assert all("/" not in name for name in names)
    assert len(names) <= 32
    assert {"halo", "libSDL3.so.0", "brokers.txt", "SDL3-LICENSE.txt"} <= set(names)


def test_flat_files_are_those_the_update_to_the_tree_deletes(build, tmp_path):
    """updater.c deletes the flat release's files (all but halo) when the flat build updates to the tree"""
    flat = tmp_path / "flat"
    linux_package.stage_flat(flat, build / "halo", build / "libSDL3.so.0", notices(build))
    source = (ROOT / "port/linux/src/updater.c").read_text()
    table = source[source.index("updater_flat_files[] ="):]
    deleted = set(re.findall(r'"([^"]+)"', table[:table.index("};")]))
    assert set(files(flat)) - {"halo"} == deleted


def test_zip_modes(build, tmp_path):
    for stage, executables in ((linux_package.stage_tree, {"bin/opence", "lib/opence/libSDL3.so.0"}),
                               (linux_package.stage_flat, {"halo", "libSDL3.so.0"})):
        folder = tmp_path / stage.__name__
        # (as GitHub's artifacts give them: no file executable)
        stage(folder, build / "halo", build / "libSDL3.so.0", notices(build))
        for path in folder.rglob("*"):
            if path.is_file():
                path.chmod(0o644)
        output = tmp_path / f"{stage.__name__}.zip"
        linux_package.write_zip(folder, output)
        with zipfile.ZipFile(output) as archive:
            for info in archive.infolist():
                assert info.create_system == 3
                mode = info.external_attr >> 16
                if info.is_dir():
                    assert stat.S_ISDIR(mode)
                    continue
                assert stat.S_ISREG(mode)
                expected = 0o755 if info.filename in executables else 0o644
                assert stat.S_IMODE(mode) == expected, info.filename
            assert archive.namelist() == sorted(archive.namelist())


def test_names_agree():
    """the executable's, the folders' and the desktop entry's name are install_layout.h's"""
    header = (ROOT / "port/linux/src/install_layout.h").read_text()
    assert re.search(r'#define INSTALL_NAME "([^"]+)"', header).group(1) == linux_package.NAME
    entry = dict(line.split("=", 1) for line in linux_package.DESKTOP_ENTRY.read_text().splitlines() if "=" in line)
    assert entry["Exec"] == linux_package.NAME
    assert entry["Icon"] == linux_package.NAME
    assert entry["StartupWMClass"] == linux_package.NAME
    assert sorted(linux_package.ICONS.glob(f"{linux_package.NAME}-48.png"))


def test_asset_names_agree():
    """the 32-bit x86 tree's zip is the one its self-updater asks for (halo-linux-i686-<flavor>.zip), and the flat
    one keeps the name the updaters of the releases before the tree ask for (halo-linux-<flavor>.zip)"""
    updater = (ROOT / "port/linux/src/updater.c").read_text()
    platforms = re.findall(r'#define UPDATE_PLATFORM "([^"]+)"', updater)
    assert "linux-i686" in platforms and "linux" not in platforms
    assert '#define UPDATE_ASSET "halo-" UPDATE_PLATFORM "-" HALO_BUILD_FLAVOR ".zip"' in updater
    ci_build = (ROOT / "tools/ci_build.py").read_text()
    assert 'f"halo-linux-i686-{args.config}", linux_package.stage_tree' in ci_build
    assert 'f"halo-linux-{args.config}", linux_package.stage_flat' in ci_build
    workflow = (ROOT / ".github/workflows/build.yml").read_text()
    for config in ("debug", "release"):
        assert f"name: halo-linux-i686-{config}" in workflow and f"name: halo-linux-{config}" in workflow
