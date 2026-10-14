"""net_compare.py's reading and judging of the network test's logs, on
logs written here (playing a game needs the retail maps: NETCODE.md)."""
import argparse
import sys
from pathlib import Path

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
