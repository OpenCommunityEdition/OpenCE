"""Where the Linux build finds its files and puts the player's (port/linux/src/install_layout.c): install_layout.c
compiled as it is with a main that prints its answers, run from the places a release can be: the release's tree
unpacked (portable), a flat folder (build/linux/halo, the flat release), a read-only prefix (/usr), a read-only portable
copy that has its own config.toml and maps/, a tree with share/opence/packaged, and installs without a home folder.
Needs a C compiler (clang, or CC), not the game.

    python -m pytest -q tools/test_install_layout.py
"""

import os
import shutil
import stat
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "port/linux/src/install_layout.c"

MAIN = r"""
#include <stdio.h>
#include "install_layout.h"

int main(void)
{
	char brokers[1024];

	install_shared_file("brokers.txt", brokers, sizeof(brokers));
	printf("folder=%s\ntree=%d\nsystem=%d\nreason=%s\nconfig=%s\ndata=%s\nbrokers=%s\n", install_folder(),
		install_is_tree(), install_is_system(), install_system_reason(), install_config_folder(),
		install_data_folder(), brokers);
	return 0;
}
"""


@pytest.fixture(scope="module")
def program(tmp_path_factory):
    work = tmp_path_factory.mktemp("install_layout")
    (work / "main.c").write_text(MAIN)
    executable = work / "layout"
    compiler = os.environ.get("CC", "clang")
    if not shutil.which(compiler):
        pytest.skip(f"no {compiler}")
    subprocess.run([compiler, "-std=gnu11", "-Wall", "-Werror", "-I", str(SOURCE.parent), str(work / "main.c"),
                    str(SOURCE), "-o", str(executable), "-lpthread"], check=True)
    return executable


def answers(executable, home):
    """home None: no HOME (nor XDG_CONFIG_HOME, XDG_DATA_HOME)"""
    environment = {"PATH": os.environ.get("PATH", "")}
    if home is not None:
        environment["HOME"] = str(home)
    output = subprocess.run([str(executable)], env=environment, capture_output=True, text=True, check=True).stdout
    return dict(line.split("=", 1) for line in output.splitlines())


def install(program, folder, *parts):
    destination = folder.joinpath(*parts)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(program, destination)
    return destination


def tree(program, root):
    executable = install(program, root, "bin", "opence")
    (root / "share/opence").mkdir(parents=True)
    return executable


def test_tree_is_portable(program, tmp_path):
    root = tmp_path / "OpenCE"
    found = answers(tree(program, root), tmp_path / "home")
    assert found == {"folder": str(root), "tree": "1", "system": "0", "reason": "", "config": f"{root}/",
                     "data": str(root), "brokers": f"{root}/share/opence/brokers.txt"}


def test_flat_folder(program, tmp_path):
    root = tmp_path / "halo"
    found = answers(install(program, root, "halo"), tmp_path / "home")
    assert found["folder"] == str(root) and found["tree"] == "0" and found["system"] == "0"
    assert found["config"] == f"{root}/" and found["data"] == str(root)
    assert found["brokers"] == f"{root}/brokers.txt"


def test_bin_without_share_is_flat(program, tmp_path):
    """a bin folder alone (~/bin/opence) is not a tree: the game keeps its files beside it"""
    found = answers(install(program, tmp_path, "bin", "opence"), tmp_path / "home")
    assert found["tree"] == "0" and found["folder"] == str(tmp_path / "bin")


def test_read_only_prefix_is_installed(program, tmp_path):
    prefix = tmp_path / "usr"
    executable = tree(program, prefix)
    home = tmp_path / "home"
    prefix.chmod(stat.S_IRUSR | stat.S_IXUSR)
    try:
        if os.access(prefix, os.W_OK):
            pytest.skip("this user can write to a read-only folder (root)")
        found = answers(executable, home)
    finally:
        prefix.chmod(stat.S_IRWXU)
    assert found["system"] == "1" and found["reason"] == "its folder is read-only"
    assert found["config"] == f"{home}/.config/halo-linux/"
    assert found["data"] == f"{home}/.local/share/halo-linux"
    assert found["brokers"] == f"{prefix}/share/opence/brokers.txt"
    assert (home / ".config/halo-linux").is_dir() and (home / ".local/share/halo-linux").is_dir()


def test_packaged_marker(program, tmp_path):
    prefix = tmp_path / "usr"
    executable = tree(program, prefix)
    (prefix / "share/opence/packaged").write_text("")
    home = tmp_path / "home"
    found = answers(executable, home)
    assert found["system"] == "1" and found["reason"] == "share/opence/packaged"
    assert found["config"] == f"{home}/.config/halo-linux/"


def test_xdg_folders(program, tmp_path):
    prefix = tmp_path / "usr"
    executable = tree(program, prefix)
    (prefix / "share/opence/packaged").write_text("")
    environment = {"PATH": os.environ.get("PATH", ""), "HOME": str(tmp_path / "home"),
                   "XDG_CONFIG_HOME": str(tmp_path / "config"), "XDG_DATA_HOME": str(tmp_path / "data")}
    output = subprocess.run([str(executable)], env=environment, capture_output=True, text=True, check=True).stdout
    found = dict(line.split("=", 1) for line in output.splitlines())
    assert found["config"] == f"{tmp_path}/config/halo-linux/"
    assert found["data"] == f"{tmp_path}/data/halo-linux"


def test_symlinked_executable(program, tmp_path):
    """/usr/bin/opence as a link to the tree's: the tree is the link's target's"""
    root = tmp_path / "opt/opence"
    executable = tree(program, root)
    link = tmp_path / "opence"
    link.symlink_to(executable)
    found = answers(link, tmp_path / "home")
    assert found["folder"] == str(root) and found["tree"] == "1"


def read_only(folder):
    """folder (and its folders) read-only for the test's run; skips when this user can write anyway (root)"""
    class ReadOnly:
        def __enter__(self):
            self.folders = [folder] + [path for path in folder.rglob("*") if path.is_dir()]
            for path in reversed(self.folders):
                path.chmod(stat.S_IRUSR | stat.S_IXUSR)
            if os.access(folder, os.W_OK):
                self.__exit__()
                pytest.skip("this user can write to a read-only folder (root)")

        def __exit__(self, *exception):
            for path in self.folders:
                path.chmod(stat.S_IRWXU)
    return ReadOnly()


def test_read_only_flat_folder_keeps_its_files(program, tmp_path):
    """a flat portable copy on a read-only disk with its own config.toml and maps/: both are read there, nothing
    goes to the home folder, and the game does not update itself (it could not)"""
    root = tmp_path / "stick/halo"
    executable = install(program, root, "halo")
    (root / "config.toml").write_text("[game]\n")
    (root / "Maps").mkdir()
    home = tmp_path / "home"
    with read_only(root):
        found = answers(executable, home)
    assert found["system"] == "1" and found["reason"] == "its folder is read-only"
    assert found["config"] == f"{root}/" and found["data"] == str(root)
    assert found["brokers"] == f"{root}/brokers.txt"
    assert not (home / ".config/halo-linux").exists() and not (home / ".local/share/halo-linux").exists()


def test_read_only_folder_without_config(program, tmp_path):
    """/opt/halo with the maps an administrator put there, and no config.toml: the maps are read there, and
    config.toml (which the game writes) goes to the home folder"""
    root = tmp_path / "opt/halo"
    executable = install(program, root, "halo")
    (root / "maps").mkdir()
    home = tmp_path / "home"
    with read_only(root):
        found = answers(executable, home)
    assert found["system"] == "1"
    assert found["config"] == f"{home}/.config/halo-linux/" and found["data"] == str(root)
    assert (home / ".config/halo-linux").is_dir() and not (home / ".local/share/halo-linux").exists()


def test_read_only_tree_keeps_its_files(program, tmp_path):
    """the release's tree, portable, on a read-only disk: its config.toml and maps/ as the flat folder's"""
    root = tmp_path / "stick/OpenCE"
    executable = tree(program, root)
    (root / "config.toml").write_text("[game]\n")
    (root / "maps").mkdir()
    home = tmp_path / "home"
    with read_only(root):
        found = answers(executable, home)
    assert found["tree"] == "1" and found["system"] == "1" and found["reason"] == "its folder is read-only"
    assert found["config"] == f"{root}/" and found["data"] == str(root)
    assert found["brokers"] == f"{root}/share/opence/brokers.txt"


def test_read_only_tree_without_files(program, tmp_path):
    """the same tree with only its config.toml: the game data goes to the home folder"""
    root = tmp_path / "stick/OpenCE"
    executable = tree(program, root)
    (root / "config.toml").write_text("[game]\n")
    home = tmp_path / "home"
    with read_only(root):
        found = answers(executable, home)
    assert found["config"] == f"{root}/" and found["data"] == f"{home}/.local/share/halo-linux"


def test_packaged_ignores_the_folders_files(program, tmp_path):
    """a package's tree is installed as designed: its folder's config.toml and maps/ are not the player's"""
    prefix = tmp_path / "usr"
    executable = tree(program, prefix)
    (prefix / "share/opence/packaged").write_text("")
    (prefix / "config.toml").write_text("[game]\n")
    (prefix / "maps").mkdir()
    home = tmp_path / "home"
    found = answers(executable, home)
    assert found["system"] == "1" and found["reason"] == "share/opence/packaged"
    assert found["config"] == f"{home}/.config/halo-linux/"
    assert found["data"] == f"{home}/.local/share/halo-linux"


def test_packaged_without_home(program, tmp_path):
    """a package's tree with no HOME: the files stay in its folder, and it still does not update itself"""
    prefix = tmp_path / "usr"
    executable = tree(program, prefix)
    (prefix / "share/opence/packaged").write_text("")
    found = answers(executable, None)
    assert found["system"] == "1" and found["reason"] == "share/opence/packaged"
    assert found["config"] == f"{prefix}/" and found["data"] == str(prefix)


def test_read_only_prefix_without_home(program, tmp_path):
    """a tree in a read-only /usr with no HOME: installed (no self-update) all the same"""
    prefix = tmp_path / "usr"
    executable = tree(program, prefix)
    with read_only(prefix):
        found = answers(executable, None)
    assert found["system"] == "1" and found["reason"] == "its folder is read-only"
    assert found["config"] == f"{prefix}/" and found["data"] == str(prefix)


def test_portable_without_home(program, tmp_path):
    """a portable tree with no HOME stays portable"""
    root = tmp_path / "OpenCE"
    found = answers(tree(program, root), None)
    assert found["system"] == "0" and found["config"] == f"{root}/" and found["data"] == str(root)
