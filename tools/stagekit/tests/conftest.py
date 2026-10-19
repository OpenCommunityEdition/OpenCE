"""StageKit's tests' fixtures (README.md): the generator, built once, and each scenario played (twice) and shared by
every test that asks for it. A scenario's test module (test_<name>.py) says how it is played: PLAY, play()'s
arguments."""

import concurrent.futures
import importlib
import os
import shutil
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import runner  # noqa: E402

# the Sessions the running test has read, whose logs a failure shows
READ = []


@pytest.fixture(autouse=True)
def read_sessions():
    READ.clear()
    yield


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    report = (yield).get_result()
    if report.when == "call" and report.failed:
        for session in READ:
            report.sections.append((f"the game's logs ({session.folder.name})", session.tails()))


@pytest.fixture(scope="session")
def stagekit(tmp_path_factory):
    if not shutil.which(os.environ.get("CC", "clang")) or not shutil.which("ninja") or not (runner.ROOT / "build.ninja").is_file() or \
            not (runner.BUILD / "map_validate").is_file():
        pytest.skip("needs clang, ninja and ninja linux's build (build/linux/map_validate)")
    return runner.build(tmp_path_factory.mktemp("stagekit"))


def sessions(stagekit, tmp_path_factory, setting, play, copies=1):
    """play(scenario, copy=0) for each scenario whose test module plays it so (setting), copies times each: every
    one the first time any is asked for, side by side. A run that failed is kept as its exception, which every test
    of it then fails with; none plays it again"""
    played = {}

    def generated_and_played(name, slot, folder, settings):
        runner.generate(stagekit, name, folder)
        return play(folder, slot=slot, **settings)

    def session(scenario, copy=0):
        if not played:
            if not runner.GAME.is_file():
                pytest.skip("needs the game (dist/halo-linux-release)")
            modules = {name: importlib.import_module(f"test_{name}") for name in runner.SCENARIOS}
            runs = [(name, copy, getattr(modules[name], setting)) for name in runner.SCENARIOS
                    if hasattr(modules[name], setting) for copy in range(copies)]
            with concurrent.futures.ThreadPoolExecutor(max(1, len(runs))) as pool:
                results = {(name, copy): pool.submit(generated_and_played, name, slot,
                                                     tmp_path_factory.mktemp(f"{name}-{setting.lower()}-{copy}"),
                                                     settings)
                           for slot, (name, copy, settings) in enumerate(runs)}
            for key, result in results.items():
                played[key] = result.exception() or result.result()
        if (scenario, copy) not in played:
            raise LookupError(f"tests/test_{scenario}.py has no {setting}")
        if isinstance(played[(scenario, copy)], BaseException):
            raise RuntimeError(f"{scenario} was not played") from played[(scenario, copy)]
        READ.append(played[(scenario, copy)])
        return played[(scenario, copy)]

    return session


@pytest.fixture(scope="session")
def play(stagekit, tmp_path_factory):
    """play(scenario, copy=0): its Session on one machine, played twice (copy 1: the same, tick for tick)"""
    return sessions(stagekit, tmp_path_factory, "PLAY", runner.play, copies=2)

