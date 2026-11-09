#!/usr/bin/env python3
"""Starts a built game for a few seconds, headless, and checks its log.

The game is started with no window or sound (SDL's dummy drivers), no game
data and no update check, in an empty data folder; it logs its start-up and
then waits for the disc image it does not find. The check passes when the
log has every --require text and none of the --forbid texts, and the game did
not die from a signal before the time ran out (it is stopped then).

    python3 tools/ci_smoke.py --seconds 60 -- build/linux/halo

What it catches is what a link or a unit test cannot: a program that does not
load (a library or loader that is not found), crashes in a start-up
constructor, or fails to reserve the memory it needs (xbox_memory.c).
"""

import argparse
import os
import signal
import subprocess
import sys
import tempfile
import time

DEFAULT_REQUIRE = ["halo-linux: settings:", "halo-linux: data root:"]
DEFAULT_FORBID = ["cannot reserve"]


def run(command, seconds, require, forbid, extra_env=None):
    """Returns (ok, output, problems) for one start of the game"""
    if os.sep in command[0]:
        command = [os.path.abspath(command[0])] + list(command[1:])  # (it runs in another folder)
    with tempfile.TemporaryDirectory() as folder:
        env = dict(os.environ)
        env.update({
            "SDL_VIDEODRIVER": "dummy",
            "SDL_AUDIODRIVER": "dummy",
            "HALO_UPDATE_AUTO": "false",
            "HALO_DATA_ROOT": folder,
            "HALO_CONFIG_FILE": os.path.join(folder, "config.toml"),
        })
        env.update(extra_env or {})
        process = subprocess.Popen(command, cwd=folder, env=env, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, start_new_session=True)
        died = None
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            if process.poll() is not None:
                code = process.returncode
                if code < 0 or code >= 128:
                    died = f"the game died with {describe(code)}"
                break
            time.sleep(0.2)
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
        output = process.communicate()[0].decode("utf-8", "replace")
    problems = []
    if died:
        problems.append(died)
    for text in require:
        if text not in output:
            problems.append(f"the log has no \"{text}\"")
    for text in forbid:
        if text in output:
            problems.append(f"the log has \"{text}\"")
    return not problems, output, problems


def describe(code):
    number = -code if code < 0 else code - 128
    try:
        return f"signal {number} ({signal.Signals(number).name})"
    except ValueError:
        return f"signal {number}"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--seconds", type=float, default=60, help="how long the game runs (default 60)")
    parser.add_argument("--no-defaults", action="store_true",
                        help="check only the --require and --forbid texts, for a program that does not log "
                        "the game's own start-up lines")
    parser.add_argument("--require", action="append", help="text the log must have (adds to the defaults)")
    parser.add_argument("--forbid", action="append", help="text the log must not have (adds to the defaults)")
    parser.add_argument("--env", action="append", default=[], metavar="NAME=VALUE", help="an environment variable")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="-- and the command")
    args = parser.parse_args(argv)
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("no command")
    extra_env = dict(item.split("=", 1) for item in args.env)
    require = ([] if args.no_defaults else DEFAULT_REQUIRE) + (args.require or [])
    forbid = ([] if args.no_defaults else DEFAULT_FORBID) + (args.forbid or [])
    ok, output, problems = run(command, args.seconds, require, forbid, extra_env)
    sys.stdout.write(output)
    if not ok:
        for problem in problems:
            print(f"ci_smoke: {problem}", file=sys.stderr)
        return 1
    print("ci_smoke: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
