"""MCC generated parameter metadata and the real MCC interpreter adapter."""
import subprocess

import pytest
from test_mcc_scripts import native_mcc_harness


@pytest.fixture(scope="module")
def parameters_tool(native_mcc_harness):
    return native_mcc_harness("mcc_script_parameters")


@pytest.mark.parametrize("case", ["metadata", "count", "type", "pointer", "slot", "orphan", "flags",
                                  "call_arity", "call_type", "global_scope", "legacy", "return", "snapshot",
                                  "nested_scope", "recursive_arguments", "lists", "set", "overflow", "ai_cast",
                                  "global_capacity", "global_overflow", "maximum", "disabled", "predicate", "script_kind",
                                  "high_node", "negative_count", "sleep_self", "sleep_named", "sleep_missing", "sleep_finished"])
def test_mcc_parameter_metadata_and_runtime(parameters_tool, case):
    result = subprocess.run([str(parameters_tool), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
