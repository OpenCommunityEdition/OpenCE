import importlib.util
from pathlib import Path

import pytest

SPEC = importlib.util.spec_from_file_location("header_receipts", Path(__file__).with_name("analyse_receipts.py"))
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)


def packet():
    row = {"offset": 0, "bytes": "8b0dac000000", "symbol": [0] * 10}
    row["symbol"][9] = 172
    return row, bytes.fromhex(row["bytes"]), [{"address": 2, "target": ["symbol", "_rasterizer_frame_statistics", 172]}]


def test_same_projection_is_not_same_web_identity():
    assert M.compare_choices([{"reg": "ecx", "id": 39}], [{"reg": "ecx", "id": 85}]) == []


def test_unequal_lengths_fail():
    with pytest.raises(ValueError, match="Unequal"):
        M.compare_choices([{}], [])


def test_register_difference_is_reported():
    assert M.compare_choices([{"reg": "ecx"}], [{"reg": "edx"}])[0]["fields"] == {"reg": ["ecx", "edx"]}


def test_load_requires_three_independent_agreements():
    r, b, rel = packet()
    assert M.decode_absolute_load(r, b, rel)["member_offset"] == 172


@pytest.mark.parametrize("fault", ["bytes", "symbol", "reloc", "duplicate", "opcode"])
def test_load_anchor_rejects_disagreement(fault):
    r, b, rel = packet()
    if fault == "bytes": b = b"\0" * 6
    if fault == "symbol": r["symbol"][9] = 156
    if fault == "reloc": rel[0]["target"][2] = 156
    if fault == "duplicate": rel += rel[:]
    if fault == "opcode": r["bytes"] = "8b4dec000000"
    with pytest.raises(ValueError):
        M.decode_absolute_load(r, b, rel)
