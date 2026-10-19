#!/usr/bin/env python3
"""Collect authored Player-Note lines from Git, without subject fallbacks."""

import argparse
from dataclasses import dataclass
from datetime import date
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MAX_ENTRIES = 8
MAX_NOTES = 4096
MAX_BUILD_NUMBER = 2147483647
SHA = re.compile(r"[0-9a-fA-F]{40}\Z")
CATEGORY = re.compile(r"[A-Z][A-Z0-9]*(?: [A-Z0-9]+)*\Z")


class NotesError(ValueError):
    """An invalid annotation, manifest or Git range."""


@dataclass
class Commit:
    sha: str
    parents: list
    notes: list
    omitted: bool


def git(repo, *args):
    result = subprocess.run(
        ["git", "--no-pager", "--no-replace-objects", *args], cwd=repo,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False,
    )
    if result.returncode:
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise NotesError(f"git {args[0]} failed: {detail}")
    return result.stdout.decode("utf-8", errors="replace")


def resolve_commit(repo, value, label, allow_head=False):
    if not isinstance(value, str) or not (SHA.fullmatch(value) or (allow_head and value == "HEAD")):
        raise NotesError(f"{label} must be a full 40-character commit SHA" +
                         (" or HEAD" if allow_head else ""))
    try:
        sha = git(repo, "rev-parse", "--verify", "--end-of-options", value).strip()
        object_type = git(repo, "cat-file", "-t", sha).strip()
    except NotesError as exc:
        raise NotesError(f"{label} {value} is unavailable; fetch complete history or correct the SHA: {exc}") from exc
    if object_type != "commit":
        raise NotesError(f"{label} {value} does not name a commit")
    return sha


def require_ancestor(repo, ancestor, head, label):
    # The count also distinguishes an unavailable object from a non-ancestor.
    if git(repo, "rev-list", "--count", ancestor, "^" + head, "--").strip() != "0":
        raise NotesError(f"{label} {ancestor} is not an ancestor of head {head}")


def validate_note(text, context):
    if not isinstance(text, str) or not 1 <= len(text) <= 100:
        raise NotesError(f"{context}: note must contain 1 to 100 printable ASCII characters")
    if any(ord(c) < 32 or ord(c) > 126 or c in "\\|" for c in text):
        raise NotesError(f"{context}: note must be printable ASCII without backslashes or menu pipes")
    text = text.strip(" ")
    if not text:
        raise NotesError(f"{context}: note is empty; use Player-Note: none to omit a commit")
    return text


def validate_category(category, context, allow_empty=True):
    if allow_empty and category == "":
        return category
    if not isinstance(category, str) or not 1 <= len(category) <= 24 or not CATEGORY.fullmatch(category):
        raise NotesError(f"{context}: category must be 1 to 24 uppercase ASCII letters, digits and single spaces, starting with a letter")
    return category


def validate_categories(categories, notes, context):
    if not isinstance(categories, list) or len(categories) != len(notes):
        raise NotesError(f"{context}: categories must be a list with exactly one label per note")
    return [validate_category(category, context) for category in categories]


def parse_notes(message, context):
    notes = []
    omitted = False
    # Split only at LF: splitlines() would silently consume hostile controls.
    for line in message.split("\n"):
        if not line.lower().startswith("player-note:"):
            continue
        if not line.startswith("Player-Note:"):
            raise NotesError(f"{context}: malformed annotation; use Player-Note: short sentence")
        text = line[len("Player-Note:"):]
        text = text.strip(" ")
        category = ""
        if text.startswith("["):
            match = re.fullmatch(r"\[([^\]]*)\] (.*)", text)
            if match is None:
                raise NotesError(f"{context}: use Player-Note: [CATEGORY] short sentence for categorized notes")
            category = validate_category(match[1], context, allow_empty=False)
            text = match[2]
        text = validate_note(text, context)
        if text.casefold() == "none":
            if category:
                raise NotesError(f"{context}: Player-Note: none cannot have a category")
            omitted = True
        else:
            notes.append((text, category))
    if omitted and notes:
        raise NotesError(f"{context}: Player-Note: none cannot accompany player notes")
    return notes, omitted


def note_key(text):
    return " ".join(text.casefold().split()).removesuffix(".").rstrip()


def load_catchup(repo, path, head):
    try:
        data = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise NotesError(f"cannot read catch-up manifest {path}: {exc}") from exc
    if not isinstance(data, dict) or type(data.get("schema_version")) is not int or data["schema_version"] != 1:
        raise NotesError(f"{path}: expected catch-up schema_version 1")
    baseline = resolve_commit(repo, data.get("baseline", ""), "catch-up baseline")
    require_ancestor(repo, baseline, head, "catch-up baseline")
    if not isinstance(data.get("notes"), list):
        raise NotesError(f"{path}: notes must be a list")
    notes = {}
    for index, entry in enumerate(data["notes"], 1):
        context = f"{path}: catch-up note {index}"
        if not isinstance(entry, dict):
            raise NotesError(f"{context}: expected commit and text")
        sha = resolve_commit(repo, entry.get("commit", ""), context + " commit")
        text = validate_note(entry.get("text"), context)
        if text.casefold() == "none":
            raise NotesError(f"{context}: omit an unwanted catch-up entry instead of writing none")
        category = validate_category(entry.get("category", ""), context)
        notes.setdefault(sha, []).append((text, category))
    return baseline, notes


def read_commits(repo, baseline, head):
    raw = git(repo, "log", "--reverse", "--topo-order", "-z",
              "--format=%H%x00%P%x00%B", f"{baseline}..{head}", "--")
    fields = raw.split("\0")
    if fields[-1] == "":
        fields.pop()
    if len(fields) % 3:
        raise NotesError("Git returned an invalid commit record")
    commits = {}
    for offset in range(0, len(fields), 3):
        sha, parents, message = fields[offset:offset + 3]
        notes, omitted = parse_notes(message, f"commit {sha}")
        commits[sha] = Commit(sha, parents.split(), notes, omitted)
    return commits


def ordered_commits(commits, head):
    """Order by first-parent introduction, then Git's topological order."""
    chain = []
    sha = head
    while sha in commits:
        chain.append(sha)
        parents = commits[sha].parents
        sha = parents[0] if parents else None
    introduced = {}
    for group, sha in enumerate(reversed(chain)):
        pending = [sha]
        while pending:
            current = pending.pop()
            if current not in commits or current in introduced:
                continue
            introduced[current] = group
            pending.extend(commits[current].parents)
    # Stable sorting retains Git's order within each group, without a
    # complete history scan for every first-parent commit.
    return sorted(commits.values(), key=lambda commit: introduced[commit.sha])


def load_previous(repo, path, base, build_number):
    if base is None:
        raise NotesError("--previous requires --base to identify the published archive's commit")
    try:
        data = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise NotesError(f"cannot read previous changelog archive {path}: {exc}") from exc
    if not isinstance(data, dict) or type(data.get("schema_version")) is not int or data["schema_version"] != 2:
        raise NotesError(f"{path}: unsupported previous archive; expected schema_version 2")
    archive_head = resolve_commit(repo, data.get("head", ""), "previous archive head")
    if archive_head != base:
        raise NotesError(f"{path}: previous archive head must equal --base {base}")
    entries = data.get("entries")
    if not isinstance(entries, list) or not 1 <= len(entries) <= MAX_ENTRIES:
        raise NotesError(f"{path}: previous archive must contain 1 to {MAX_ENTRIES} entries")
    seen_builds = set()
    validated = []
    last_build = build_number
    for index, entry in enumerate(entries, 1):
        context = f"{path}: previous entry {index}"
        if not isinstance(entry, dict):
            raise NotesError(f"{context}: expected build, date, commit and notes")
        build = entry.get("build")
        if type(build) is not int or not 1 <= build <= MAX_BUILD_NUMBER or build in seen_builds:
            raise NotesError(f"{context}: build must be a unique positive integer at most {MAX_BUILD_NUMBER}")
        if last_build is not None and build >= last_build:
            raise NotesError(f"{context}: build numbers must decrease from the current build through older entries")
        seen_builds.add(build)
        last_build = build
        entry_date = entry.get("date")
        try:
            if not isinstance(entry_date, str) or date.fromisoformat(entry_date).isoformat() != entry_date:
                raise ValueError()
        except ValueError as exc:
            raise NotesError(f"{context}: date must be a valid YYYY-MM-DD date") from exc
        commit = resolve_commit(repo, entry.get("commit", ""), context + " commit")
        require_ancestor(repo, commit, archive_head, context + " commit")
        if index == 1 and commit != archive_head:
            raise NotesError(f"{context}: newest entry commit must equal the archive head")
        if not isinstance(entry.get("notes"), list):
            raise NotesError(f"{context}: notes must be a list")
        notes = [validate_note(note, context) for note in entry["notes"]]
        saved = {"build": build, "date": entry_date, "commit": commit, "notes": notes}
        if "categories" in entry:
            saved["categories"] = validate_categories(entry["categories"], notes, context)
        validated.append(saved)
    if sum(len(entry["notes"]) for entry in validated) > MAX_NOTES:
        raise NotesError(f"{path}: previous archive exceeds {MAX_NOTES} notes")
    return validated


def collect(repo, head="HEAD", base=None, catchup=None, preview=False,
            build_number=None, previous=None, require_notes=False):
    if build_number is not None and (type(build_number) is not int or not 1 <= build_number <= MAX_BUILD_NUMBER):
        raise NotesError(f"build number must be a positive integer at most {MAX_BUILD_NUMBER}")
    if preview and build_number is not None:
        raise NotesError("--preview cannot accompany --build-number")
    repo = Path(repo)
    if git(repo, "rev-parse", "--is-shallow-repository").strip() != "false":
        raise NotesError("player notes require complete Git history; use checkout fetch-depth: 0 or git fetch --unshallow")
    head = resolve_commit(repo, head, "head", allow_head=True)
    baseline, catchup_notes = load_catchup(
        repo, catchup or repo / "port/assets/changelog/catchup.json", head)
    if base is not None:
        base = resolve_commit(repo, base, "base")
        require_ancestor(repo, base, head, "base")
    previous_entries = load_previous(repo, previous, base, build_number) if previous is not None else []
    commits = read_commits(repo, baseline, head)
    release_range = set(git(repo, "rev-list", f"{base or baseline}..{head}", "--").split())
    seen = set()
    release_highlights = []
    # Deduplicate across full authored history, including already released notes.
    for commit in ordered_commits(commits, head):
        if commit.omitted:
            continue
        for note, category in catchup_notes.get(commit.sha, []) + commit.notes:
            key = note_key(note)
            if key not in seen:
                seen.add(key)
                if commit.sha in release_range:
                    release_highlights.append((note, category))
    release_notes = [note for note, _ in release_highlights]
    if require_notes and release_range and not release_notes:
        # A release-tip omission is an explicit review of the integrated range.
        # An omission on an unrelated maintenance commit cannot cover a merge.
        reviewed = head in commits and commits[head].omitted
        unreviewed = [sha for sha in release_range
                      if sha not in commits or not commits[sha].omitted]
        if unreviewed and not reviewed:
            raise NotesError(
                "new commits have no release notes; add Player-Note: highlights "
                "to the integration commit or reviewed catch-up notes. If the "
                "entire release has no player-facing changes, explicitly mark "
                "its tip Player-Note: none after review")
    commit_date = git(repo, "show", "-s", "--format=%cs", head, "--").strip()
    entries = [{"build": build_number, "date": commit_date, "commit": head, "notes": release_notes}]
    categories = [category for _, category in release_highlights]
    if any(categories):
        entries[0]["categories"] = categories
    entries.extend(previous_entries)
    warnings = []
    if len(entries) > MAX_ENTRIES:
        warnings.append(f"omitted {len(entries) - MAX_ENTRIES} older changelog entries (keeping newest {MAX_ENTRIES})")
    entries = entries[:MAX_ENTRIES]
    if sum(len(entry["notes"]) for entry in entries) > MAX_NOTES:
        raise NotesError(f"changelog exceeds {MAX_NOTES} notes; shorten or consolidate authored notes (none were silently discarded)")
    markdown = "\n".join("- " + note for note in release_notes) + "\n" if release_notes else "No player-facing notes were recorded for this build.\n"
    return {"schema_version": 2, "head": head, "entries": entries}, markdown, warnings


def write_outputs(output, data, markdown):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    staged = []
    try:
        for name, text in (("changelog.json", json.dumps(data, indent=2) + "\n"),
                           ("notes.md", markdown)):
            with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n",
                                             dir=output, prefix=".player-notes-", delete=False) as file:
                staged.append((Path(file.name), output / name))
                file.write(text)
        for temporary, destination in staged:
            os.replace(temporary, destination)
    finally:
        for temporary, _ in staged:
            temporary.unlink(missing_ok=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--head", default="HEAD", help="full commit SHA, or HEAD (default)")
    parser.add_argument("--base", help="full SHA of the previous successful release")
    parser.add_argument("--catchup", type=Path, default=ROOT / "port/assets/changelog/catchup.json")
    parser.add_argument("--build-number", type=int, help="positive GitHub Actions build number; otherwise preview")
    parser.add_argument("--preview", action="store_true", help="create an UNRELEASED entry without a build number (default)")
    parser.add_argument("--previous", type=Path, help="published schema-2 changelog archive for --base")
    parser.add_argument("--require-notes", action="store_true",
                        help="reject new release commits without notes or explicit omission review")
    parser.add_argument("--output", type=Path, required=True, help="directory for changelog.json and notes.md")
    args = parser.parse_args(argv)
    try:
        data, markdown, warnings = collect(ROOT, args.head, args.base, args.catchup, args.preview,
                                          args.build_number, args.previous, args.require_notes)
        write_outputs(args.output, data, markdown)
    except (NotesError, OSError) as exc:
        parser.exit(1, f"player notes: {exc}\n")
    for warning in warnings:
        print(f"player notes: {warning}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
