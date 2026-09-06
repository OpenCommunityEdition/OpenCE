from contextlib import contextmanager
import faulthandler

import pytest

pytest.importorskip("unicorn")
from tools.audit import hud_weapon_crosshair_runtime as oracle


@contextmanager
def quiet_unicorn_windows_exceptions():
	enabled = faulthandler.is_enabled()
	if enabled:
		faulthandler.disable()
	try:
		yield
	finally:
		if enabled:
			faulthandler.enable()


def test_real_setter_contract() -> None:
	with quiet_unicorn_windows_exceptions():
		cases = oracle.evaluate(oracle.EXPECTED_RAW)
	assert len(cases) == len(oracle.ARGUMENTS) * len(oracle.FLAG_SEEDS)
	assert not [case for case in cases if case["errors"]]
	assert {case["initial_df"] for case in cases} == {0, 1}
	assert all(case["actual_df"] == case["initial_df"] for case in cases)


@pytest.mark.parametrize(
	("argument", "initial_flags", "expected_flags"),
	[
		(0x00000100, 0xFFFFFFFF, 0xFFFFFFFE),
		(0x80000000, 0x00000003, 0x00000002),
		(0x00000101, 0xA5A5A5A4, 0xA5A5A5A5),
		(0x12345680, 0x5A5A5A5B, 0x5A5A5A5B),
	],
)
def test_only_low_boolean_byte_controls_bit_zero(
	argument: int, initial_flags: int, expected_flags: int
) -> None:
	with quiet_unicorn_windows_exceptions():
		case = oracle.run_case(oracle.EXPECTED_RAW, argument, initial_flags)
	assert not case["errors"]
	assert case["actual_flags"] == expected_flags


@pytest.mark.parametrize(
	("offset", "value"),
	[(5, 9), (0x17, 2), (0x22, 0xFC)],
)
def test_semantic_mutation_is_detected(offset: int, value: int) -> None:
	code = bytearray(oracle.EXPECTED_RAW)
	code[offset] = value
	with quiet_unicorn_windows_exceptions():
		cases = oracle.evaluate(bytes(code))
	assert any(case["errors"] for case in cases)
