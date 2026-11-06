"""tools/ci_smoke.py: the start-up check of a built game, on stand-in programs.

    python -m pytest -q tools/test_ci_smoke.py
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import ci_smoke

GOOD = "print('halo-linux: settings: x'); print('halo-linux: data root: y'); import time; time.sleep(30)"


def smoke(script, **options):
    return ci_smoke.run([sys.executable, "-u", "-c", script], options.pop("seconds", 2),
                        options.pop("require", ci_smoke.DEFAULT_REQUIRE),
                        options.pop("forbid", ci_smoke.DEFAULT_FORBID), options.pop("env", None))


def test_passes_a_game_that_logs_and_waits():
    ok, output, problems = smoke(GOOD)
    assert ok, problems
    assert "halo-linux: data root: y" in output


def test_fails_without_the_start_up_lines():
    ok, _, problems = smoke("import time; time.sleep(30)")
    assert not ok
    assert any("settings" in problem for problem in problems)


def test_fails_on_a_forbidden_line():
    ok, _, problems = smoke(GOOD.replace("import time", "print('halo-linux: cannot reserve the window'); import time"))
    assert not ok
    assert problems == ['the log has "cannot reserve"']


def test_fails_when_the_game_dies_of_a_signal():
    ok, _, problems = smoke("print('halo-linux: settings: x'); print('halo-linux: data root: y');"
                            " import os, signal; os.kill(os.getpid(), signal.SIGSEGV)")
    assert not ok
    assert any("SIGSEGV" in problem for problem in problems)


def test_a_game_that_stops_by_itself_is_not_a_failure():
    ok, _, problems = smoke("print('halo-linux: settings: x'); print('halo-linux: data root: y')")
    assert ok, problems


def test_the_game_is_started_headless_with_no_updates():
    ok, output, _ = smoke("import os; print('halo-linux: settings: ' + os.environ['SDL_VIDEODRIVER'] + ' '"
                          " + os.environ['HALO_UPDATE_AUTO']); print('halo-linux: data root: ' + os.environ['HALO_DATA_ROOT'])")
    assert ok
    assert "settings: dummy false" in output


def test_main_returns_the_result(capsys):
    assert ci_smoke.main(["--seconds", "2", "--", sys.executable, "-u", "-c", GOOD]) == 0
    assert ci_smoke.main(["--seconds", "2", "--", sys.executable, "-c", "print('nothing')"]) == 1
    assert "ci_smoke: the log has no" in capsys.readouterr().err


def test_a_relative_program_path_is_resolved_first(tmp_path, monkeypatch):
    program = tmp_path / "game.sh"
    program.write_text("#!/bin/sh\necho 'halo-linux: settings: x'\necho 'halo-linux: data root: y'\n")
    program.chmod(0o755)
    monkeypatch.chdir(tmp_path)
    ok, _, problems = ci_smoke.run(["./game.sh"], 5, ci_smoke.DEFAULT_REQUIRE, [])
    assert ok, problems


def test_no_defaults_checks_only_what_is_named(capsys):
    script = "print('halo: Halo for Linux starting'); print('halo: no game data: quitting')"
    assert ci_smoke.main(["--seconds", "2", "--", sys.executable, "-u", "-c", script]) == 1
    assert ci_smoke.main(["--seconds", "2", "--no-defaults", "--require", "Halo for Linux starting",
                          "--", sys.executable, "-u", "-c", script]) == 0


def test_ci_starts_the_linux_builds():
    text = (Path(__file__).resolve().parent.parent / ".github" / "workflows" / "build.yml").read_text()
    assert 'tools/ci_smoke.py -- "$PWD/dist/halo-linux-release/halo"' in text
    assert 'tools/ci_smoke.py -- "$PWD/dist/halo-linux-debug/halo"' in text
