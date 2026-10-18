"""Published history, rather than the last push, bounds release highlights."""

from types import SimpleNamespace
import json
from pathlib import Path
import subprocess

import pytest

from changelog_release_base import download_archive, release_base


def release(tag, date, draft=False, prerelease=False):
    return dict(tagName=tag, publishedAt=date, isDraft=draft, isPrerelease=prerelease)


def test_skips_current_drafts_other_tags_and_unrelated_history():
    calls = []

    def run(command, **kwargs):
        calls.append(command)
        if command[1] == "rev-parse":
            return SimpleNamespace(stdout="a" * 40 if "build-2" in command[-1] else "b" * 40)
        return SimpleNamespace(returncode=1 if command[-2] == "a" * 40 else 0, stderr="")

    releases = [release("build-1", "2026-01-01"), release("build-2", "2026-01-02"),
                release("build-3", "2026-01-03"), release("other", "2026-01-04"),
                release("build-4", "2026-01-05", draft=True),
                release("build-5", "2026-01-06", prerelease=True)]
    assert release_base(releases, "c" * 40, "build-3", run) == ("b" * 40, "build-1")
    assert [call[-1] for call in calls if call[1] == "rev-parse"] == [
        "refs/tags/build-2^{commit}", "refs/tags/build-1^{commit}"]


def test_no_published_build_uses_collector_baseline():
    assert release_base([], "a" * 40, "build-1") is None


def test_bad_head_and_git_errors_are_not_hidden():
    with pytest.raises(ValueError):
        release_base([], "--help", "build-1")

    def failed(command, **kwargs):
        return SimpleNamespace(stdout="b" * 40, returncode=128, stderr="broken repository")

    with pytest.raises(RuntimeError, match="broken repository"):
        release_base([release("build-1", "2026-01-01")], "a" * 40, "build-2", failed)


def test_real_tags_choose_published_ancestor_after_failed_build(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)

    def git(*args):
        return subprocess.check_output(["git", *args], stderr=subprocess.PIPE, text=True).strip()

    git("init", "-b", "main")
    git("config", "user.name", "Changelog test")
    git("config", "user.email", "changelog@example.invalid")
    git("config", "commit.gpgsign", "false")
    git("commit", "--allow-empty", "-m", "Published build")
    published = git("rev-parse", "HEAD")
    git("tag", "build-1")
    git("checkout", "-b", "unrelated")
    git("commit", "--allow-empty", "-m", "Unrelated release")
    git("tag", "build-2")
    git("checkout", "main")
    git("commit", "--allow-empty", "-m", "Build failed before publication")
    git("commit", "--allow-empty", "-m", "Current build")
    head = git("rev-parse", "HEAD")
    git("tag", "build-4")
    assert release_base([
        release("build-1", "2026-01-01"), release("build-2", "2026-01-02"),
        release("build-4", "2026-01-04"),
    ], head, "build-4") == (published, "build-1")


def archive_runner(data, size=100):
    def run(command, **kwargs):
        assert command[:2] == ["gh", "release"]
        if command[2] == "view":
            return SimpleNamespace(stdout=json.dumps({"assets": [{"name": "changelog.json", "size": size}]}))
        assert command[2:6] == ["download", "build-12", "--pattern", "changelog.json"]
        (Path(command[-1]) / "changelog.json").write_text(json.dumps(data), encoding="utf-8")
        return SimpleNamespace(stdout="")
    return run


def test_download_preserves_exact_numbered_archive(tmp_path):
    data = {"schema_version": 2, "head": "a" * 40,
            "entries": [{"build": 12, "commit": "a" * 40, "date": "2026-10-18", "notes": ["Fixed menus."]}]}
    output = tmp_path / "previous.json"
    assert download_archive(("a" * 40, "build-12"), output, archive_runner(data))
    assert json.loads(output.read_text()) == data
    assert list(tmp_path.iterdir()) == [output]


@pytest.mark.parametrize("field,value", [("head", "b" * 40), ("schema_version", 1), ("entries", [])])
def test_mismatched_archive_fails_without_overwriting_previous(tmp_path, field, value):
    data = {"schema_version": 2, "head": "a" * 40, "entries": [{"build": 12}]}
    data[field] = value
    output = tmp_path / "previous.json"
    output.write_text("preserved")
    with pytest.raises(ValueError, match="does not match"):
        download_archive(("a" * 40, "build-12"), output, archive_runner(data))
    assert output.read_text() == "preserved"
    assert list(tmp_path.iterdir()) == [output]


def test_wrong_build_number_or_oversized_archive_fails(tmp_path):
    data = {"schema_version": 2, "head": "a" * 40, "entries": [{"build": 13}]}
    with pytest.raises(ValueError, match="does not match"):
        download_archive(("a" * 40, "build-12"), tmp_path / "previous.json", archive_runner(data))
    with pytest.raises(ValueError, match="invalid size"):
        download_archive(("a" * 40, "build-12"), tmp_path / "previous.json", archive_runner(data, 3 * 1024 * 1024))


def test_pre_feature_release_or_no_release_removes_stale_archive(tmp_path):
    output = tmp_path / "previous.json"
    output.write_text("stale")
    assert not download_archive(None, output)
    assert not output.exists()
    output.write_text("stale")
    assert not download_archive(("a" * 40, "build-12"), output,
                                lambda *args, **kwargs: SimpleNamespace(stdout='{"assets": []}'))
    assert not output.exists()
