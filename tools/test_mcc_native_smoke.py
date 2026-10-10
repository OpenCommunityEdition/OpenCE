"""The startup checker must not repeat the old false-positive B40 result."""
import pytest
from mcc_native_smoke import check_result

LOADED = "mcc: loaded mcc_maps\\b40 (5172 tags, 20224832 bytes of runtime tags)"
MARKER = "mcc_smoke_test"


@pytest.mark.parametrize("fatal", ["type is inconsistent with usage (you need to recompile scripts.)",
                                  "the scenario's scripts won't run", "EXCEPTION halt"])
def test_loader_success_cannot_hide_fatal_script_error(fatal):
    assert not check_result(LOADED + "\n" + fatal, MARKER, MARKER, "b40", 0)["passed"]


@pytest.mark.parametrize("console", ["", '(begin (sleep 150) (print "mcc_smoke_test"))',
                                    "previous_mcc_smoke_test"])
def test_requires_actual_delayed_output(console):
    assert not check_result(LOADED, console, MARKER, "b40", 0)["passed"]


def test_completed_simulation_and_normal_exit():
    assert check_result(LOADED, "\r\nmcc_smoke_test\r\n", MARKER, "b40", 0)["passed"]
    assert not check_result(LOADED, MARKER, MARKER, "b40", 1)["passed"]
    assert not check_result(LOADED, MARKER, MARKER, "a10", 0)["passed"]
