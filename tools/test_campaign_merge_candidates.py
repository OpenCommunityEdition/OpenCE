"""Tests for the fail-closed candidate-copy merger."""

import pytest

from tools.campaign import merge_candidates as merger


BASE = "one\r\ntwo\r\nthree\r\nfour\r\n"


def test_merges_disjoint_replacements_deterministically():
    candidate_a = BASE.replace("two", "TWO")
    candidate_b = BASE.replace("four", "FOUR")
    merged, hunks = merger.merge_candidate_texts(
        BASE,
        [("b.c", candidate_b), ("a.c", candidate_a)],
    )
    assert merged == "one\r\nTWO\r\nthree\r\nFOUR\r\n"
    assert [(hunk.source, hunk.start, hunk.end) for hunk in hunks] == [
        ("a.c", 1, 2),
        ("b.c", 3, 4),
    ]


def test_rejects_overlapping_replacements():
    with pytest.raises(merger.MergeError, match="overlapping edits"):
        merger.merge_candidate_texts(
            BASE,
            [
                ("a.c", BASE.replace("two", "TWO")),
                ("b.c", BASE.replace("two", "deux")),
            ],
        )


def test_rejects_two_insertions_at_same_base_position():
    with pytest.raises(merger.MergeError, match="overlapping edits"):
        merger.merge_candidate_texts(
            BASE,
            [
                ("a.c", BASE.replace("two", "insert-a\r\ntwo")),
                ("b.c", BASE.replace("two", "insert-b\r\ntwo")),
            ],
        )


def test_rejects_insertion_touching_replacement_boundary():
    with pytest.raises(merger.MergeError, match="overlapping edits"):
        merger.merge_candidate_texts(
            BASE,
            [
                ("a.c", BASE.replace("two", "insert-a\r\ntwo")),
                ("b.c", BASE.replace("two", "TWO")),
            ],
        )


def test_rejects_mixed_or_changed_line_endings():
    mixed = "one\r\ntwo\nthree\r\nfour\r\n"
    with pytest.raises(merger.MergeError, match="mixed line endings"):
        merger.merge_candidate_texts(BASE, [("mixed.c", mixed)])

    changed = BASE.replace("\r\n", "\n").replace("two", "TWO")
    with pytest.raises(merger.MergeError, match="line endings"):
        merger.merge_candidate_texts(BASE, [("changed.c", changed)])


def test_cli_writes_atomically_beneath_scratch(tmp_path, capsys):
    base = tmp_path / "base.c"
    candidate = tmp_path / "candidate.c"
    output = tmp_path / "scratch" / "merged.c"
    base.write_bytes(BASE.encode("latin-1"))
    candidate.write_bytes(BASE.replace("two", "TWO").encode("latin-1"))

    result = merger.main([
        str(base),
        str(output),
        str(candidate),
        "--root",
        str(tmp_path),
    ])

    assert result == 0
    assert output.read_bytes() == BASE.replace("two", "TWO").encode("latin-1")
    assert not list(output.parent.glob(".*.tmp"))
    assert "wrote" in capsys.readouterr().out


def test_cli_rejects_output_outside_scratch_and_input_alias(tmp_path, capsys):
    base = tmp_path / "base.c"
    candidate = tmp_path / "candidate.c"
    base.write_bytes(BASE.encode("latin-1"))
    candidate.write_bytes(BASE.replace("two", "TWO").encode("latin-1"))

    outside = tmp_path / "source" / "merged.c"
    assert merger.main([
        str(base),
        str(outside),
        str(candidate),
        "--root",
        str(tmp_path),
    ]) == 1
    assert not outside.exists()

    scratch_base = tmp_path / "scratch" / "base.c"
    scratch_base.parent.mkdir()
    scratch_base.write_bytes(BASE.encode("latin-1"))
    assert merger.main([
        str(scratch_base),
        str(scratch_base),
        str(candidate),
        "--root",
        str(tmp_path),
    ]) == 1
    assert "must not alias" in capsys.readouterr().err
