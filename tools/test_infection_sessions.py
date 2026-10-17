"""Opt-in local sessions: built executable and legally supplied game maps.

Set OPENCE_TEST_EXE, OPENCE_TEST_MAPS and OPENCE_TEST_RUN_ROOT. Each process
gets a separate data/save directory and its own log. Nothing is downloaded.
"""
import os
from pathlib import Path
import shutil
import subprocess
import time
import re

import pytest


def run_session(exe, maps, root, duration=65, loss=0, latency=0, late_join=0,
                variants="infection", kill=0, shoot=0, drop=False, map_name="ratrace", objective=False,
                render=False):
    processes, logs = [], []
    root.mkdir(parents=True, exist_ok=False)
    try:
        for index, role in enumerate(("host:" + map_name + ":" + variants, "join", "join")):
            if index == 1:
                time.sleep(2)
            if index == 2 and late_join:
                time.sleep(late_join)
            work = root / str(index)
            (work / "maps").mkdir(parents=True)
            for name in dict.fromkeys(("ui.map", map_name + ".map", "carousel.map")):
                shutil.copy2(maps / name, work / "maps" / name)
            shutil.copy2(exe, work / exe.name)
            shutil.copy2(exe.with_name("SDL3.dll"), work / "SDL3.dll")
            # The null renderer has no OpenGL context for high-res HUD textures.
            (work / "config.toml").write_text('[crash_reports]\nupload = "no"\n[discord]\napplication_id = ""\n'
                                            '[display]\nhigh_res_hud = false\nhigh_res_text = false\n')
            env = {k: v for k, v in os.environ.items() if not k.startswith('HALO_')}
            env.update(HALO_DATA_ROOT=str(work), HALO_SAVE_ROOT=str(work / "saves"),
                       HALO_NETWORK_TEST=role, HALO_NETWORK_TEST_START="12",
                       HALO_NET_ADDRESS=f"127.0.0.{index + 11}", HALO_NET_BROADCAST="127.0.0.11,127.0.0.12,127.0.0.13",
                       HALO_NET_ONLINE="false", HALO_TEST_INPUT="look:objective" if objective else ("look:drop" if drop else f"look:{index + 1}"),
                       HALO_NETWORK_TEST_KILL=str(kill),
                       HALO_NETWORK_TEST_SHOOT=str(shoot),
                       HALO_NETWORK_TEST_PICKUP="20" if objective else "0", HALO_NETWORK_TEST_PICKUP_WEAPON="ball" if objective else "",
                       HALO_HIDDEN_WINDOW="1", HALO_NULL_RENDERER="0" if render else "1", HALO_EXIT_AFTER=str(duration),
                       HALO_NETWORK_LATENCY=str(latency), HALO_NETWORK_LOSS=str(loss))
            if render:
                (work / 'frames').mkdir()
                env.update(HALO_DISPLAY_MODE='windowed', HALO_WINDOW_SIZE=os.environ.get('OPENCE_TEST_RESOLUTION', '800x600'),
                           HALO_SCREENSHOT_DIR=str(work / 'frames'), HALO_SCREENSHOT_EVERY='300')
            log = (work / "session.log").open("wb")
            logs.append(log)
            processes.append(subprocess.Popen([str(work / exe.name)], cwd=work, env=env,
                             stdout=log, stderr=subprocess.STDOUT,
                             creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0))
        # null_renderer creates no SDL window, so the engine's window-based
        # exit_after does not run. Bound only our own test processes here.
        deadline = time.monotonic() + duration
        while time.monotonic() < deadline and all(p.poll() is None for p in processes):
            time.sleep(0.5)
        for process in processes:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=15)
        return [p.returncode for p in processes]
    finally:
        for process in processes:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=15)
        for log in logs:
            log.close()


@pytest.mark.skipif(not all(os.environ.get(k) for k in
                    ("OPENCE_TEST_EXE", "OPENCE_TEST_MAPS", "OPENCE_TEST_RUN_ROOT")),
                    reason="requires an OpenCE build, maps, and an isolated run directory")
def test_three_machine_infection():
    root = Path(os.environ["OPENCE_TEST_RUN_ROOT"])
    scenario = os.environ.get("OPENCE_TEST_SCENARIO", "infection")
    survival = scenario == "survival"
    late = scenario == "late"
    melee = scenario in ("melee", "drop-melee")
    tuned = scenario == "tuned"
    drop = scenario in ("drop", "drop-melee")
    objective = scenario == "oddball"
    variants = {"survival": "infection-survival,slayer", "tuned": "infection-tuned,slayer",
                "oddball": "oddball"}.get(scenario, "infection,slayer")
    run_session(Path(os.environ["OPENCE_TEST_EXE"]), Path(os.environ["OPENCE_TEST_MAPS"]), root,
                         duration=135 if survival else (60 if objective else 80), variants=variants,
                         kill=0 if survival or melee or objective else (40 if late else 10), late_join=30 if late else 0,
                         loss=5 if late else 0, latency=150 if late else 0, shoot=1 if melee else 0,
                         drop=drop, map_name=os.environ.get("OPENCE_TEST_MAP", "ratrace"), objective=objective,
                         render=os.environ.get('OPENCE_TEST_RENDER') == '1')
    host = (root / "0/session.log").read_text(errors="replace")
    if objective:
        for index in range(3):
            log = (root / str(index) / 'session.log').read_text(errors='replace')
            assert 'infection state:' not in log and 'ASSERTION FAILED' not in log
            assert 'crash: exception' not in log
            assert 'EXCEPTION' not in (root / str(index) / 'debug.txt').read_text(errors='replace')
            assert_oddball_pickup_score_drop(log)
        return
    assert f"infection: active players={2 if late else 3} zombies=1" in host
    assert "infection: cancelled" not in host
    for index in range(3):
        log = (root / str(index) / "session.log").read_text(errors="replace")
        assert "infection state:" in log
        assert "ASSERTION FAILED" not in log
        assert "crash: exception" not in log
        assert "EXCEPTION" not in (root / str(index) / "debug.txt").read_text(errors="replace")
        assert_zombie_inventory(log)
        if drop:
            assert_drop_controls(log, index, require_local=not melee)
        if tuned:
            assert 'infection options: initial=1 speed=125 damage=150 respawn=3 countdown=3 minutes=5' in log
            assert 'ready0 speed1.25' in log
        assert f'phase=3 humans={"2 zombies=1" if survival else "0 zombies=3"}' in log
        assert f'winner={0 if survival else 1}' in log
        after_infection = log.rsplit('infection state:', 1)[1]
        assert any('network test: tick' in line and 'playing' in line and 'ready' not in line and 'g4/0' in line
                   for line in after_infection.splitlines()), 'peer never resumed normal Slayer equipment'
    assert 'network test: game 2, slayer' in host
    slayer = host.split('network test: game 2, slayer', 1)[1]
    assert 'infection state:' not in slayer
    if drop:
        probes = re.findall(r'infection human skull probe: pickup=(\d+) ready=(\d+)', host)
        assert len(probes) >= 3 and all(p == ('0', '0') for p in probes), 'human could pick up or ready a skull'
    assert re.search(r'network test: tick (?:[3-9]\d|\d{3,}) player 0:', slayer), 'Slayer never played'
    if melee:
        assert 'infection test: damage' not in host, 'melee scenario used forced deaths'
        infection_log = host.split('network test: game 2, slayer', 1)[0]
        assert re.search(r'\bdealt [1-9]\d*', infection_log), 'host never accepted remote damage during Infection'
        assert 'infection: infected slot=' in infection_log
    if late:
        assert re.search(r'infection: joined slot=\d+ role=2 phase=2', host), 'late join was not a zombie'
        assert_infection_deaths(host)
    if not survival and not melee and not late:
        assert_infection_deaths(host)
        deaths = list(re.finditer(r'infection test: damage victim=(\d+)', host))
        assert len(deaths) >= 3, 'missing human/zombie/human death sequence'
        victim = deaths[1].group(1)
        after = host[deaths[1].end():deaths[2].start()]
        assert re.search(r'player ' + victim + r': dead[^\n]*\bd[1-9]', after)
        assert re.search(r'player ' + victim + r': \([^\n]*?g0/0[^\n]*?\bd[1-9]\d*\b[^\n]*?t1', after), 'zombie did not respawn'
        if tuned:
            samples = [(int(tick), body) for tick, body in re.findall(
                r'network test: tick (\d+)[^\n]*?player ' + victim + r': (.*?)(?= player | \| items)', after)]
            dead_tick = next(tick for tick, body in samples if body.startswith('dead'))
            alive_tick = next(tick for tick, body in samples if tick > dead_tick and not body.startswith('dead'))
            assert 60 <= alive_tick - dead_tick <= 120, 'three-second respawn not observed'


def assert_zombie_inventory(log):
    checked = 0
    readied = 0
    in_infection = False
    for line in log.splitlines():
        if 'infection state:' in line:
            in_infection = True
            continue
        if 'network test: tick' not in line:
            continue
        if not in_infection:
            continue
        in_infection = False
        for player in re.split(r' player \d+:', line)[1:]:
            if re.search(r'\bt1\b', player) and 'dead' not in player:
                weapons = re.findall(r'\b[0-9a-f]+:\d+\b', player.split('|')[0])
                assert len(weapons) == 1, f'zombie has {len(weapons)} weapons: {weapons}'
                assert 'g0/0' in player, 'zombie has grenades'
                if re.search(r'\bready[0-3]\b', player):
                    readied += 1
                checked += 1
    assert checked, 'no living zombie observed'
    assert readied, 'no zombie ever readied its skull'


def assert_drop_controls(log, peer, require_local=True):
    """Observe the real input stream and stable weapon datums during each life."""
    held, attempted, local_attempts = {}, 0, 0
    pending_ready = {}
    active = False
    skull_tag = None
    for line in log.splitlines():
        if 'infection state:' in line:
            active = 'phase=2 ' in line
        if not active:
            continue
        ground = re.search(r'infection drop test: ground_skulls=(\d+) ball=([0-9a-f]+)', line)
        if ground:
            # Host deletes a corpse's skull on its next player tick. Samples
            # are one second apart; uncollected or duplicate skulls must not persist.
            assert ground.group(1) == '0', 'an orphaned skull remained on the ground'
            skull_tag = ground.group(2)
        for player, body in re.findall(r' player (\d+): (.*?)(?= player | \| items)', line):
            if not re.search(r'\bt1\b', body) or 'dead' in body:
                continue
            controls = re.search(r'controls([0-9a-f]+) held([0-9a-f]+)', body)
            if not controls or not int(controls.group(1), 16) & 0x1800:
                continue
            if skull_tag is not None:
                assert re.findall(r'\b([0-9a-f]+):\d+\b', body) == [skull_tag]
            life = re.search(r'\bd(\d+)\b', body).group(1)
            key = (player, life)
            tick = int(re.search(r'network test: tick (\d+)', line).group(1))
            if controls.group(2) == 'ffffffff':
                # A remote inventory packet sets desired_weapon_index first;
                # its draw animation can still be active at the first sample.
                # Allow only this initial, remote, skull-present transition.
                # Losing a previously readied skull or failing to recover on
                # the very next one-second sample remains a hard failure.
                assert int(player) != peer and key not in held and key not in pending_ready
                assert skull_tag is not None and 'ready-1 ' in body and re.search(r'\bst33\b', body)
                pending_ready[key] = tick
                continue
            assert re.search(r'\bready[0-3]\b', body)
            if key in pending_ready:
                assert 0 < tick - pending_ready.pop(key) <= 30, 'remote skull did not ready in one sample'
            assert held.setdefault(key, controls.group(2)) == controls.group(2), 'skull was replaced while dropping'
            attempted += 1
            if int(player) == peer:
                local_attempts += 1
    assert not pending_ready, 'remote skull never readied'
    assert attempted >= 3, 'no sustained trigger attempts were observed'
    # The deterministic alpha is remote in ratrace. Both joining clients
    # become zombies early enough to exercise their own input prediction.
    if peer and require_local:
        assert local_attempts >= 3, 'remote client did not attempt to drop its skull'


def assert_oddball_pickup_score_drop(log):
    pickup = re.search(r'last player stands on weapon [0-9a-f]+ \(([0-9a-f]+)\)', log)
    assert pickup, 'no Oddball pickup attempt'
    tag = format(int(pickup.group(1), 16) & 65535, 'x')
    samples = re.findall(r'network test: tick (\d+)[^\n]* player 2: (.*?)(?= \| items)', log)
    carrying = [(int(t), int(re.search(r'\bs(\d+)\b', body).group(1)))
                for t, body in samples if re.search(r'\b' + tag + r':\d+\b', body)]
    assert any(score > 0 for _, score in carrying), 'Oddball never awarded possession time'
    final_tick, final_score = carrying[-1]
    after = [(int(t), body) for t, body in samples if int(t) > final_tick]
    assert len(after) >= 3 and final_tick >= 720, 'objective drop was not observed'
    assert all(not re.search(r'\b' + tag + r':\d+\b', body) and 'dead' not in body for _, body in after)
    assert all(int(re.search(r'\bs(\d+)\b', body).group(1)) >= final_score for _, body in after)
    settled_scores = [int(re.search(r'\bs(\d+)\b', body).group(1)) for _, body in after[2:]]
    assert len(set(settled_scores)) == 1, 'Oddball kept scoring after dropping the objective'


def assert_infection_deaths(log):
    assert 'infection: infected slot=' in log, 'no actual human death reached Infection'
    assert re.search(r'\bd[1-9]\d*\b', log), 'no death recorded in player statistics'
