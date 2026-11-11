"""Actual P2P lifecycle code must finish its workers before exit returns."""
from pathlib import Path
import re
import subprocess
import pytest
from harness import function
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]
NAMES = ["upnp_thread", "p2p_shutdown", "update_upnp", "p2p_thread", "p2p_initialize"]


def build(tmp_path, mutation=None):
    source = (ROOT / "port/linux/src/p2p.c").read_text()
    if mutation:
        before, after = mutation
        assert before in source
        source = source.replace(before, after, 1)
    code = (ROOT / "tools/tests/p2p_shutdown.c").read_text()
    code = code.replace("/* ACTUAL INITIALIZER */", re.search(r"\} p2p = (\{[^;]+\});", source)[1])
    code = code.replace("/* ACTUAL FUNCTIONS */", "\n".join(function(source, name) for name in NAMES))
    path = tmp_path / "shutdown.c"
    path.write_text(code)
    return compile_tool(tmp_path, [path])


@pytest.fixture(scope="module")
def binary(tmp_path_factory):
    return build(tmp_path_factory.mktemp("p2p_shutdown"))


@pytest.mark.parametrize("case", ["poll", "flow", "upnp", "renew", "upnp_fail", "start_fail", "retry_start", "hook_fail", "offline"])
def test_workers_end_before_cleanup(binary, case):
    subprocess.run([str(binary), case], check=True, capture_output=True, timeout=10)


@pytest.mark.parametrize("mutation,case", [
    (("WaitForSingleObject(p2p.thread, INFINITE);", "/* detached: no wait */"), "poll"),
    (("WaitForSingleObject(p2p.upnp_thread, INFINITE);", "/* detached: no wait */"), "upnp"),
    (("if (p2p.stopping)\n\t\t\tbreak;", "/* process one more pass after stop */"), "poll"),
])
def test_regressions_reject_detached_or_post_stop_work(tmp_path, mutation, case):
    changed = build(tmp_path, mutation)
    result = subprocess.run([str(changed), case], capture_output=True, timeout=10)
    assert result.returncode != 0


@pytest.mark.parametrize("case", ["before", "discovery", "scan", "address", "retry", "permanent", "success", "normal"])
def test_upnp_cancellation_between_owned_requests(tmp_path, case):
    source = (ROOT / "port/linux/src/posix_upnp.c").read_text()
    names = ["posix_upnp_cancel", "upnp_cancelled", "swap_short", "upnp_remove_left_forwardings",
             "upnp_find_router", "upnp_add", "posix_upnp_forward_udp", "posix_upnp_stop_forwarding_udp"]
    code = (ROOT / "tools/tests/p2p_upnp_cancel.c").read_text()
    path = tmp_path / "upnp.c"
    path.write_text(code.replace("/* ACTUAL FUNCTIONS */", "\n".join(function(source, n) for n in names)))
    binary = compile_tool(tmp_path, [path])
    subprocess.run([str(binary), case], check=True, capture_output=True, timeout=10)
