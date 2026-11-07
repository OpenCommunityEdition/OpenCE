"""Keep MCC checks executable in CI, including scripts pytest cannot collect."""
import ast
from pathlib import Path
import re

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[1]


def ci_commands():
    for path in (ROOT / ".github/workflows").glob("*.yml"):
        workflow = yaml.safe_load(path.read_text(encoding="utf-8"))
        for job in workflow.get("jobs", {}).values():
            for step in job.get("steps", []):
                if "run" in step and not step.get("continue-on-error"):
                    yield step["run"].replace("\\\n", " ")


def pytest_inputs(commands):
    for command in commands:
        for args in re.findall(r"\bpython3?\s+-m\s+pytest\b([^\n]*)", command):
            yield from re.findall(r"\btools/[\w./-]+", args)


def has_pytest_tests(path):
    tree = ast.parse(path.read_text(encoding="utf-8"))
    return any(isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and
               node.name.startswith("test_") for node in ast.walk(tree))


@pytest.fixture(scope="module")
def commands():
    return list(ci_commands())


@pytest.mark.parametrize("path", sorted((ROOT / "tools").rglob("test_*mcc*.py")),
                         ids=lambda path: path.relative_to(ROOT).as_posix())
def test_mcc_check_has_ci_runner(path, commands):
    relative = path.relative_to(ROOT).as_posix()
    if has_pytest_tests(path):
        # Passing a directory to pytest covers its discovered harness tests.
        assert any(relative == entry or relative.startswith(entry.rstrip("/") + "/")
                   for entry in pytest_inputs(commands)), relative
    else:
        # A standalone main() in a pytest argument list executes no checks.
        direct = r"\bpython3?\s+" + re.escape(relative) + r"(?:\s|$)"
        assert any(re.search(direct, command) for command in commands), relative


def test_standalone_meter_is_not_pytest_coverage():
    path = ROOT / "tools/test_mcc_hud_meter_render.py"
    assert not has_pytest_tests(path)
    assert list(pytest_inputs(["python3 tools/test_mcc_hud_meter_render.py"])) == []


def test_native_windows_reservation_checks_run_on_windows():
    workflow = yaml.safe_load((ROOT / ".github/workflows/mcc.yml").read_text(encoding="utf-8"))
    job = workflow["jobs"]["windows-adapters"]
    assert job["runs-on"].startswith("windows-")
    inputs = set(pytest_inputs(step.get("run", "") for step in job["steps"]))
    assert {"tools/test_mcc_memory.py", "tools/test_windows_mcc_startup.py"} <= inputs
