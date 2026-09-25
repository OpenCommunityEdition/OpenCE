"""Run with python -m pytest -q research/astra_breakthroughs_20260925/census/test_census.py."""
import importlib.util
from pathlib import Path

SPEC = importlib.util.spec_from_file_location("fresh_residual_census", Path(__file__).with_name("census.py"))
CENSUS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CENSUS)


def observe(code, symbol="__chkstk", address=9, kind=20, addend=0):
    rows = [] if symbol is None else [{"address": address, "type": kind,
                                      "target": ["symbol", symbol, addend]}]
    return CENSUS.allocation_from_body(bytes.fromhex(code), rows)


def test_fixed_frame_short_immediate():
    result = observe("558bec83ec305356", None)
    assert result["sub_esp"] == result["allocation_bytes"] == 48
    assert result["allocation_method"] == "sub-esp"


def test_fixed_frame_long_immediate():
    result = observe("558bec81ec1008000053", None)
    assert result["allocation_bytes"] == 0x810


def test_chkstk_requires_relocation_and_adjacent_immediate():
    result = observe("558becb808730000e80000000053")
    assert result["sub_esp"] is None
    assert result["allocation_bytes"] == 0x7308
    assert result["allocation_method"] == "relocation-verified-chkstk-eax"


def test_no_assumption_from_immediate_before_unrelated_call():
    result = observe("558becb808730000e80000000053", "_unrelated")
    assert result["allocation_bytes"] is None


def test_no_assumption_without_relocation():
    assert observe("558becb808730000e80000000053", None)["allocation_bytes"] is None


def test_wrong_relocation_type_address_or_addend():
    body = "558becb808730000e80000000053"
    for options in ({"kind": 6}, {"address": 8}, {"addend": 4}):
        assert observe(body, **options)["allocation_bytes"] is None


def test_intervening_eax_write_rejected():
    # Original frame-like immediate is stale when xor replaces EAX before call.
    body = "558becb80873000031c0e80000000053"
    assert observe(body, address=11)["allocation_bytes"] is None


def test_nonadjacent_immediate_conservatively_rejected():
    body = "558becb80873000090e80000000053"
    assert observe(body, address=10)["allocation_bytes"] is None


def test_other_immediate_register_rejected():
    assert observe("558becb908730000e80000000053")["allocation_bytes"] is None


def test_function_body_stack_change_after_call_not_a_prologue():
    body = "558bece80000000081ec10080000"
    assert observe(body, "_unrelated", address=4)["allocation_bytes"] is None


def test_return_terminates_prologue_scan():
    assert observe("c381ec10080000", None)["allocation_bytes"] is None


def test_jump_terminates_prologue_scan():
    assert observe("eb0081ec10080000", None)["allocation_bytes"] is None


def test_trap_terminates_prologue_scan():
    assert observe("0f0b81ec10080000", None)["allocation_bytes"] is None
