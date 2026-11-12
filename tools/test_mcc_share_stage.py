"""Managed MCC staging preserves original files and refuses filesystem links."""
from pathlib import Path
import os
import subprocess
import sys

import pytest
from mcc_test_support import compile_tool

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def stage_tool(tmp_path_factory):
    return compile_tool(tmp_path_factory.mktemp("mcc-stage"), [
        ROOT / "tools/tests/mcc_share_stage.c",
        ROOT / "port/linux/game/mcc_share_stage.c",
        ROOT / "port/linux/game/mcc_share_rules.c",
        ROOT / "port/linux/game/mcc_package.c",
        ROOT / "port/linux/game/mcc_sha256.c",
        ROOT / "port/linux/src/posix_mcc_share.c",
    ])


def run(tool, *args):
    result = subprocess.run([str(tool), *map(str, args)], capture_output=True, text=True, timeout=20)
    assert result.returncode == 0, result.stderr
    return result.stdout.strip()


@pytest.mark.parametrize("name", ["../escape.json", "/escape.json", "a/../escape.json",
                                    "a\\..\\escape.json", "CON.json", "a/LPT1.json", "a:stream.json",
                                    "a//b.json", "a./b.json", "a /b.json", "a.json.exe"])
def test_bad_relative_names(stage_tool, tmp_path, name):
    assert run(stage_tool, "manifest", tmp_path, name) == "0"


def test_case_alias_file_directory_collision(stage_tool, tmp_path):
    assert run(stage_tool, "collision", tmp_path) == "0"


def test_aggregate_preview_budget(stage_tool, tmp_path):
    assert run(stage_tool, "budget", tmp_path) == "1 0"


def test_partial_file_race_does_not_truncate_or_delete(stage_tool, tmp_path):
    assert run(stage_tool, "partial_collision", tmp_path) == "write 7"
    assert (tmp_path / "mcc_staging/snapshot/m0.map.part").read_bytes() == b"not ours"
    assert not (tmp_path / "mcc_maps/snapshot").exists()


def test_atomic_nested_install(stage_tool, tmp_path):
    assert run(stage_tool, "install", tmp_path) == "install 0"
    package = tmp_path / "mcc_maps/snapshot"
    assert {p.relative_to(package).as_posix(): p.read_bytes() for p in package.rglob("*") if p.is_file()} == {
        "m0.map": b"x", "metadata/m0.json": b"x", "assets/m0.png": b"x"}
    assert not (tmp_path / "mcc_staging/snapshot").exists()


def test_direct_install_requires_no_file_or_directory_rename(stage_tool, tmp_path):
    assert run(stage_tool, "direct_install", tmp_path) == "install 0 pending 0 committed 1"
    package = tmp_path / "mcc_maps/snapshot"
    assert {p.relative_to(package).as_posix(): p.read_bytes() for p in package.rglob("*") if p.is_file()} == {
        "m0.map": b"x", "metadata/m0.json": b"x", "assets/m0.png": b"x", "download.receipt": b"receipt"}
    assert not (tmp_path / "mcc_staging").exists()


@pytest.mark.parametrize("mode,expected", [
    ("direct_cancel", "cancel pending 0 committed 0"),
    ("direct_unverified", "install 4 pending 0 committed 0"),
    ("direct_flush_fail", "write 7 pending 0 committed 0"),
    ("direct_commit_fail", "install 10 pending 0 committed 0"),
])
def test_direct_failure_cleans_owned_content_and_releases_lease(stage_tool, tmp_path, mode, expected):
    original = tmp_path / "mcc_maps/original/savegame.bin"
    original.parent.mkdir(parents=True)
    original.write_bytes(b"preserve")
    assert run(stage_tool, mode, tmp_path) == expected
    assert not (tmp_path / "mcc_maps/snapshot").exists()
    assert original.read_bytes() == b"preserve"


def test_direct_existing_destination_is_never_owned(stage_tool, tmp_path):
    original = tmp_path / "mcc_maps/snapshot/m0.map"
    original.parent.mkdir(parents=True)
    original.write_bytes(b"preserve")
    assert run(stage_tool, "direct_install", tmp_path) == "begin 9"
    assert original.read_bytes() == b"preserve"


def test_uncertain_commit_is_retained_even_after_repeated_cancel(stage_tool, tmp_path):
    assert run(stage_tool, "direct_commit_uncertain", tmp_path) == "install 11 pending 0 committed 1"
    package = tmp_path / "mcc_maps/snapshot"
    assert (package / "m0.map").read_bytes() == b"x"
    assert (package / "download.receipt").read_bytes() == b"receipt"


def test_uncertain_begin_leaves_recovery_ownership(stage_tool, tmp_path):
    assert run(stage_tool, "direct_begin_uncertain", tmp_path) == "begin 11"
    assert (tmp_path / "mcc_maps/snapshot").is_dir()


def test_cancel_only_owned_nested_files(stage_tool, tmp_path):
    keep = tmp_path / "mcc_maps/original/savegame.bin"
    keep.parent.mkdir(parents=True)
    keep.write_bytes(b"preserve")
    assert run(stage_tool, "cancel", tmp_path) == "cancelled"
    assert not (tmp_path / "mcc_staging/snapshot").exists()
    assert keep.read_bytes() == b"preserve"


def test_existing_stage_is_not_reused_or_cleaned(stage_tool, tmp_path):
    keep = tmp_path / "mcc_staging/snapshot/notes.txt"
    keep.parent.mkdir(parents=True)
    keep.write_bytes(b"not ours")
    assert run(stage_tool, "install", tmp_path).startswith("begin ")
    assert keep.read_bytes() == b"not ours"


def test_stage_left_by_a_cut_off_download_is_cleared(stage_tool, tmp_path):
    # the folder's name comes from the offer: a crash must not block it for good
    stage = tmp_path / "mcc_staging/snapshot"
    (stage / "metadata").mkdir(parents=True)
    (stage / "m0.map").write_bytes(b"x")
    (stage / "metadata/m0.json.part").write_bytes(b"partial")
    assert run(stage_tool, "install", tmp_path) == "install 0"
    assert not stage.exists()
    assert (tmp_path / "mcc_maps/snapshot/m0.map").read_bytes() == b"x"


def test_existing_destination_is_preserved(stage_tool, tmp_path):
    keep = tmp_path / "mcc_maps/snapshot/m0.map"
    keep.parent.mkdir(parents=True)
    keep.write_bytes(b"original")
    assert run(stage_tool, "install", tmp_path).startswith("begin ")
    assert keep.read_bytes() == b"original"
    assert not (tmp_path / "mcc_staging").exists()


def test_verification_is_required(stage_tool, tmp_path):
    assert run(stage_tool, "unverified", tmp_path) == "install 4"
    assert not (tmp_path / "mcc_maps/snapshot").exists()


def test_optional_preview_can_be_discarded(stage_tool, tmp_path):
    assert run(stage_tool, "discard", tmp_path) == "install 0"
    assert (tmp_path / "mcc_maps/snapshot/metadata/m0.json").is_file()
    assert not (tmp_path / "mcc_maps/snapshot/assets").exists()


def test_publish_never_replaces_existing_directory(stage_tool, tmp_path):
    source, target = tmp_path / "source", tmp_path / "target"
    source.mkdir(); target.mkdir()
    (source / "new").write_bytes(b"new")
    assert run(stage_tool, "publish", source, target) == "0"
    assert (source / "new").read_bytes() == b"new"
    assert list(target.iterdir()) == []


def test_staging_link_refused(stage_tool, tmp_path):
    outside = tmp_path / "outside"
    outside.mkdir()
    keep = outside / "snapshot/m0.map.part"
    keep.parent.mkdir()
    keep.write_bytes(b"preserve")
    link = tmp_path / "mcc_staging"
    if sys.platform == "win32":
        result = subprocess.run(["cmd", "/c", "mklink", "/J", str(link), str(outside)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
    else:
        link.symlink_to(outside, target_is_directory=True)
    try:
        assert run(stage_tool, "safe", link) == "0"
        assert run(stage_tool, "install", tmp_path).startswith("begin ")
        assert keep.read_bytes() == b"preserve"
    finally:
        if sys.platform == "win32":
            os.rmdir(link)
        else:
            link.unlink()


def make_link(link, target):
    if sys.platform == "win32":
        result = subprocess.run(["cmd", "/c", "mklink", "/J", str(link), str(target)], capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
    else:
        link.symlink_to(target, target_is_directory=True)


def remove_link(link):
    if sys.platform == "win32":
        os.rmdir(link)
    else:
        link.unlink()


def test_link_above_the_managed_folders_is_the_users_own(stage_tool, tmp_path):
    # ~/.steam/steam is a link to ~/.local/share/Steam; a data root may be too
    real = tmp_path / "real"
    (real / "steamapps/workshop/content/1695791/1").mkdir(parents=True)
    (real / "mcc_staging").mkdir()
    link = tmp_path / "root"
    make_link(link, real)
    try:
        assert run(stage_tool, "safe", link / "steamapps/workshop/content/1695791/1") == "1"
        assert run(stage_tool, "safe", link / "mcc_staging/snapshot") == "1"
        assert run(stage_tool, "install", link) == "install 0"
    finally:
        remove_link(link)


def test_link_below_the_managed_folders_is_refused(stage_tool, tmp_path):
    outside = tmp_path / "outside"
    outside.mkdir()
    (tmp_path / "steamapps/workshop/content").mkdir(parents=True)
    link = tmp_path / "steamapps/workshop/content/1695791"
    make_link(link, outside)
    try:
        assert run(stage_tool, "safe", link / "1") == "0"
        # a later folder of the same name does not move the boundary down
        assert run(stage_tool, "safe", tmp_path / "steamapps/workshop/content/1695791/mcc_maps") == "0"
    finally:
        remove_link(link)
