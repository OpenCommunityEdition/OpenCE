"""Real MCC campaign handlers against local/remote players and state snapshots."""
import subprocess
import pytest
from test_mcc_scripts import native_mcc_harness


@pytest.fixture(scope="module")
def campaign_tool(native_mcc_harness):
    return native_mcc_harness("mcc_campaign")


@pytest.mark.parametrize("case", ["players", "distance", "gravity", "controls", "navigation", "validation", "bits", "print_predict"])
def test_campaign_handlers(campaign_tool, case):
    result = subprocess.run([str(campaign_tool), case], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
