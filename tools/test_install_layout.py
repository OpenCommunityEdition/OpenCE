"""Where the Linux build finds its files and puts the player's (port/linux/src/install_layout.c): install_layout.c
compiled as it is with a main that prints its answers, run from the places a release can be: the release's tree
unpacked (portable), a flat folder (build/linux/halo, the flat release), a read-only prefix (/usr), and a tree with
share/opence/packaged. Needs a C compiler (clang, or CC), not the game.

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
    environment = {"PATH": os.environ.get("PATH", ""), "HOME": str(home)}
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
