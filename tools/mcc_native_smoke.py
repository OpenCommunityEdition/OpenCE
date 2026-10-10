"""Bounded Windows MCC startup checks using locally supplied, private test assets.

Use a disposable data root containing maps/ui.map and mcc_maps/*.map (plus
matching resources where required). Never commit the assets or generated logs.
Only one hidden game runs at a time. Success requires a console expression to
finish after 150 simulation ticks, not just the tag-loader's 'loaded' message.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import time
import uuid


FAILURES = ("recompile scripts", "recompile.)", "error loading scripts",
            "scripts won't run", "unsupported script", "invalid script",
            "mcc: refused", "exception halt", "exception assert", "assertion failed",
            "failed to allocate", "could not load", "invalid mcc script parameter",
            "this is not a valid object name")


def check_result(trace, console, marker, map_name, exit_code):
    combined = (trace + "\n" + console).lower()
    failures = [message for message in FAILURES if message in combined]
    loaded = f"mcc: loaded mcc_maps\\{map_name.lower()} (" in trace.lower()
    # The console echoes commands; an echoed print expression is not evidence
    # it executed. Require the entire output line after the sleep has finished.
    advanced = marker.lower() in {line.strip().lower() for line in console.splitlines()}
    return dict(map=map_name, loaded=loaded, simulation_advanced=advanced,
                failures=failures, exit_code=exit_code,
                passed=loaded and advanced and exit_code == 0 and not failures)


def run_case(args, name, binary):
    started = time.monotonic()
    data, output = args.data_root, args.output / name
    output.mkdir(parents=True, exist_ok=True)
    init = data / "init.txt"
    original = init.read_bytes() if init.exists() else None
    debug = data / "debug.txt"
    offset = debug.stat().st_size if debug.exists() else 0
    marker = "mcc_smoke_" + uuid.uuid4().hex
    env = {key: value for key, value in os.environ.items() if not key.startswith("HALO_")}
    env.update(HALO_DATA_ROOT=str(data), HALO_SAVE_ROOT=str(output / "saves"),
               HALO_HIDDEN_WINDOW="1", HALO_EXIT_AFTER=str(args.seconds),
               HALO_NULL_RENDERER="0", HALO_MAX_FPS="30", HALO_NO_VSYNC="1", HALO_NO_AUDIO="1",
               HALO_DISPLAY_MODE="windowed", HALO_WINDOW_SIZE="640x480", HALO_FULLSCREEN="0",
               HALO_NET_ONLINE="0", HALO_NET_PUBLIC_LOBBY="0", HALO_NET_ALLOW_UPNP="0",
               HALO_NET_ADDRESS="127.0.0.236", HALO_NET_BROADCAST="127.0.0.237",
               HALO_NET_JOIN_FROM_CLIPBOARD="0", HALO_UPDATE_ANSWER="never", HALO_CRASH_REPORTS="no",
               HALO_TELNET_CONSOLE="1", HALO_TELNET_CONSOLE_PORT=str(args.port))
    transcript = ""
    process = connection = None
    def trace():
        if not debug.exists():
            return ""
        with debug.open("rb") as stream:
            stream.seek(offset)
            return stream.read().decode("latin1")
    try:
        init.write_text(f"map_name mcc_maps\\{name}\n", encoding="ascii")
        with (output / "stdout.log").open("wb") as stdout, (output / "stderr.log").open("wb") as stderr:
            process = subprocess.Popen([str(binary)], cwd=data, env=env, stdout=stdout, stderr=stderr,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            deadline = time.monotonic() + args.seconds + 60
            sent = False
            while process.poll() is None and time.monotonic() < deadline:
                current = trace()
                if any(message in current.lower() for message in FAILURES):
                    break
                if connection is None and f"mcc: loaded mcc_maps\\{name.lower()} (" in current.lower():
                    try:
                        connection = socket.create_connection(("127.0.0.1", args.port), .2)
                        connection.settimeout(.1)
                    except OSError:
                        connection = None
                if connection is not None:
                    try:
                        if not sent:
                            connection.sendall(f'(begin (sleep 150) (print "{marker}"))\r\n'.encode("ascii"))
                            sent = True
                        received = connection.recv(65536)
                        if received:
                            transcript += received.decode("latin1")
                    except socket.timeout:
                        pass
                    except OSError:
                        connection.close()
                        connection = None
                time.sleep(.1)
    finally:
        if connection is not None:
            connection.close()
        if process is not None and process.poll() is None:
            process.kill()
            process.wait(timeout=10)
        if original is None:
            init.unlink(missing_ok=True)
        else:
            init.write_bytes(original)
        (output / "debug.log").write_text(trace(), encoding="utf-8")
        (output / "console.log").write_text(transcript, encoding="utf-8")
    diagnostics = trace() + (output / "stderr.log").read_text(errors="replace")
    result = check_result(diagnostics, transcript, marker, name, process.returncode)
    result["elapsed_seconds"] = round(time.monotonic() - started, 2)
    result["executable_sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--data-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seconds", type=int, default=45)
    parser.add_argument("--port", type=int, default=23251)
    parser.add_argument("maps", nargs="+")
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("this launcher currently supports Windows only")
    if args.seconds < 20 or args.seconds > 300:
        parser.error("--seconds must be between 20 and 300")
    for name in args.maps:
        if not re.fullmatch(r"[A-Za-z0-9_-]+", name) or not (args.data_root / "mcc_maps" / (name + ".map")).is_file():
            parser.error("supply existing map basenames from the disposable data root")
    args.data_root, args.output = args.data_root.resolve(), args.output.resolve()
    binary = args.output / "bin" / "halo.exe"
    binary.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.executable, binary)
    shutil.copy2(args.executable.parent / "SDL3.dll", binary.parent / "SDL3.dll")
    results = []
    for name in args.maps:
        print(f"{name}: hidden startup and simulation check", flush=True)
        result = run_case(args, name, binary)
        results.append(result)
        print(json.dumps(result), flush=True)
        (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return 0 if all(result["passed"] for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
