"""MCC conversion progress cannot hold every round's start: each machine's allowance."""
from pathlib import Path
import subprocess

import pytest
from mcc_test_support import compile_tool

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "port/linux/game/mcc_load_network.c"
DRIVER = ROOT / "tools/tests/mcc_load_network.c"
CASES = ["pulse_only", "converting", "limit"]


@pytest.fixture(scope="module")
def load_tool(tmp_path_factory):
    return compile_tool(tmp_path_factory.mktemp("mcc-load"), [DRIVER, SOURCE])


@pytest.mark.parametrize("case", CASES)
def test_load_allowance(load_tool, case):
    result = subprocess.run([str(load_tool), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("before,after,case", [
    ("allowance->left = elapsed >= allowance->left ? 0 : allowance->left - elapsed;", "(void)0;", "pulse_only"),
    ("        elapsed /= MCC_LOAD_NETWORK_EARN;\n", "", "pulse_only"),
    ("MCC_LOAD_NETWORK_LIMIT : allowance->left + elapsed;", "allowance->left + elapsed : allowance->left + elapsed;", "limit"),
])
def test_negative_controls(tmp_path, before, after, case):
    """Each case must detect its intended fault in the real code."""
    source = SOURCE.read_text()
    assert source.count(before) == 1
    changed = tmp_path / "mcc_load_network.c"
    changed.write_text(source.replace(before, after))
    tool = compile_tool(tmp_path, [DRIVER, changed])
    result = subprocess.run([str(tool), case], capture_output=True, text=True, timeout=30)
    assert result.returncode != 0
