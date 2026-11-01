"""Run two headless Windows instances against private, locally supplied maps.

Checks authoritative policy on both peers, including a late join and co-op.
Uses isolated data/settings/save directories and loopback only. No game assets
are committed. Only processes started by this script are terminated.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time


def run_case(binary, maps, output, name, policy, preferences, coop=False, late=False, combat=False, latency=0, saves=None):
    jobs = []
    results = []
    expected_policy = {"player_choice": 0, "forced_on": 1, "forced_off": 2}[policy]
    try:
        for index, role in enumerate(("host", "client")):
            folder = output / name / role
            (folder / "maps").mkdir(parents=True)
            if saves:
                shutil.copytree(saves, folder / "saves")
            for filename in ("halo.exe", "SDL3.dll"):
                shutil.copy2(binary.parent / filename, folder / filename)
            for filename in ("ui.map", "carousel.map", "bloodgulch.map", *(["a30.map"] if coop else [])):
                shutil.copy2(maps / filename, folder / "maps" / filename)
            (folder / "config.toml").write_text(f"[display]\nthird_person = {'true' if preferences[index] else 'false'}\nfov = {140 if index else 70}\n")
            env = {k: v for k, v in os.environ.items() if not k.startswith("HALO_")}
            env.update(SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", HALO_NULL_RENDERER="1",
                       HALO_HIDDEN_WINDOW="1", HALO_NO_AUDIO="1", HALO_NET_ONLINE="0",
                       HALO_NET_PUBLIC_LOBBY="0", HALO_NET_ALLOW_UPNP="0", HALO_MAX_FPS="30",
                       HALO_NETWORK_TEST_PUBLIC="0", HALO_NET_JOIN_FROM_CLIPBOARD="0",
                       HALO_NETWORK_TEST=("host:levels\\a30\\a30:coop" if coop else "host:levels\\test\\bloodgulch\\bloodgulch:slayer") if index == 0 else "join",
                       HALO_NETWORK_TEST_START="10", HALO_NET_ADDRESS=f"127.0.0.{236+index}",
                       HALO_NET_BROADCAST=f"127.0.0.{237-index}", HALO_DATA_ROOT=str(folder),
                       HALO_SAVE_ROOT=str(folder / "saves"), HALO_UPDATE_ANSWER="never",
                       # An opposing client's host preference must have no authority.
                       HALO_SESSION_COMPATIBILITY="gulchce_third_person",
                       HALO_CAMERA_POLICY=policy if index == 0 else ("forced_off" if policy != "forced_off" else "forced_on"))
            if index and late:
                time.sleep(20)
            if combat:
                env.update(HALO_TEST_INPUT=f"bot:{index}", HALO_NETWORK_LATENCY=str(latency),
                           HALO_NETWORK_LOSS="5" if latency else "0")
            process = subprocess.Popen([str(folder / "halo.exe")], cwd=folder, env=env,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            jobs.append((role, folder, process))
        deadline = time.monotonic() + 45
        while time.monotonic() < deadline:
            ready = []
            for role, folder, process in jobs:
                log = (folder / "halo.log").read_text(errors="replace") if (folder / "halo.log").exists() else ""
                records = [line for line in log.splitlines() if "camera-policy" in line and "player 1:" in line]
                ready.append(len(records) >= (20 if combat else 5))
            if all(ready) or any(p.poll() is not None for _, _, p in jobs):
                break
            time.sleep(.5)
    finally:
        for index, (role, folder, process) in enumerate(jobs):
            early_exit = process.poll()
            if early_exit is None:
                process.terminate()
                process.wait(timeout=10)
            log = (folder / "halo.log").read_text(errors="replace")
            records = [line for line in log.splitlines() if "camera-policy" in line and "player 1:" in line]
            values = [tuple(map(int, match)) for line in records
                      for match in re.findall(r"camera-policy (\d+) preference (\d+) effective (\d+) active (\d+)", line)]
            expected = (expected_policy, preferences[index], 1 if expected_policy == 1 else 0 if expected_policy == 2 else preferences[index])
            saved = (folder / "config.toml").read_text()
            preference_preserved = f"third_person = {'true' if preferences[index] else 'false'}" in saved
            camera_observed = (not expected[2] and all(v[3] == 0 for v in values)) or (expected[2] and (coop or any(v[3] == 1 for v in values)))
            passed = early_exit is None and len(values) >= 5 and all(v[:3] == expected for v in values) and preference_preserved and camera_observed
            muzzle_players = sorted(set(int(p) for p in re.findall(r"network muzzle: player (\d+)", log)))
            if combat and not coop:
                # Player indices are assigned by join order, not machine order.
                machines = {int(p): int(m) for line in records for p, m in
                            re.findall(r"player (\d+):.*?\bm(\d+)(?= player | \|)", line)}
                expected_shooters = sorted(p for p, m in machines.items()
                                           if expected_policy == 1 or (expected_policy == 0 and preferences[m]))
                passed = passed and muzzle_players == expected_shooters
            result = dict(case=name, role=role, passed=passed, samples=len(values), expected=expected,
                          observed=sorted(set(values)), preference_preserved=preference_preserved,
                          early_exit=early_exit)
            result.update(muzzle_players=muzzle_players, combat=combat, latency=latency)
            results.append(result)
            print(json.dumps(result), flush=True)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--maps", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--saves", type=Path, help="Copy a preselected profile into each isolated save folder")
    parser.add_argument("--combat", action="store_true", help="Fire bot weapons and check muzzle decisions on both peers")
    args = parser.parse_args()
    results = []
    for name, policy, preferences, coop, late in (
        ("choice", "player_choice", (0, 1), False, False),
        ("on", "forced_on", (0, 0), False, False),
        ("off", "forced_off", (1, 1), False, False),
        ("late", "forced_on", (0, 0), False, True),
        ("coop", "forced_on", (0, 0), True, False),
    ):
        print("Starting " + name, flush=True)
        results += run_case(args.binary.resolve(), args.maps.resolve(), args.output.resolve(), name, policy, preferences, coop, late, args.combat, saves=args.saves)
    if args.combat:
        results += run_case(args.binary.resolve(), args.maps.resolve(), args.output.resolve(), "latency", "forced_on", (0, 0), combat=True, latency=100, saves=args.saves)
    (args.output / "results.json").write_text(json.dumps(results, indent=2))
    return 0 if len(results) == (12 if args.combat else 10) and all(row["passed"] for row in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
