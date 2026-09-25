"""Small decoding controls; no original game/compiler binaries are required."""
import importlib.util
from pathlib import Path

SPEC = importlib.util.spec_from_file_location(
    "return_contract_census", Path(__file__).with_name("return_contract_census.py"))
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def observe(hex_bytes, entries=()):
    return MOD.observation(list(MOD.MD.disasm(bytes.fromhex(hex_bytes), 0)), 0, entries)


def test_cleanup_first_positive():
    # call; add esp,8; mov edi,eax; cmp di,bx
    row = observe("e800000000 83c408 8bf8 663bfb")
    assert row["narrow_compare_width"] == 16
    assert row["cleanup_before_copy"] is True


def test_copy_first_positive():
    row = observe("e800000000 8bf8 83c408 663bfb")
    assert row["cleanup_before_copy"] is False


def test_return_extension_positive():
    assert observe("e800000000 0fbec8")["extension"] == ["movsx", 8]
    assert observe("e800000000 0fb7c8")["extension"] == ["movzx", 16]


def test_redefined_return_is_not_evidence():
    assert observe("e800000000 33c0 0fbec8")["extension"] is None
    assert observe("e800000000 b001 0fbec8")["extension"] is None
    assert observe("e800000000 b400 0fb7c8")["extension"] is None


def test_copy_overwrite_is_not_evidence():
    row = observe("e800000000 8bd8 83c408 bb00000000 80fb00")
    assert row["narrow_compare_width"] is None
    row = observe("e800000000 8bd8 83c408 b300 80fb00")
    assert row["narrow_compare_width"] is None


def test_call_jump_and_join_end_observation():
    assert observe("e800000000 e800000000 0fbec8")["extension"] is None
    assert observe("e800000000 e202 0fbec8")["extension"] is None
    assert observe("e800000000 0fbec8", entries={5})["extension"] is None
    assert observe("e800000000 0f0b 0fbec8")["extension"] is None
    assert observe("e800000000 f4 0fbec8")["extension"] is None


def test_high_byte_is_not_low_return_contract():
    row = observe("e800000000 89c3 83c404 80ff00")
    assert row["narrow_compare_width"] is None


def test_implicit_eax_writes_kill_contract():
    assert observe("e800000000 f7e1 0fbec8")["extension"] is None
    assert observe("e800000000 58 0fbec8")["extension"] is None


def test_incoming_loop_target_is_a_join():
    insns = list(MOD.MD.disasm(bytes.fromhex("e800000000900fbec831c0e2f8"), 0))
    entries = MOD.block_entry_offsets(insns)
    assert entries == {5}
    assert MOD.observation(insns, 0, entries)["extension"] is None


def test_repeated_call_pairing_is_conservative():
    assert list(MOD.paired([{"callee": "a"}, {"callee": "a"}],
                           [{"callee": "a"}])) == []
    assert len(list(MOD.paired([{"callee": "a"}, {"callee": "a"}],
                               [{"callee": "a"}, {"callee": "a"}]))) == 2
    ambiguous = [{"callee": "a"}, {"callee": None}, {"callee": "a"}]
    assert list(MOD.paired(ambiguous, ambiguous)) == []
