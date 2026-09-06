from contextlib import contextmanager
import faulthandler
from pathlib import Path
from unittest.mock import patch

import pytest

pytest.importorskip("unicorn")
from tools.audit import hud_get_return_eip_runtime as oracle


@contextmanager
def quiet_unicorn_windows_exceptions():
	"""Avoid reporting Unicorn's internally handled Windows access probes."""
	enabled = faulthandler.is_enabled()
	if enabled:
		faulthandler.disable()
	try:
		yield
	finally:
		if enabled:
			faulthandler.enable()


def test_real_instruction_contract() -> None:
	with quiet_unicorn_windows_exceptions():
		cases = oracle.evaluate(oracle.EXPECTED_RAW)
	assert len(cases) == 24
	assert not [case for case in cases if case["errors"]]


@pytest.mark.parametrize("displacement", [0, 8])
def test_wrong_frame_displacement_is_detected(displacement: int) -> None:
	code = bytearray(oracle.EXPECTED_RAW)
	code[2] = displacement
	with quiet_unicorn_windows_exceptions():
		cases = oracle.evaluate(bytes(code))
	assert len(cases) == 24
	assert all(case["errors"] for case in cases)


def mock_owner(*, flags: int = oracle.EXPECTED_FLAGS, selection: int = 1):
	obj = {
		"symbols": [{
			"name": oracle.SYMBOL,
			"section": 1,
			"value": 0,
			"storage": 2,
			"type": 0x20,
		}],
		"sections": [{"flags": flags}],
	}
	info = {
		"size": 16,
		"relocation_count": 0,
		"normalized_sha256": oracle.EXPECTED_NORMALIZED,
	}
	return (
		patch.object(oracle.cc, "load", return_value=obj),
		patch.object(oracle.cc, "section_info", return_value=info),
		patch.object(oracle.cc, "_section_bytes", return_value=oracle.EXPECTED_RAW),
		patch.object(oracle, "_section_aux_selection", return_value=selection),
		patch.object(oracle, "sha256", return_value="ab" * 32),
	)


def call_mock_owner(*, flags: int = oracle.EXPECTED_FLAGS, selection: int = 1) -> dict:
	patches = mock_owner(flags=flags, selection=selection)
	with patches[0], patches[1], patches[2], patches[3], patches[4]:
		return oracle.load_owner(Path("mock.obj"))


def test_complete_exact_owner_metadata_is_accepted() -> None:
	owner = call_mock_owner()
	assert owner["raw"] == oracle.EXPECTED_RAW
	assert owner["selection"] == 1
	assert owner["flags"] == oracle.EXPECTED_FLAGS
	assert owner["file_sha256"] == "ab" * 32


@pytest.mark.parametrize(
	("flags", "selection"),
	[
		(oracle.EXPECTED_FLAGS, 2),
		(oracle.EXPECTED_FLAGS & ~oracle.IMAGE_SCN_CNT_CODE, 1),
		(oracle.EXPECTED_FLAGS & ~oracle.IMAGE_SCN_LNK_COMDAT, 1),
		(oracle.EXPECTED_FLAGS & ~oracle.IMAGE_SCN_MEM_EXECUTE, 1),
	],
)
def test_wrong_owner_metadata_is_rejected(flags: int, selection: int) -> None:
	with pytest.raises(AssertionError):
		call_mock_owner(flags=flags, selection=selection)
