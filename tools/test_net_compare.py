"""net_compare.py's reading and judging of the network test's logs, on
logs written here (playing a game needs the retail maps: NETCODE.md)."""
import argparse
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import net_compare  # noqa: E402


def player(index, x, y=10.0, z=0.5, *, dead=False, kills=0, deaths=0, grenades="2/2", rounds=60, team=0, machine=0):
    if dead:
        return f" player {index}: dead s{kills} k{kills} d{deaths} f0 t{team} m{machine}"
    return (f" player {index}: ({x:.3f} {y:.3f} {z:.3f}) h1.00/1.00 g{grenades} w a51/-3 f55 l51/-3 as140/120 st4 "
            f"thr0.93 501:{rounds} 4e2:8 s{kills} k{kills} d{deaths} f0 t{team} m{machine}")


def line(tick, players, drift=(30, 0, 0.0, 0.0), local=0):
    return (f"halo-linux: network test: tick {tick}{''.join(players)} | items 12 (+0 -0 !0 x0) | playing to 25 | "
            f"sent 10 received 10 corrected 0 | hits 0 dealt 0 rejected 0 replayed 0 | local {local} camera "
            f"(1.0 2.0 3.0) respawn 0 | drift {drift[0]} {drift[1]} {drift[2]:.6f} {drift[3]:.6f}\n")


def write_logs(tmp_path, seconds=40, client_shift=0.0, drift=lambda second: (30, 0, 0.0, 0.0),
               client_player_1=None):
    host = tmp_path / "host.log"
    client = tmp_path / "client.log"
    with open(host, "w") as host_log, open(client, "w") as client_log:
        host_log.write("halo-linux: network test: player 0 is named Player\n")
        for second in range(1, seconds + 1):
            tick = second * net_compare.TICKS_PER_SECOND
            x = second * 2.0
            host_log.write(line(tick, [player(0, x), player(1, -x, machine=1)], drift(second), local=0))
            other = client_player_1(second) if client_player_1 else player(1, -x + client_shift, machine=1)
            client_log.write(line(tick, [player(0, x + client_shift), other], local=1))
    return host, client


def judge(host, client, **tolerances):
    arguments = argparse.Namespace(seconds=30, skip=3)
    merged = dict(net_compare.TOLERANCES)
    merged.update(tolerances)
    return net_compare.judge_run(host, client, arguments, merged)


def test_reads_a_line():
    players = net_compare.parse_players(player(0, 1.5) + player(1, 0, dead=True, deaths=2))
    assert players[0]["position"] == (1.5, 10.0, 0.5)
    assert players[0]["weapons"] == [("501", 60), ("4e2", 8)]
    assert players[0]["grenades"] == (2, 2)
    assert players[1] == {"alive": False, "score": 0, "kills": 0, "deaths": 2, "ff": 0, "team": 0, "machine": 0}


def test_in_step(tmp_path):
    result = judge(*write_logs(tmp_path, client_shift=0.1))
    assert not result["problems"] and not result["divergences"]


def test_position_apart(tmp_path):
    result = judge(*write_logs(tmp_path, client_shift=2.5))
    first = result["divergences"][0]
    assert first["field"] == "position" and first["tick"] == 3 * net_compare.TICKS_PER_SECOND


def test_a_second_apart_is_lag(tmp_path):
    host, client = write_logs(tmp_path, client_player_1=lambda second: player(
        1, -second * 2.0 + (5.0 if second == 10 else 0.0), machine=1))
    assert not judge(host, client)["divergences"]


def test_deaths_apart(tmp_path):
    host, client = write_logs(tmp_path, client_player_1=lambda second: player(
        1, -second * 2.0, deaths=1 if second >= 20 else 0, machine=1))
    first = judge(host, client)["divergences"][0]
    assert (first["player"], first["field"], first["tick"]) == (1, "deaths", 20 * net_compare.TICKS_PER_SECOND)


def test_drift_now_and_then_passes(tmp_path):
    result = judge(*write_logs(tmp_path, drift=lambda second: (30, 12, 0.003, 0.04) if second % 5 == 0 else (30, 1, 0.0, 0.0)))
    assert not result["divergences"]


def test_drift_every_second_fails(tmp_path):
    result = judge(*write_logs(tmp_path, drift=lambda second: (30, 28, 0.0015, 0.003) if second > 8 else (30, 0, 0.0, 0.0)))
    first = result["divergences"][0]
    assert (first["player"], first["field"], first["tick"]) == (1, "drift", 9 * net_compare.TICKS_PER_SECOND)


def test_no_drift_figures_is_a_problem(tmp_path):
    host, client = write_logs(tmp_path)
    host.write_text(host.read_text().replace(" | drift 30 0 0.000000 0.000000", ""))
    assert any("drift" in problem for problem in judge(host, client)["problems"])


# ---------- the exit status, the tolerances, the copies' folders and processes


def test_a_fail_is_not_masked_by_an_error():
    assert net_compare.exit_status(["PASS", "ERROR", "FAIL"]) == 1
    assert net_compare.exit_status(["FAIL", "ERROR"]) == 1
    assert net_compare.exit_status(["PASS", "ERROR"]) == 2
    assert net_compare.exit_status(["PASS", "PASS"]) == 0
    assert net_compare.exit_status([]) == 0


@pytest.mark.parametrize("setting, expected", [
    ("persist=4", ("persist", 4)),
    ("position=1.5", ("position", 1.5)),
    ("drift_share=0.7", ("drift_share", 0.7)),
])
def test_tolerance_read(setting, expected):
    assert net_compare.parse_tolerance(setting) == expected


@pytest.mark.parametrize("setting, reason", [
    ("persist=3.5", "whole number"),
    ("position=far", "a number"),
    ("position=nan", "finite"),
    ("aim=-1", "negative"),
    ("drift_share=1.5", "from 0 to 1"),
    ("speed=1", "no tolerance"),
    ("persist", "NAME=VALUE"),
])
def test_tolerance_refused(setting, reason):
    with pytest.raises(ValueError, match=reason):
        net_compare.parse_tolerance(setting)


def test_tolerance_reported_without_a_traceback(tmp_path):
    result = subprocess.run([sys.executable, str(Path(net_compare.__file__)), "--tolerance", "persist=3.5",
                             "--compare", "a", "b"], capture_output=True, text=True)
    assert result.returncode == 2
    assert "persist takes a whole number" in result.stderr and "Traceback" not in result.stderr


def make_data(tmp_path):
    data = tmp_path / "data"
    (data / "maps").mkdir(parents=True)
    return data


def test_copy_of_a_flat_folder(tmp_path):
    build = tmp_path / "build"
    build.mkdir()
    (build / "halo").write_text("#!/bin/sh\n")
    (build / "libSDL3.so.0").write_text("")
    (build / "config.toml").write_text("")
    copy = net_compare.prepare_copy(tmp_path / "work", "A", build / "halo", make_data(tmp_path))
    assert copy["executable"] == tmp_path / "work/A/bin/halo"
    assert (tmp_path / "work/A/bin/libSDL3.so.0").resolve() == build / "libSDL3.so.0"
    assert not (tmp_path / "work/A/bin/config.toml").exists()
    assert (copy["data"] / "maps").is_symlink()


def environment_of(window=False, display=None):
    args = argparse.Namespace(start=8.0, seconds=60, skip=3, events=False, window=window, display=display)
    copy = {"data": Path("/data"), "saves": Path("/saves")}
    return net_compare.copy_environment(copy, "127.0.0.210", "127.0.0.211", "join", 1, args, [])


def test_headless_copies_have_no_display(monkeypatch):
    monkeypatch.setenv("DISPLAY", ":0")
    monkeypatch.setenv("WAYLAND_DISPLAY", "wayland-0")
    environment = environment_of()
    assert "DISPLAY" not in environment and "WAYLAND_DISPLAY" not in environment
    assert environment["HALO_NULL_RENDERER"] == "1" and environment["SDL_VIDEO_DRIVER"] == "dummy"


def test_window_display(monkeypatch):
    monkeypatch.setenv("DISPLAY", ":0")
    monkeypatch.setenv("WAYLAND_DISPLAY", "wayland-0")
    assert environment_of(window=True)["DISPLAY"] == ":0"
    environment = environment_of(window=True, display=":99")
    assert environment["DISPLAY"] == ":99" and "WAYLAND_DISPLAY" not in environment
    assert environment["SDL_VIDEO_DRIVER"] == "x11" and environment["HALO_HIDDEN_WINDOW"] == "1"


# a copy that ignores SIGTERM and starts a process of its own, as a game
# under a wrapper might: each writes its PID
FAKE_GAME = """#!/bin/sh
trap '' TERM
sleep 300 &
echo $$ $! >> "{pids}"
while :; do sleep 1; done
"""


def alive(pid):
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return False
    # (a zombie waiting for its parent is gone)
    try:
        return Path(f"/proc/{pid}/stat").read_text().split(") ", 1)[1][0] != "Z"
    except OSError:
        return False


def wait_for(condition, seconds=10.0):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(0.1)
    return condition()


def fake_game(tmp_path):
    pids = tmp_path / "pids"
    game = tmp_path / "game" / "halo"
    game.parent.mkdir()
    game.write_text(FAKE_GAME.format(pids=pids))
    game.chmod(0o755)
    return game, pids


def read_pids(pids):
    return [int(pid) for pid in pids.read_text().split()] if pids.exists() else []


def test_stop_ends_the_copy_and_what_it_started(tmp_path):
    game, pids = fake_game(tmp_path)
    process = net_compare.start([str(game)])
    assert wait_for(lambda: len(read_pids(pids)) == 2)
    assert os.getpgid(process.pid) == process.pid  # (a session, and a group, of its own)
    net_compare.stop(process)
    assert process not in net_compare.RUNNING
    assert wait_for(lambda: not any(alive(pid) for pid in read_pids(pids)))


@pytest.mark.parametrize("number", [signal.SIGTERM, signal.SIGHUP, signal.SIGINT])
def test_the_tool_stopped_leaves_no_copy(tmp_path, number):
    game, pids = fake_game(tmp_path)
    tool = subprocess.Popen([sys.executable, str(Path(net_compare.__file__)), "--a", str(game), "--data",
                             str(make_data(tmp_path)), "--work", str(tmp_path / "work"), "--maps", "bloodgulch",
                             "--seconds", "30", "--timeout", "60"],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        # (the host and the client, each with its child)
        assert wait_for(lambda: len(read_pids(pids)) == 4, 20)
        tool.send_signal(number)
        _, errors = tool.communicate(timeout=30)
    finally:
        if tool.poll() is None:
            tool.kill()
    assert tool.returncode == 128 + number, errors
    assert f"stopped by signal {int(number)}" in errors and "Traceback" not in errors
    assert not [pid for pid in read_pids(pids) if alive(pid)]


def test_a_copy_dies_with_a_killed_tool(tmp_path):
    # (SIGKILL cannot be caught: the kernel ends the copies, PR_SET_PDEATHSIG;
    # what a copy started is its own, and the test's to end)
    game, pids = fake_game(tmp_path)
    tool = subprocess.Popen([sys.executable, str(Path(net_compare.__file__)), "--a", str(game), "--data",
                             str(make_data(tmp_path)), "--work", str(tmp_path / "work"), "--maps", "bloodgulch"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        assert wait_for(lambda: len(read_pids(pids)) == 4, 20)
        tool.kill()
        tool.wait(timeout=10)
        copies = read_pids(pids)[0::2]
        assert wait_for(lambda: not any(alive(pid) for pid in copies))
    finally:
        for pid in read_pids(pids)[0::2]:
            try:
                os.killpg(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
