"""Exercise the note collector against actual temporary Git histories."""

import json
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import player_notes


class Repository:
    def __init__(self, root):
        self.root = root
        root.mkdir()
        self.git("init", "-b", "main")
        self.git("config", "user.name", "Notes test")
        self.git("config", "user.email", "notes@example.invalid")
        self.baseline = self.commit("Initial tree")
        self.catchup = root / "catchup.json"
        self.manifest()

    def git(self, *args):
        return subprocess.run(["git", *args], cwd=self.root, check=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout.decode().strip()

    def commit(self, message):
        subprocess.run(["git", "commit", "--allow-empty", "-F", "-"], cwd=self.root,
                       input=message.encode("utf-8"), check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return self.git("rev-parse", "HEAD")

    def manifest(self, notes=None, baseline=None):
        self.catchup.write_text(json.dumps({"schema_version": 1,
                                          "baseline": baseline or self.baseline,
                                          "notes": notes or []}), encoding="utf-8")

    def collect(self, **kwargs):
        return player_notes.collect(self.root, catchup=self.catchup, **kwargs)


@pytest.fixture
def repo(tmp_path):
    return Repository(tmp_path / "repo")


def texts(data):
    return [note for entry in data["entries"] for note in entry["notes"]]


def test_body_annotations_and_explicit_none(repo):
    first = repo.commit("Internal subject\n\nPlayer-Note: Added controller navigation.\nPlayer-Note: Fixed map loading.")
    repo.commit("No user facing effect\n\nPlayer-Note: none")
    repo.commit("A very exciting but unannotated subject")
    data, markdown, warnings = repo.collect()
    assert data["schema_version"] == 2
    assert data["head"] == repo.git("rev-parse", "HEAD")
    assert data["entries"][0]["commit"] == data["head"]
    assert data["entries"][0]["build"] is None
    assert texts(data) == ["Added controller navigation.", "Fixed map loading."]
    assert markdown == "- Added controller navigation.\n- Fixed map loading.\n"
    assert warnings == []


def test_empty_history_and_no_subject_fallback(repo):
    repo.commit("Player-visible sounding subject with no annotation")
    data, markdown, _ = repo.collect()
    assert data["entries"][0]["notes"] == []
    assert markdown == "No player-facing notes were recorded for this build.\n"
    assert repo.collect(head=repo.baseline)[0]["entries"][0]["notes"] == []


def test_prose_mentioning_annotation_syntax_is_not_an_annotation(repo):
    repo.commit("Document Player-Note usage\n\nPlayer-Note approach is authored, not inferred.\n"
                "Write Player-Note: short text in the body.\n"
                "    Player-Note: an indented documentation example.")
    assert repo.collect()[0]["entries"][0]["notes"] == []


def test_release_requires_notes_for_unannotated_upstream_merge(repo):
    repo.git("checkout", "-b", "upstream")
    repo.commit("Add an upstream feature without our annotation convention")
    repo.git("checkout", "main")
    repo.commit("Maintenance\n\nPlayer-Note: none")
    repo.git("merge", "--no-ff", "upstream", "-m", "Merge upstream")
    with pytest.raises(player_notes.NotesError, match="new commits have no release notes"):
        repo.collect(base=repo.baseline, build_number=2, require_notes=True)
    merge = repo.git("rev-parse", "HEAD")
    repo.manifest([{"commit": merge, "text": "Added local Slayer bots."}])
    assert texts(repo.collect(base=repo.baseline, build_number=2,
                              require_notes=True)[0]) == ["Added local Slayer bots."]


def test_release_tip_can_explicitly_review_internal_only_range(repo):
    repo.commit("Internal refactor without an annotation")
    repo.commit("Release internal maintenance\n\nPlayer-Note: none")
    assert texts(repo.collect(build_number=2, require_notes=True)[0]) == []


def test_empty_release_range_and_explicit_omissions_are_allowed(repo):
    tip = repo.commit("Maintenance\n\nPlayer-Note: none")
    assert texts(repo.collect(base=tip, build_number=2, require_notes=True)[0]) == []
    assert texts(repo.collect(base=repo.baseline, build_number=2,
                              require_notes=True)[0]) == []


def test_old_duplicate_note_cannot_satisfy_release_check(repo):
    base = repo.commit("Old feature\n\nPlayer-Note: Added bots.")
    repo.commit("Unreviewed integration\n\nPlayer-Note: Added bots.")
    with pytest.raises(player_notes.NotesError, match="new commits have no release notes"):
        repo.collect(base=base, build_number=2, require_notes=True)


def test_all_merged_commits_group_once_and_keep_chronological_order(repo):
    repo.git("checkout", "-b", "upstream")
    upstream = repo.commit("Upstream work\n\nPlayer-Note: Improved upstream controls.")
    repo.git("checkout", "-b", "testing")
    testing = repo.commit("Tested work\n\nPlayer-Note: Added tested maps.")
    repo.git("checkout", "main")
    old = repo.commit("Local work\n\nPlayer-Note: Improved local controls.")
    repo.git("merge", "--no-ff", "testing", "-m", "Merge testing\n\nPlayer-Note: Improved integration.")
    merge = repo.git("rev-parse", "HEAD")
    repo.git("merge", "upstream")
    data, markdown, _ = repo.collect()
    assert [entry["commit"] for entry in data["entries"]] == [merge]
    assert data["entries"][0]["notes"] == ["Improved local controls.", "Improved upstream controls.", "Added tested maps.", "Improved integration."]
    assert markdown.count("Improved upstream controls.") == 1
    assert upstream not in [entry["commit"] for entry in data["entries"]]
    assert testing not in [entry["commit"] for entry in data["entries"]]


def test_duplicate_notes_belong_to_earliest_introduction(repo):
    first = repo.commit("First\n\nPlayer-Note: Improved  controls.")
    repo.commit("Second\n\nPlayer-Note: improved controls\nPlayer-Note: Added maps.")
    data, markdown, _ = repo.collect(base=first)
    assert texts(data) == ["Added maps."]
    assert markdown == "- Added maps.\n"


def test_later_merge_does_not_repeat_already_introduced_ancestry(repo):
    repo.git("checkout", "-b", "upstream")
    repo.commit("Upstream\n\nPlayer-Note: Improved upstream controls.")
    repo.git("checkout", "-b", "testing")
    repo.commit("Testing\n\nPlayer-Note: Added tested maps.")
    repo.git("checkout", "main")
    repo.git("merge", "--no-ff", "upstream", "-m", "Merge upstream")
    previous_release = repo.git("rev-parse", "HEAD")
    repo.git("merge", "--no-ff", "testing", "-m", "Merge testing")
    data, markdown, _ = repo.collect(base=previous_release)
    assert texts(data) == ["Added tested maps."]
    assert markdown == "- Added tested maps.\n"


def test_release_range_includes_prior_commits_not_only_head(repo):
    base = repo.commit("Old\n\nPlayer-Note: Old controls.")
    repo.commit("First\n\nPlayer-Note: New controls.")
    repo.commit("Second\n\nPlayer-Note: New maps.")
    repo.commit("Packaging only")
    _, markdown, _ = repo.collect(base=base)
    assert markdown == "- New controls.\n- New maps.\n"


def test_catchup_only_applies_to_included_commits_and_keeps_order(repo):
    first = repo.commit("Unannotated historic change")
    repo.manifest([{"commit": repo.baseline, "text": "Excluded baseline note."},
                   {"commit": first, "text": "First catch-up highlight."},
                   {"commit": first, "text": "Second catch-up highlight."}])
    assert texts(repo.collect()[0]) == ["First catch-up highlight.", "Second catch-up highlight."]
    assert repo.collect(head=repo.baseline)[0]["entries"][0]["notes"] == []


def test_explicit_none_omits_catchup_for_that_commit(repo):
    commit = repo.commit("Omit\n\nPlayer-Note: none")
    repo.manifest([{"commit": commit, "text": "Old catch-up highlight."}])
    assert texts(repo.collect()[0]) == []


def test_preview_combines_history_without_mislabeling_as_release(repo):
    repo.commit("First\n\nPlayer-Note: First highlight.")
    head = repo.commit("Second\n\nPlayer-Note: Second highlight.")
    data, _, _ = repo.collect(preview=True)
    assert len(data["entries"]) == 1
    assert data["entries"][0]["build"] is None
    assert data["entries"][0]["commit"] == head
    assert texts(data) == ["First highlight.", "Second highlight."]


def test_release_number_groups_all_changes_since_previous_success(repo):
    for index in range(10):
        repo.commit(f"Change {index}\n\nPlayer-Note: Highlight {index}.")
    data, markdown, warnings = repo.collect(build_number=431)
    assert len(data["entries"]) == 1
    assert data["entries"][0]["build"] == 431
    assert texts(data) == [f"Highlight {index}." for index in range(10)]
    assert len(markdown.splitlines()) == 10
    assert warnings == []


def test_no_per_entry_truncation_and_excessive_total_fails(repo, monkeypatch):
    repo.commit("Many\n\n" + "\n".join(f"Player-Note: Highlight {i}." for i in range(30)))
    assert len(texts(repo.collect()[0])) == 30
    monkeypatch.setattr(player_notes, "MAX_NOTES", 29)
    with pytest.raises(player_notes.NotesError, match="none were silently discarded"):
        repo.collect()


@pytest.mark.parametrize("annotation", [
    "Player-Note:", "Player-Note: ", "Player-Note: " + "x" * 101,
    "Player-Note: New | menus", "Player-Note: New\\nmenus",
    "Player-Note: New\tmenus", "Player-Note: New\rmenus",
    "Player-Note: New\x1bmenus", "Player-Note: Caf\u00e9 controls.",
    "player-note: New menus.",
    "Player-Note: none\nPlayer-Note: Actual change.",
])
def test_invalid_annotations_fail_actionably(repo, annotation):
    sha = repo.commit("Change\n\n" + annotation)
    with pytest.raises(player_notes.NotesError, match=sha):
        repo.collect()


@pytest.mark.parametrize("option", ["head", "base"])
@pytest.mark.parametrize("value", ["--all", "HEAD~1", "-h", "abc", "HEAD; echo unsafe"])
def test_revision_options_are_full_shas_not_options_or_expressions(repo, option, value):
    with pytest.raises(player_notes.NotesError, match="must be a full"):
        repo.collect(**{option: value})


def test_base_must_be_ancestor(repo):
    repo.git("checkout", "-b", "side")
    unrelated = repo.commit("Side")
    repo.git("checkout", "main")
    repo.commit("Main")
    with pytest.raises(player_notes.NotesError, match="base .*not an ancestor"):
        repo.collect(base=unrelated)


def test_missing_baseline_and_nonancestor_baseline_fail(repo):
    repo.manifest(baseline="1" * 40)
    with pytest.raises(player_notes.NotesError, match="catch-up baseline .*fetch complete history"):
        repo.collect()
    repo.git("checkout", "-b", "side")
    unrelated = repo.commit("Side")
    repo.git("checkout", "main")
    repo.commit("Main")
    repo.manifest(baseline=unrelated)
    with pytest.raises(player_notes.NotesError, match="baseline .*not an ancestor"):
        repo.collect()


@pytest.mark.parametrize("entry", [
    {"commit": "2" * 40, "text": "Missing commit."},
    {"commit": "--all", "text": "Hostile option."},
    {"commit": None, "text": "Invalid SHA."},
])
def test_catchup_references_must_exist_even_when_not_included(repo, entry):
    repo.manifest([entry])
    with pytest.raises(player_notes.NotesError):
        repo.collect()


@pytest.mark.parametrize("text", [None, "", "none", "bad|menu", "bad\\nmenu", "bad\u00a0menu", "x" * 101])
def test_catchup_text_is_validated(repo, text):
    repo.manifest([{"commit": repo.baseline, "text": text}])
    with pytest.raises(player_notes.NotesError, match="catch-up note 1"):
        repo.collect()


def test_missing_and_malformed_manifest(repo):
    repo.catchup.unlink()
    with pytest.raises(player_notes.NotesError, match="cannot read catch-up manifest"):
        repo.collect()
    repo.catchup.write_text('{"schema_version": 2}')
    with pytest.raises(player_notes.NotesError, match="schema_version 1"):
        repo.collect()


def test_shallow_history_requires_complete_checkout(repo):
    head = repo.commit("Change")
    # Git's real shallow boundary avoids spawning a transport shell on Windows.
    (repo.root / ".git/shallow").write_text(head + "\n", encoding="ascii")
    with pytest.raises(player_notes.NotesError, match="fetch-depth: 0"):
        repo.collect()


def test_output_is_deterministic_atomic_and_cli_reports_older_history(repo, tmp_path, monkeypatch, capsys):
    repo.commit("Change\n\nPlayer-Note: Actual highlight.")
    output = tmp_path / "output"
    monkeypatch.setattr(player_notes, "ROOT", repo.root)
    args = ["--catchup", str(repo.catchup), "--output", str(output)]
    assert player_notes.main(args) == 0
    first = {path.name: path.read_bytes() for path in output.iterdir()}
    assert player_notes.main(args) == 0
    assert {path.name: path.read_bytes() for path in output.iterdir()} == first
    assert sorted(first) == ["changelog.json", "notes.md"]
    monkeypatch.setattr(player_notes, "MAX_ENTRIES", 0)
    assert player_notes.main(args) == 0
    assert "omitted 1 older changelog entries" in capsys.readouterr().err


def archive(repo, tmp_path, data):
    path = tmp_path / "previous.json"
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


def test_published_archive_preserves_exact_builds_and_release_boundaries(repo, tmp_path):
    first = repo.commit("First\n\nPlayer-Note: First released highlight.")
    first_data, _, _ = repo.collect(build_number=431)
    previous = archive(repo, tmp_path, first_data)
    repo.commit("Failed build\n\nPlayer-Note: Highlight from a failed build.")
    head = repo.commit("Successful build\n\nPlayer-Note: Highlight from a successful build.")
    data, markdown, _ = repo.collect(build_number=433, base=first, previous=previous)
    assert [entry["build"] for entry in data["entries"]] == [433, 431]
    assert data["entries"][1] == first_data["entries"][0]
    assert data["entries"][0]["commit"] == head
    assert markdown == "- Highlight from a failed build.\n- Highlight from a successful build.\n"


def test_git_releases_carry_exact_build_numbers_into_menu_pages(repo, tmp_path):
    import changelog_menu

    base = repo.commit("Published change\n\nPlayer-Note: First released highlight.")
    published, markdown, _ = repo.collect(build_number=12)
    output = tmp_path / "published"
    player_notes.write_outputs(output, published, markdown)
    repo.commit("Next change\n\nPlayer-Note: [INTERFACE] Next released highlight.\n"
                "Player-Note: [QUALITY OF LIFE] Added shortcuts.\n"
                "Player-Note: [INTERFACE] Clearer labels.")
    current, markdown, _ = repo.collect(build_number=13, base=base,
                                       previous=output / "changelog.json")
    menu = ET.fromstring("<menus>" + "\n".join(changelog_menu.widget_lines(current)) + "</menus>")
    roots = [widget for widget in menu.findall("widget")
             if "no_history" in widget.get("flags", "").split()]
    assert len(roots) == 2
    headings = [widget.get("text") for widget in menu.findall("widget")
                if widget.get("name", "").endswith("/heading")]
    assert headings == ["BUILD 13", "BUILD 12"]
    prefix = changelog_menu.BASE + "/build_1"
    heading_table = menu.find(f"strings[@name='{prefix}/headings']")
    body_table = menu.find(f"strings[@name='{prefix}/lines']")
    assert [node.get("text") for node in heading_table if node.get("text")] == ["INTERFACE", "QUALITY OF LIFE"]
    assert [node.get("text") for node in body_table if node.get("text")] == [
        "- Next released highlight.", "- Clearer labels.", "- Added shortcuts."]
    assert len(heading_table) == len(body_table)
    blue_rows = [widget for widget in menu.findall("widget")
                 if widget.get("name", "").startswith(prefix + "/heading_row_")]
    assert blue_rows and all(widget.get("color") == "#FF2896FF" for widget in blue_rows)
    old_heading_table = menu.find(f"strings[@name='{changelog_menu.BASE}/build_2/headings']")
    assert [node.get("text") for node in old_heading_table if node.get("text")] == ["GENERAL"]
    assert markdown == "- Next released highlight.\n- Added shortcuts.\n- Clearer labels.\n"


def test_no_change_release_and_same_sha_new_build_have_pages(repo, tmp_path):
    head = repo.commit("Internal maintenance")
    first_data, _, _ = repo.collect(build_number=5)
    previous = archive(repo, tmp_path, first_data)
    data, markdown, _ = repo.collect(build_number=6, base=head, previous=previous)
    assert [entry["build"] for entry in data["entries"]] == [6, 5]
    assert all(entry["commit"] == head and entry["notes"] == [] for entry in data["entries"])
    assert markdown == "No player-facing notes were recorded for this build.\n"


def test_rollout_without_archive_does_not_invent_prior_build_numbers(repo):
    base = repo.commit("Old change\n\nPlayer-Note: Old highlight.")
    repo.commit("New change\n\nPlayer-Note: Current highlight.")
    data, _, _ = repo.collect(build_number=72, base=base)
    assert len(data["entries"]) == 1
    assert data["entries"][0]["build"] == 72
    assert texts(data) == ["Current highlight."]


def test_preview_can_show_published_history_and_respects_base(repo, tmp_path):
    base = repo.commit("Old\n\nPlayer-Note: Published highlight.")
    previous = archive(repo, tmp_path, repo.collect(build_number=8)[0])
    repo.commit("New\n\nPlayer-Note: Preview highlight.")
    data, markdown, _ = repo.collect(preview=True, base=base, previous=previous)
    assert [entry["build"] for entry in data["entries"]] == [None, 8]
    assert data["entries"][0]["notes"] == ["Preview highlight."]
    assert markdown == "- Preview highlight.\n"


def test_history_window_drops_only_older_published_builds(repo, tmp_path):
    previous = None
    base = None
    for build in range(1, 10):
        repo.commit(f"Release {build}\n\nPlayer-Note: Highlight {build}.")
        data, _, warnings = repo.collect(build_number=build, base=base, previous=previous)
        base = data["head"]
        previous = archive(repo, tmp_path, data)
    assert [entry["build"] for entry in data["entries"]] == list(range(9, 1, -1))
    assert warnings == ["omitted 1 older changelog entries (keeping newest 8)"]


@pytest.mark.parametrize("build", [0, -1, True, "3", 1.5])
def test_invalid_current_build_number_fails(repo, build):
    with pytest.raises(player_notes.NotesError, match="positive integer"):
        repo.collect(build_number=build)


def test_preview_and_numbered_release_are_mutually_exclusive(repo):
    with pytest.raises(player_notes.NotesError, match="cannot accompany"):
        repo.collect(build_number=3, preview=True)


def test_explicit_categories_keep_note_text_and_uncategorized_compatibility(repo):
    repo.commit("Mixed notes\n\nPlayer-Note: [INTERFACE] Improved menus.\n"
                "Player-Note: A plain older-style note.\n"
                "Player-Note: [QUALITY OF LIFE] Added controller shortcuts.")
    data, markdown, _ = repo.collect(build_number=12)
    entry = data["entries"][0]
    assert entry["categories"] == ["INTERFACE", "", "QUALITY OF LIFE"]
    assert entry["notes"] == ["Improved menus.", "A plain older-style note.", "Added controller shortcuts."]
    assert markdown == "- Improved menus.\n- A plain older-style note.\n- Added controller shortcuts.\n"


def test_category_prefix_does_not_reduce_note_text_limit(repo):
    repo.commit("Note\n\nPlayer-Note: [QUALITY OF LIFE] " + "x" * 100)
    assert len(texts(repo.collect()[0])[0]) == 100


@pytest.mark.parametrize("annotation", [
    "Player-Note: [interface] Improved menus.", "Player-Note: [] Improved menus.",
    "Player-Note: [TOO  MANY SPACES] Improved menus.", "Player-Note: [BAD|MENU] Improved menus.",
    "Player-Note: [BAD\\MENU] Improved menus.", "Player-Note: [A" + "B" * 24 + "] Improved menus.",
    "Player-Note: [INTERFACE]", "Player-Note: [INTERFACE]none",
    "Player-Note: [INTERFACE] none", "Player-Note: [INTERFACE] " + "x" * 101,
])
def test_invalid_explicit_category_fails_actionably(repo, annotation):
    sha = repo.commit("Invalid category\n\n" + annotation)
    with pytest.raises(player_notes.NotesError, match=sha):
        repo.collect()


def test_catchup_categories_are_explicit_and_never_inferred(repo):
    commit = repo.commit("Old change")
    repo.manifest([{"commit": commit, "text": "Improved menus.", "category": "INTERFACE"},
                   {"commit": commit, "text": "Fixed multiplayer sounds."}])
    entry = repo.collect()[0]["entries"][0]
    assert entry["categories"] == ["INTERFACE", ""]


@pytest.mark.parametrize("category", [None, "interface", "BAD|MENU", "BAD\\MENU", "BAD\nMENU", "X" * 25])
def test_unsafe_catchup_category_fails(repo, category):
    repo.manifest([{"commit": repo.baseline, "text": "Improved menus.", "category": category}])
    with pytest.raises(player_notes.NotesError, match="category must"):
        repo.collect()


def test_categories_survive_archives_without_changing_old_uncategorized_entries(repo, tmp_path):
    base = repo.commit("First\n\nPlayer-Note: Old highlight.")
    old = repo.collect(build_number=12)[0]
    previous = archive(repo, tmp_path, old)
    head = repo.commit("Second\n\nPlayer-Note: [GRAPHICS] Improved lights.")
    new, _, _ = repo.collect(build_number=13, base=base, previous=previous)
    assert new["entries"][1] == old["entries"][0]
    assert "categories" not in new["entries"][1]
    previous = archive(repo, tmp_path, new)
    newest, _, _ = repo.collect(build_number=14, base=head, previous=previous)
    assert newest["entries"][1] == new["entries"][0]
    assert newest["entries"][1]["categories"] == ["GRAPHICS"]


@pytest.mark.parametrize("categories", [None, "INTERFACE", [], ["INTERFACE", "GRAPHICS"],
                                         [None], ["lowercase"], ["BAD|MENU"], ["X" * 25]])
def test_prior_archive_category_metadata_is_validated(repo, tmp_path, categories):
    base = repo.commit("First\n\nPlayer-Note: Old highlight.")
    data = repo.collect(build_number=12)[0]
    data["entries"][0]["categories"] = categories
    with pytest.raises(player_notes.NotesError, match="categor"):
        repo.collect(build_number=13, base=base, previous=archive(repo, tmp_path, data))


def test_duplicate_text_keeps_earliest_category(repo):
    base = repo.commit("First\n\nPlayer-Note: [QUALITY OF LIFE] Added shortcuts.")
    repo.commit("Second\n\nPlayer-Note: [INTERFACE] Added shortcuts.")
    assert repo.collect()[0]["entries"][0]["categories"] == ["QUALITY OF LIFE"]
    current = repo.collect(base=base)[0]["entries"][0]
    assert current["notes"] == []
    assert "categories" not in current


def test_current_build_rejects_signed_integer_overflow(repo):
    assert repo.collect(build_number=2147483647)[0]["entries"][0]["build"] == 2147483647
    with pytest.raises(player_notes.NotesError, match="at most 2147483647"):
        repo.collect(build_number=2147483648)


def test_prior_build_rejects_signed_integer_overflow(repo, tmp_path):
    base = repo.commit("Published build")
    data = repo.collect(build_number=2147483647)[0]
    previous = archive(repo, tmp_path, data)
    assert repo.collect(base=base, previous=previous)[0]["entries"][1]["build"] == 2147483647
    data["entries"][0]["build"] = 2147483648
    with pytest.raises(player_notes.NotesError, match="at most 2147483647"):
        repo.collect(base=base, previous=archive(repo, tmp_path, data))


@pytest.mark.parametrize("field,value,match", [
    ("build", None, "positive integer"), ("build", 0, "positive integer"),
    ("build", True, "positive integer"), ("build", 3, "must decrease"),
    ("date", "2026-02-30", "YYYY-MM-DD"), ("date", "20261018", "YYYY-MM-DD"),
    ("date", None, "YYYY-MM-DD"), ("commit", "--all", "full 40-character"),
    ("notes", ["bad|menu"], "printable ASCII"), ("notes", "text", "must be a list"),
])
def test_prior_archive_entries_are_validated(repo, tmp_path, field, value, match):
    base = repo.commit("Old\n\nPlayer-Note: Published highlight.")
    data = repo.collect(build_number=2)[0]
    data["entries"][0][field] = value
    with pytest.raises(player_notes.NotesError, match=match):
        repo.collect(build_number=3, base=base, previous=archive(repo, tmp_path, data))


def test_archive_head_and_newest_entry_match_base(repo, tmp_path):
    first = repo.commit("First")
    data = repo.collect(build_number=1)[0]
    second = repo.commit("Second")
    with pytest.raises(player_notes.NotesError, match="head must equal --base"):
        repo.collect(build_number=2, base=second, previous=archive(repo, tmp_path, data))
    data["head"] = second
    with pytest.raises(player_notes.NotesError, match="newest entry commit must equal"):
        repo.collect(build_number=2, base=second, previous=archive(repo, tmp_path, data))
    assert first != second


def test_archive_requires_base_and_unique_builds(repo, tmp_path):
    base = repo.commit("First")
    data = repo.collect(build_number=1)[0]
    previous = archive(repo, tmp_path, data)
    with pytest.raises(player_notes.NotesError, match="requires --base"):
        repo.collect(build_number=2, previous=previous)
    data["entries"] *= 2
    with pytest.raises(player_notes.NotesError, match="unique positive integer"):
        repo.collect(build_number=2, base=base, previous=archive(repo, tmp_path, data))


def test_archive_ancestry_and_schema_cannot_be_fabricated(repo, tmp_path):
    repo.git("checkout", "-b", "side")
    side = repo.commit("Side")
    repo.git("checkout", "main")
    base = repo.commit("Main")
    data = repo.collect(build_number=2)[0]
    data["entries"].append(dict(data["entries"][0], build=1, commit=side))
    with pytest.raises(player_notes.NotesError, match="not an ancestor"):
        repo.collect(build_number=3, base=base, previous=archive(repo, tmp_path, data))
    data["schema_version"] = 1
    with pytest.raises(player_notes.NotesError, match="expected schema_version 2"):
        repo.collect(build_number=3, base=base, previous=archive(repo, tmp_path, data))


def test_supplied_missing_archive_fails_instead_of_inventing_history(repo, tmp_path):
    with pytest.raises(player_notes.NotesError, match="cannot read previous changelog archive"):
        repo.collect(build_number=3, base=repo.baseline, previous=tmp_path / "missing.json")


def test_validation_failure_does_not_replace_prior_outputs(repo, tmp_path, monkeypatch):
    output = tmp_path / "output"
    output.mkdir()
    original = output / "changelog.json"
    original.write_text("preserved")
    repo.commit("Invalid\n\nPlayer-Note:")
    monkeypatch.setattr(player_notes, "ROOT", repo.root)
    with pytest.raises(SystemExit) as exc:
        player_notes.main(["--catchup", str(repo.catchup), "--output", str(output)])
    assert exc.value.code == 1
    assert original.read_text() == "preserved"
    assert not (output / "notes.md").exists()
