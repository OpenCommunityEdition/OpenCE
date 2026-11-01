"""Snapshot ownership, stale replies and cleanup of the optional path worker."""
import sys
from pathlib import Path
import pytest
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, build, run, mutated


@pytest.mark.parametrize("case", ["snapshot", "stale", "cleanup", "disabled", "malformed", "allocation", "graph_allocation"])
def test_worker(case):
    source = read("port/linux/game/bots_worker.inl")
    code = source[source.index("struct bots_path_job"):]
    status, output = run(build("bots_worker", (("under_test.inc", code),)), case)
    assert status == 0, output


def test_worker_uses_engine_allocator():
    source = read("port/linux/game/bots_worker.inl")
    code = source[source.index("struct bots_path_job"):]
    code = mutated(code, "malloc(BOTS_SEARCHES_A_TICK * sizeof(struct bots_path_job))",
                   "calloc(BOTS_SEARCHES_A_TICK, sizeof(struct bots_path_job))")
    status, output = run(build("bots_worker", (("under_test.inc", code),)), "cleanup")
    assert status == 1 and "does not belong to engine allocator" in output, output
