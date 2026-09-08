"""Tests for complete, fail-closed relocation comparison."""

from tools import coff_compare as cc
from tools.campaign import relocdiff


FUNCTION = "_example"
UNIT = "source/example"


def make_object(callee=None, address=4, relocation_type=6):
    symbols = [{
        "name": FUNCTION,
        "value": 0,
        "section": 1,
        "type": 0x20,
        "storage": 2,
        "aux_count": 0,
    }]
    relocations = []
    if callee is not None:
        symbols.append({
            "name": callee,
            "value": 0,
            "section": 0,
            "type": 0,
            "storage": 2,
            "aux_count": 0,
        })
        relocations.append((address, 1, relocation_type, 0))
    raw, relocation_data = cc.make_section_raw(32, relocations)
    return cc.build_coff(
        sections=[{
            "name": ".text",
            "size": 32,
            "raw_data": raw,
            "reloc_data": relocation_data,
            "flags": 0x60001020,
        }],
        symbols=symbols,
    )


def write_pair(tmp_path, target, ours):
    target_path = tmp_path / "build" / "split" / "source" / "example.obj"
    ours_path = tmp_path / "build" / "base" / "source" / "example.obj"
    target_path.parent.mkdir(parents=True)
    ours_path.parent.mkdir(parents=True)
    target_path.write_bytes(target)
    ours_path.write_bytes(ours)


def relocation(name, address=4, relocation_type=6):
    return {
        "address": address,
        "type": relocation_type,
        "target": ["symbol", name, 0],
    }


def info(relocations):
    return {
        "size": 32,
        "relocation_count": len(relocations),
        "normalized_sha256": "same",
        "relocations": relocations,
    }


def test_identity_preconditions_validate_address_and_type():
    target = info([relocation("_target", address=4, relocation_type=6)])
    moved = info([relocation("_target", address=8, relocation_type=20)])
    problems = relocdiff.identity_precondition_problems(target, moved)
    assert "relocation 0 address 0x4 != 0x8" in problems
    assert "relocation 0 type 0x6 != 0x14" in problems


def test_rows_retain_unmatched_relocation_tails():
    rows = relocdiff.relocation_rows(
        info([relocation("_a"), relocation("_b", address=8)]),
        info([relocation("_a")]),
    )
    assert len(rows) == 2
    assert rows[0].equal
    assert rows[1].target["target"][1] == "_b"
    assert rows[1].ours is None
    assert not rows[1].equal


def test_cli_reports_identity_only_difference(tmp_path, capsys):
    write_pair(
        tmp_path,
        make_object("_january_target"),
        make_object("_candidate_target"),
    )
    result = relocdiff.main([
        UNIT,
        FUNCTION,
        "--root",
        str(tmp_path),
        "--count-by-target",
    ])
    output = capsys.readouterr()
    assert result == 0
    assert "1 differing row(s)" in output.out
    assert "_january_target" in output.out
    assert "_candidate_target" in output.out
    assert "TOTAL" in output.out
    assert not output.err


def test_cli_refuses_structural_comparison_by_default(tmp_path, capsys):
    write_pair(tmp_path, make_object("_callee"), make_object())
    result = relocdiff.main([UNIT, FUNCTION, "--root", str(tmp_path)])
    output = capsys.readouterr()
    assert result == 2
    assert "not a pure relocation-identity residual" in output.err
    assert "relocation count 1 != 0" in output.err
    assert "Traceback" not in output.err


def test_allow_structural_renders_missing_tail(tmp_path, capsys):
    write_pair(tmp_path, make_object("_callee"), make_object())
    result = relocdiff.main([
        UNIT,
        FUNCTION,
        "--root",
        str(tmp_path),
        "--allow-structural",
    ])
    output = capsys.readouterr()
    assert result == 0
    assert "<missing>" in output.out
    assert "summary: 1 target, 0 candidate, 1 differing row(s)" in output.out
    assert "structural differences:" in output.out
    assert not output.err


def test_missing_function_is_a_friendly_nonzero_error(tmp_path, capsys):
    write_pair(tmp_path, make_object(), make_object())
    result = relocdiff.main([UNIT, "_missing", "--root", str(tmp_path)])
    output = capsys.readouterr()
    assert result == 2
    assert "missing or ambiguously owned" in output.err
    assert "Traceback" not in output.err
