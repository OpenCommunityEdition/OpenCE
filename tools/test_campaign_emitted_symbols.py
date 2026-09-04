"""Candidate-only emission checks and the opt-in gate CLI contract."""

import os
from pathlib import Path
import runpy
from types import SimpleNamespace

import pytest

from tools import coff_compare as cc
from tools.campaign.emitted_symbols import forbidden_emitted_symbols


REPO_ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN = "_point_from_line3d"


def make_object(symbols, *, code_name=".text", code_flags=0x60001020):
    return cc.build_coff(
        sections=[
            {
                "name": code_name,
                "size": 16,
                "raw_data": b"\xc3" + b"\x90" * 15,
                "flags": code_flags,
            },
            {
                "name": ".data",
                "size": 4,
                "raw_data": b"\0" * 4,
                "flags": 0xC0000040,
            },
        ],
        symbols=[
            {
                "name": name,
                "section": section,
                "value": offset,
                "type": 0x20,
                "storage": storage,
            }
            for name, section, offset, storage in symbols
        ],
    )


@pytest.mark.parametrize("storage,offset", [(2, 0), (3, 8)])
def test_finds_code_definition_in_named_subsection(storage, offset):
    obj = cc.load(make_object(
        [(FORBIDDEN, 1, offset, storage)],
        code_name=".text$mn",
    ))
    findings = forbidden_emitted_symbols(obj, [FORBIDDEN])
    assert findings == [{
        "name": FORBIDDEN,
        "symbol_index": 0,
        "section_number": 1,
        "section_name": ".text$mn",
        "offset": offset,
        "section_size": 16,
    }]


@pytest.mark.parametrize("section,offset", [(0, 0), (0, 4), (-1, 0), (-2, 0), (2, 0)])
def test_undefined_common_absolute_debug_and_data_are_not_code(section, offset):
    obj = cc.load(make_object([(FORBIDDEN, section, offset, 2)]))
    assert forbidden_emitted_symbols(obj, [FORBIDDEN]) == []


def test_code_flag_not_section_spelling_decides_emission():
    obj = cc.load(make_object([(FORBIDDEN, 1, 0, 2)], code_flags=0xC0000040))
    assert forbidden_emitted_symbols(obj, [FORBIDDEN]) == []


def test_scans_all_repeated_symbols_but_deduplicates_requested_names():
    obj = cc.load(make_object([
        (FORBIDDEN, 0, 0, 2),
        (FORBIDDEN, 2, 0, 2),
        (FORBIDDEN, 1, 0, 2),
        (FORBIDDEN, 1, 8, 3),
        ("_other_forbidden", 1, 12, 3),
        ("_unrelated", 1, 14, 2),
    ]))
    findings = forbidden_emitted_symbols(
        obj, [FORBIDDEN, FORBIDDEN, "_other_forbidden"]
    )
    assert [(item["name"], item["symbol_index"]) for item in findings] == [
        (FORBIDDEN, 2), (FORBIDDEN, 3), ("_other_forbidden", 4)
    ]
    assert forbidden_emitted_symbols(obj, []) == []


def run_gate(monkeypatch, tmp_path, candidate, options):
    """Exercise the real CLI with a synthetic compiler result, never VC7/Ninja."""
    unit = "source/saved games/example"
    source = tmp_path / (unit + ".c")
    source.parent.mkdir(parents=True)
    source.write_text("void example(void) { return; }\n", encoding="ascii")
    (tmp_path / "scratch").mkdir()
    target = tmp_path / "build/split" / (unit + ".obj")
    target.parent.mkdir(parents=True)
    target.write_bytes(make_object([("_known", 1, 0, 2)]))
    (tmp_path / "build.ninja").write_text(
        "build build\\base\\source\\saved$ games\\example.obj: cl source.c\n"
        "  cflags = /O2 /Oy-\n"
        "build all: phony\n",
        encoding="ascii",
    )

    def fake_compile(command, **kwargs):
        assert "/Isource/saved games" in command
        assert "/O2" in command
        output = next(arg[3:] for arg in command if arg.startswith("/Fo"))
        Path(output).write_bytes(candidate)
        return SimpleNamespace(returncode=0, stdout="", stderr="")

    monkeypatch.chdir(tmp_path)
    monkeypatch.syspath_prepend(str(REPO_ROOT / "tools"))
    monkeypatch.setattr("subprocess.run", fake_compile)
    monkeypatch.setattr("sys.argv", ["gate.py", unit, "--fn", "_known", *options])
    runpy.run_path(str(REPO_ROOT / "tools/campaign/gate.py"), run_name="__main__")


def test_cli_fails_for_candidate_only_definition_outside_fn_selection(
    monkeypatch, tmp_path, capsys
):
    candidate = make_object([("_known", 1, 0, 2), (FORBIDDEN, 1, 8, 3)])
    with pytest.raises(SystemExit) as result:
        run_gate(monkeypatch, tmp_path, candidate, [
            "--forbid-emitted-symbol", "_absent",
            "--forbid-emitted-symbol", FORBIDDEN,
        ])
    assert result.value.code == 1
    output = capsys.readouterr().out
    assert "FORBIDDEN-EMITTED-SYMBOL " + FORBIDDEN in output
    assert "symbol-index=1 section=1:.text offset=0x8 section-size=16" in output
    assert "object=scratch/_gate_%d.obj" % os.getpid() in output
    assert "absent from the target or --fn selection" in output


def test_cli_guard_allows_undefined_reference(monkeypatch, tmp_path, capsys):
    candidate = make_object([("_known", 1, 0, 2), (FORBIDDEN, 0, 0, 2)])
    run_gate(monkeypatch, tmp_path, candidate, ["--forbid-emitted-symbol", FORBIDDEN])
    output = capsys.readouterr().out
    assert "== emitted-symbol guard passed (1 forbidden names checked)" in output
    assert "== exact 1  residual 0  unwritten 0  (of 1 listed)" in output


def test_cli_default_output_and_exit_remain_unchanged(monkeypatch, tmp_path, capsys):
    candidate = make_object([("_known", 1, 0, 2), (FORBIDDEN, 1, 8, 3)])
    run_gate(monkeypatch, tmp_path, candidate, [])
    output = capsys.readouterr().out
    assert "FORBIDDEN-EMITTED-SYMBOL" not in output
    assert "emitted-symbol guard" not in output
    assert "== exact 1  residual 0  unwritten 0  (of 1 listed)" in output
