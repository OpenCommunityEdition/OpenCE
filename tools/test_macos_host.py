"""The macOS host running the guest image in its VM (docs/macos_host.md).

After `ninja macos`: the host loads build/macos/halo_guest.elf, the guest's
main runs, logs through its own standard error (port/linux/src/xbox_kernel.c's
platform_log, "halo-linux: ...", which reaches the host as a write system
call), and goes on through its start-up to look for the maps: with no game
data, the game stops itself there (its debug.txt says "no valid map
directory exists") and the guest ends the process through exit, not by a
crash. With the game data in assets/maps, a run under debug.exit_after
ends with status 0, and one under debug.network_test hosts a game on
bloodgulch and logs its player's state each second. With
HALO_HOST_IMPORT_COUNTS set, that run reports its VM exits per second,
in all and by import: the baseline for the renderer's batching. Two
copies on loopback addresses of their own, one hosting and one joining,
each log the other's player; macOS has only 127.0.0.1 on lo0, so this
needs aliases (`sudo ifconfig lo0 alias 127.0.0.2 up`, and 127.0.0.3).

The runs never touch the home folder: HOME and XDG_DATA_HOME (where the
platform layer keeps saves) are the test's own.

Each run is headless: debug.null_renderer, debug.hidden_window, and SDL's
dummy video and audio drivers.
"""

import os
import platform
import re
import socket
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
HOST = ROOT / "build/macos/halo_host"
IMAGE = ROOT / "build/macos/halo_guest.elf"
MAPS = ROOT / "assets/maps"

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not (HOST.is_file() and IMAGE.is_file()), reason="needs `ninja macos`"),
]


def host_environment(data_root, home, exit_after, extra=None):
    environment = dict(os.environ)
    (home / "saves").mkdir(parents=True, exist_ok=True)
    environment.update(
        {
            "HOME": str(home),
            "XDG_DATA_HOME": str(home / "saves"),
            "HALO_DATA_ROOT": str(data_root),
            "HALO_EXIT_AFTER": str(exit_after),
            "HALO_NULL_RENDERER": "1",
            "HALO_HIDDEN_WINDOW": "1",
            "SDL_VIDEO_DRIVER": "dummy",
            "SDL_AUDIO_DRIVER": "dummy",
            **(extra or {}),
        }
    )
    return environment


def run_host(data_root, home, exit_after=2, timeout=120, extra=None):
    return subprocess.run(
        [str(HOST), str(IMAGE)],
        cwd=data_root,
        env=host_environment(data_root, home, exit_after, extra),
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def test_the_guest_runs_its_main_and_exits_by_itself(tmp_path):
    data_root = tmp_path / "data"
    data_root.mkdir()
    result = run_host(data_root, tmp_path / "home")
    output = result.stdout + result.stderr
    assert "halo_host: guest image 88000000-" in output, output[-4000:]
    assert "halo-linux: " in output, output[-4000:]
    assert "halo_host: the guest exited with status" in output, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]
    log = (data_root / "debug.txt").read_text(errors="replace")
    assert "no valid map directory exists" in log, log[-4000:]
    # its settings and saves stayed where they were told to be
    assert (data_root / "config.toml").is_file()
    assert (tmp_path / "home" / "saves" / "halo-linux").is_dir()


@pytest.mark.skipif(not (MAPS / "ui.map").is_file(), reason="needs the game data in assets/maps")
def test_a_run_with_the_game_data_ends_cleanly_after_exit_after(tmp_path):
    result = run_host(game_data(tmp_path), tmp_path / "home", exit_after=5)
    output = result.stdout + result.stderr
    assert result.returncode == 0, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]


def game_data(tmp_path):
    # (a data root of its own, so that debug.txt and config.toml are the
    # test's, with the maps linked into it)
    data_root = tmp_path / "data"
    data_root.mkdir()
    (data_root / "maps").symlink_to(MAPS)
    return data_root


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game data in assets/maps")
def test_a_hosted_network_game_plays_headless(tmp_path):
    result = run_host(
        game_data(tmp_path),
        tmp_path / "home",
        exit_after=30,
        timeout=180,
        extra={
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NET_ADDRESS": "127.0.0.1",
            "HALO_HOST_IMPORT_COUNTS": "1",
            "HALO_HOST_SAMPLE": "10",
        },
    )
    output = result.stdout + result.stderr
    assert result.returncode == 0, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]
    assert "network test: hosting bloodgulch" in output, output[-4000:]
    assert "network test: starting the game" in output, output[-4000:]
    # the local player joined its own game: its state each second
    ticks = re.findall(r"network test: tick \d+ player 0: \(", output)
    assert len(ticks) >= 5, output[-4000:]
    # the exits: "halo_host: exits: <count> in <seconds> s, <rate>/s", and
    # each import's "halo_host: import <name>: <count>, <rate>/s"
    exits = re.search(r"halo_host: exits: (\d+) in ([\d.]+) s, (\d+)/s", output)
    assert exits and int(exits[1]) > 0 and float(exits[2]) >= 25, output[-4000:]
    imports = re.findall(r"halo_host: import (\w+): (\d+), (\d+)/s", output)
    assert "host_syscall" in {name for name, _, _ in imports}, output[-4000:]
    assert sum(int(count) for _, count, _ in imports) <= int(exits[1])
    # the sampler (HALO_HOST_SAMPLE): where each vCPU's guest code was
    assert re.search(r"halo_host: sample: vCPU \d+ at 0x8[89ab][0-9a-f]{6}", output), output[-4000:]


def bindable(address):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe:
        try:
            probe.bind((address, 0))
        except OSError:
            return False
    return True


HOSTING, JOINING = "127.0.0.2", "127.0.0.3"


def two_copies(tmp_path, extra=None):
    """one copy hosting bloodgulch and one joining it, on loopback addresses
    of their own, with system link only; the output of each"""

    def start(name, test, address, other, exit_after):
        folder = tmp_path / name
        folder.mkdir()
        data_root = game_data(folder)
        environment = host_environment(
            data_root,
            folder / "home",
            exit_after,
            {
                "HALO_NETWORK_TEST": test,
                "HALO_NET_ADDRESS": address,
                "HALO_NET_BROADCAST": other,
                "HALO_NET_ONLINE": "false",
                "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
                **(extra or {}),
            },
        )
        return subprocess.Popen(
            [str(HOST), str(IMAGE)],
            cwd=data_root,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

    host = start("host", "host:bloodgulch", HOSTING, JOINING, 60)
    client = start("client", "join", JOINING, HOSTING, 50)
    try:
        client_output, _ = client.communicate(timeout=180)
        host_output, _ = host.communicate(timeout=180)
    finally:
        for process in (host, client):
            if process.poll() is None:
                process.kill()
    for process, output in ((host, host_output), (client, client_output)):
        assert process.returncode == 0, output[-4000:]
        assert "halo_host: guest crash" not in output, output[-4000:]
        # each logs both players: its own and the other machine's
        assert re.search(r"network test: tick \d+ player 1: \(", output), output[-4000:]
    return host_output, client_output


def two_copies_available():
    return (MAPS / "bloodgulch.map").is_file() and bindable(HOSTING) and bindable(JOINING)


TWO_COPIES = pytest.mark.skipif(
    not two_copies_available(),
    reason=(
        f"needs the game data in assets/maps, and {HOSTING} and {JOINING} on lo0 (sudo ifconfig lo0 alias <address> up)"
    ),
)
# game.ragdolls and game.physics_debris on: each machine's own, cosmetic
PHYSICS = {"HALO_RAGDOLLS": "true", "HALO_PHYSICS_DEBRIS": "true"}


@TWO_COPIES
def test_a_second_copy_joins_the_hosted_game(tmp_path):
    two_copies(tmp_path)


@TWO_COPIES
def test_a_second_copy_joins_with_ragdolls_and_debris_on(tmp_path):
    # (the bodies are each machine's own: nothing more is sent, so the two
    # play as they do with the settings off)
    two_copies(tmp_path, PHYSICS)


@pytest.mark.skipif(not (MAPS / "bloodgulch.map").is_file(), reason="needs the game data in assets/maps")
def test_a_hosted_game_plays_with_ragdolls_and_debris_on(tmp_path):
    # (one copy, on 127.0.0.1: debug.network_test_crowd's thirty bipeds shot
    # dead and two hundred casings thrown, with the host's physics stepping
    # them, headless)
    result = run_host(
        game_data(tmp_path),
        tmp_path / "home",
        exit_after=40,
        timeout=240,
        extra={
            "HALO_NETWORK_TEST": "host:bloodgulch",
            "HALO_NET_ADDRESS": "127.0.0.1",
            "HALO_NET_ONLINE": "false",
            "HALO_NET_JOIN_FROM_CLIPBOARD": "false",
            "HALO_NETWORK_TEST_CROWD": "15",
            "HALO_HOST_PHYSICS_LOG": "1",
            **PHYSICS,
        },
    )
    output = result.stdout + result.stderr
    assert result.returncode == 0, output[-4000:]
    assert "halo_host: guest crash" not in output, output[-4000:]
    assert len(re.findall(r"network test: tick \d+ player 0: \(", output)) >= 5, output[-4000:]
    crowd = re.search(r"the crowd: (\d+) of (\d+) bipeds dead, (\d+) casings thrown", output)
    assert crowd and crowd[1] == crowd[2] == "30" and crowd[3] == "200", output[-4000:]
    later = re.search(r"the crowd: (\d+) ragdolls, (\d+) casings left, (\d+) of them Jolt's bodies", output)
    assert later and int(later[1]) > 0 and int(later[3]) > 0, output[-4000:]
    steps = re.findall(r"halo_host: physics step: (\d+) refused, (\d+) ragdolls, (\d+) bodies", output)
    assert steps and max(int(ragdolls) for _, ragdolls, _ in steps) >= 30, output[-4000:]
    assert all(refused == "0" for refused, _, _ in steps), output[-4000:]


def test_the_network_version_is_unchanged():
    """the settings change nothing the machines send each other: the version is main's"""
    pattern = r"#define HALO_PORT_NETWORK_VERSION (\d+)"
    here = re.search(pattern, (ROOT / "port/linux/include/halo_port_limits.h").read_text())[1]
    main = subprocess.run(
        ["git", "show", "main:port/linux/include/halo_port_limits.h"], cwd=ROOT, capture_output=True, text=True
    )
    if main.returncode != 0:
        pytest.skip("no main branch to compare with")
    assert re.search(pattern, main.stdout)[1] == here
