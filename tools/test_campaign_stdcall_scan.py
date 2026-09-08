"""Tests for conservative __stdcall naming proposals."""

from tools import coff_compare as cc
from tools.campaign import stdcall_scan


def make_object():
    bodies = [
        b"\x55\xc2\x08\x00\x90\xcc\x90",
        b"\xc2\x0c\x00",
        b"\xc2\x08\x00",
        b"\xc3\x90",
    ]
    names = [
        "_code_00123456",
        "_semantic_name",
        "_already@8",
        "_code_87654321",
    ]
    sections = [
        {
            "name": ".text",
            "size": len(body),
            "raw_data": body,
            "flags": 0x60001020,
        }
        for body in bodies
    ]
    symbols = [
        {
            "name": name,
            "value": 0,
            "section": index,
            "type": 0x20,
            "storage": 2,
            "aux_count": 0,
        }
        for index, name in enumerate(names, start=1)
    ]
    return cc.build_coff(sections=sections, symbols=symbols)


def write_target(tmp_path, unit="source/example"):
    object_path = tmp_path / "build" / "split" / (unit + ".obj")
    object_path.parent.mkdir(parents=True, exist_ok=True)
    object_path.write_bytes(make_object())
    return object_path


def test_terminal_ret_parser_handles_mixed_padding_and_full_immediate():
    assert stdcall_scan.stdcall_argument_bytes(
        b"\x55\xc2\x08\x00\x90\xcc\x90"
    ) == 8
    assert stdcall_scan.stdcall_argument_bytes(b"\xc2\x00\x01") == 256
    assert stdcall_scan.stdcall_argument_bytes(b"\xc3\x90\xcc") is None
    assert stdcall_scan.stdcall_argument_bytes(b"\xc2\x02\x00") is None
    assert stdcall_scan.stdcall_argument_bytes(b"\xc2\x04\x10") is None


def test_default_scan_only_proposes_address_placeholders():
    obj = cc.load(make_object())
    proposals = stdcall_scan.scan_object(obj)
    assert [(item.name, item.argument_bytes) for item in proposals] == [
        ("_code_00123456", 8),
    ]


def test_all_undecorated_still_excludes_decorated_and_cdecl_names():
    obj = cc.load(make_object())
    proposals = stdcall_scan.scan_object(obj, all_undecorated=True)
    assert [(item.name, item.argument_bytes) for item in proposals] == [
        ("_code_00123456", 8),
        ("_semantic_name", 12),
    ]


def test_missing_explicit_unit_is_a_nonzero_error(tmp_path, capsys):
    result = stdcall_scan.main([
        "source/missing",
        "--root",
        str(tmp_path),
    ])
    output = capsys.readouterr()
    assert result == 2
    assert "source/missing" in output.err
    assert "0 non-authoritative" not in output.out


def test_keep_going_marks_partial_scan_nonzero(tmp_path, capsys):
    write_target(tmp_path)
    result = stdcall_scan.main([
        "source/example",
        "source/missing",
        "--keep-going",
        "--root",
        str(tmp_path),
    ])
    output = capsys.readouterr()
    assert result == 1
    assert "_code_00123456" in output.out
    assert "result is partial" in output.err
    assert "source/missing" in output.err


def test_valid_explicit_scan_succeeds_with_truthful_summary(tmp_path, capsys):
    write_target(tmp_path)
    result = stdcall_scan.main([
        "source/example",
        "source/example",
        "--root",
        str(tmp_path),
    ])
    output = capsys.readouterr()
    assert result == 0
    assert "1 non-authoritative __stdcall naming proposal(s)" in output.out
    assert "manual provenance review required" in output.out
    assert not output.err


def test_all_scan_refuses_an_empty_built_directory(tmp_path, capsys):
    (tmp_path / "build" / "split" / "source").mkdir(parents=True)
    result = stdcall_scan.main(["--all", "--root", str(tmp_path)])
    output = capsys.readouterr()
    assert result == 2
    assert "no built target objects found" in output.err
