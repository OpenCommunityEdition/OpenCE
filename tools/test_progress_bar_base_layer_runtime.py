"""Fixture-free checks for the bounded private-wrapper audit's rejection gates.

Actual target/first/ordinary object execution is a separate pinned audit run;
these tests do not claim that source or target binaries are available in CI.
"""

import hashlib
import struct

import pytest

from tools.audit import progress_bar_base_layer_runtime as audit


def test_hash_pin_accepts_only_the_given_content():
    data = b"independent pin test"
    pin = hashlib.sha256(data).hexdigest()
    assert audit.verify_pin(data, pin) == pin
    with pytest.raises(AssertionError, match="object SHA-256 mismatch"):
        audit.verify_pin(data + b"!", pin)
    with pytest.raises(AssertionError, match="invalid SHA-256 pin"):
        audit.verify_pin(data, pin.upper())


@pytest.mark.parametrize("bits", audit.PROGRESS_BITS)
def test_transport_cases_are_raw_32_bit_patterns(bits):
    for control_word in audit.CONTROL_WORDS:
        audit.validate_case(bits, control_word)
        audit.check_boundary(audit.EXPECTED_LAYER, audit.COLOR, (0x3F800000, bits), bits)


@pytest.mark.parametrize("bits", (-1, 0x100000000, 0.5, True))
def test_invalid_transport_input_is_rejected(bits):
    with pytest.raises(AssertionError):
        audit.validate_case(bits, audit.CONTROL_WORDS[0])


def test_unreviewed_control_word_is_rejected():
    with pytest.raises(AssertionError):
        audit.validate_case(0, 0x007F)


def test_complete_layer_contract_rejects_wrong_width_and_distance():
    for offset, value in ((0, 641.0), (24, 1.0)):
        layer = bytearray(audit.EXPECTED_LAYER)
        struct.pack_into("<f", layer, offset, value)
        with pytest.raises(AssertionError, match="layer construction mismatch"):
            audit.check_boundary(bytes(layer), audit.COLOR, (0x3F800000, 0), 0)


def test_pointer_and_bit_transport_are_not_float_equal_comparisons():
    with pytest.raises(AssertionError, match="color pointer mismatch"):
        audit.check_boundary(audit.EXPECTED_LAYER, audit.COLOR + 4, (0x3F800000, 0), 0)
    with pytest.raises(AssertionError, match="alpha/progress transport mismatch"):
        audit.check_boundary(audit.EXPECTED_LAYER, audit.COLOR, (0x3F800000, 0), 0x80000000)
    with pytest.raises(AssertionError, match="alpha/progress transport mismatch"):
        audit.check_boundary(audit.EXPECTED_LAYER, audit.COLOR, (0x3F800000, 0x7FC00000), 0x7FC12345)


def test_mutations_require_the_reviewed_instruction_site():
    with pytest.raises(AssertionError):
        audit.mutation(b"\x90" * 96, "width")
    with pytest.raises(AssertionError):
        audit.mutation(b"\x90" * 96, "progress")
    with pytest.raises(ValueError, match="unknown mutation"):
        audit.mutation(b"\x90" * 96, "unreviewed")
