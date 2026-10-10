"""Exercise MCC capacity, graph validation and isolated script safety traversal."""
import subprocess

import pytest

from test_mcc_scripts import native_mcc_harness


@pytest.fixture(scope="module")
def syntax_tool(native_mcc_harness):
    return native_mcc_harness("mcc_syntax")


@pytest.mark.parametrize("case", [
    "empty", "high_slot", "sparse", "deep", "wide", "deep_cycle", "salt", "zero_salt",
    "out_of_range", "unused", "sibling_cycle", "call_cycle", "short_blob", "long_blob",
    "negative_count", "negative_capacity", "count_over_capacity", "actual_count",
    "negative_actual", "free_hint", "signature", "node_size", "invalid_array", "callbacks",
    "root_type", "damaged_root", "shared_child",
])
def test_mcc_syntax_behavior(syntax_tool, case):
    result = subprocess.run([str(syntax_tool), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
