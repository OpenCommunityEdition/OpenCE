"""The bots (port/linux/game/bots.c, bots.count) in the game: a local Slayer game on a stock map, played without a
person (debug.network_test "local:", the scripted controller 1 of debug.test_input) for a minute and more, in which
every bot walks the map, the bots kill and die, and the game ends cleanly; and a system link game, which no bot joins
whatever bots.count asks. Needs the retail maps in assets/maps and ninja linux's build/linux/halo; the game draws
offscreen (SDL's offscreen video driver), with no audio, internet play or update check."""

import math
import os
import re
import shutil
import subprocess
from pathlib import Path

import pytest

RETAIL_MAPS = Path("assets/maps")
GAME = Path("build/linux/halo")
# a player's line of the network test's each second (network_test.c): alive or dead, its kills, deaths, team and
# machine (a bot's is BOTS_FIRST_MACHINE, 113, and up)
PLAYER = re.compile(r"player (\d+): (?:\((-?[\d.]+) (-?[\d.]+) -?[\d.]+\)|dead).*? s-?\d+ k(-?\d+) d(-?\d+) f-?\d+ "
                    r"t-?\d+ m(\d+)")
BOTS_FIRST_MACHINE = 113


def _game(map_name):
    if not GAME.is_file() or not (RETAIL_MAPS / f"{map_name}.map").is_file():
        pytest.skip(f"needs {map_name}.map in assets/maps and ninja linux's build/linux/halo")


def _play(tmp_path, test, seconds, **settings):
    """the game played for that many seconds: (exit status, its output, debug.txt)"""
    (tmp_path / "data").mkdir()
    (tmp_path / "save").mkdir()
    (tmp_path / "data" / "maps").symlink_to(RETAIL_MAPS.resolve())
    game = tmp_path / "halo"
    shutil.copy(GAME, game)
    env = {**os.environ, "SDL_VIDEODRIVER": "offscreen", "SDL_AUDIODRIVER": "dummy",
           "HALO_DATA_ROOT": str(tmp_path / "data"), "HALO_SAVE_ROOT": str(tmp_path / "save"),
           "HALO_HIDDEN_WINDOW": "1", "HALO_FULLSCREEN": "0", "HALO_NO_VSYNC": "1", "HALO_NO_AUDIO": "1",
           "HALO_UPDATE_AUTO": "false", "HALO_NET_ONLINE": "false", "HALO_NET_ADDRESS": "127.0.0.1",
           "HALO_NET_BROADCAST": "127.255.255.255", "HALO_NETWORK_TEST": test, "HALO_NETWORK_TEST_START": "5",
           "HALO_NETWORK_TEST_SCORE": "500", "HALO_TEST_INPUT": "bot:1", "HALO_EXIT_AFTER": str(seconds + 20)}
    env.update({name: str(value) for name, value in settings.items()})
    result = subprocess.run([str(game)], cwd=tmp_path, env=env, capture_output=True, text=True,
                            errors="replace", timeout=seconds + 120)
    debug = tmp_path / "data" / "debug.txt"
    return result.returncode, result.stdout + result.stderr, debug.read_text(errors="replace") if debug.is_file() else ""


def _players(output):
    """each player's walk (world units), kills, deaths and machine at the end"""
    players, last = {}, {}
    for line in output.splitlines():
        if "network test: tick" not in line:
            continue
        for match in PLAYER.finditer(line):
            index = int(match.group(1))
            player = players.setdefault(index, {"walked": 0.0})
            player.update(kills=int(match.group(4)), deaths=int(match.group(5)), machine=int(match.group(6)))
            if match.group(2) is None:
                last.pop(index, None)
                continue
            point = (float(match.group(2)), float(match.group(3)))
            # (a step of more than 10 units in a second is a respawn or a teleporter)
            if index in last and math.dist(last[index], point) < 10.0:
                player["walked"] += math.dist(last[index], point)
            last[index] = point
    return players


@pytest.mark.parametrize("map_name,variant,count,skill", [
    ("bloodgulch", "slayer", 6, "normal"),
    ("chillout", "team_slayer", 5, "heroic"),
    ("bloodgulch", "slayer", 6, "veteran"),
])
def test_bots_play_a_local_slayer_game(tmp_path, map_name, variant, count, skill):
    _game(map_name)
    status, output, debug = _play(tmp_path, f"local:{map_name}:{variant}", 75, HALO_BOTS=count, HALO_BOT_SKILL=skill)
    assert status == 0, output[-2000:]
    assert not re.search(r"assert|exception", output + debug, re.I), "an assert or exception in the game's log"
    bots = {index: player for index, player in _players(output).items() if player["machine"] >= BOTS_FIRST_MACHINE}
    assert len(bots) == count, f"{len(bots)} bots in the game, not {count}"
    for index, bot in bots.items():
        assert bot["walked"] >= 30.0, f"bot {index} walked {bot['walked']:.0f} units"
    kills = sum(bot["kills"] for bot in bots.values())
    deaths = sum(bot["deaths"] for bot in bots.values())
    assert kills >= count // 2 and deaths > 0, f"the bots killed {kills} and died {deaths} times"
    assert "bots: tick" in output, "no bots line a minute into the game"
    assert "bots: fights since" in output, "no line of the fight's counts after the bots line"


def test_no_bots_in_a_system_link_game(tmp_path):
    _game("bloodgulch")
    status, output, _ = _play(tmp_path, "host:bloodgulch:slayer", 30, HALO_BOTS=6)
    assert status == 0, output[-2000:]
    players = _players(output)
    assert players, "the system link game did not start"
    assert all(player["machine"] < BOTS_FIRST_MACHINE for player in players.values()), "a bot in a system link game"
    assert not re.search(r"\bbots: ", output), "the bots ran in a system link game"
