"""Bounded Windows MCC startup checks using locally supplied, private test assets.

Use a disposable data root containing maps/ui.map. Local levels use mcc_maps;
--workshop accepts installed numeric-package/map names without copying maps.
--steam-root selects a Steam library and --campaigns enables mission discovery.
Never commit assets or generated logs. The data root's init.txt is restored.
Each case retains isolated saves/logs; generated map caches are removed only
after child exit. Uncertain shutdown also retains the copied executable.
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
import stat
import subprocess
import tempfile
import time
import uuid


FAILURES = ("recompile scripts", "recompile.)", "error loading scripts",
            "scripts won't run", "unsupported script", "invalid script",
            "mcc: refused", "exception halt", "exception assert", "assertion failed",
            "failed to allocate", "could not load", "invalid mcc script parameter",
            "this is not a valid object name", "i expected a value of type",
            "create_transport_endpoint() failed on telnet console endpoint",
            "bind_endpoint() failed on telnet console endpoint",
            "listen_endpoint() failed on telnet console endpoint")


def check_result(trace, console, marker, map_name, exit_code):
    combined = (trace + "\n" + console).lower()
    failures = [message for message in FAILURES if message in combined]
    map_name = map_name.replace("/", "\\")
    loaded = f"mcc: loaded mcc_maps\\{map_name.lower()} (" in trace.lower()
    # The console echoes commands; an echoed print expression is not evidence
    # it executed. Require the entire output line after the sleep has finished.
    advanced = marker.lower() in {line.strip().lower() for line in console.splitlines()}
    return dict(map=map_name, loaded=loaded, simulation_advanced=advanced,
                failures=failures, exit_code=exit_code,
                passed=loaded and advanced and exit_code == 0 and not failures)


def case_environment(args, output):
    env = {key: value for key, value in os.environ.items() if not key.startswith("HALO_")}
    env.update(HALO_DATA_ROOT=str(args.data_root), HALO_SAVE_ROOT=str(output / "saves"),
               HALO_LOG_ROOT=str(output),
               HALO_HIDDEN_WINDOW="1", HALO_EXIT_AFTER=str(args.seconds),
               HALO_MAX_FPS="30", HALO_NO_VSYNC="1", HALO_NO_AUDIO="1",
               HALO_DISPLAY_MODE="windowed", HALO_WINDOW_SIZE="640x480", HALO_FULLSCREEN="0",
               HALO_NET_ONLINE="0", HALO_NET_PUBLIC_LOBBY="0", HALO_NET_ALLOW_UPNP="0",
               HALO_NET_ADDRESS="127.0.0.236", HALO_NET_BROADCAST="127.0.0.237",
               HALO_NET_JOIN_FROM_CLIPBOARD="0", HALO_UPDATE_AUTO="0", HALO_CRASH_REPORTS="no",
               HALO_CONSOLE_LOG="all", HALO_TELNET_CONSOLE="1", HALO_TELNET_CONSOLE_PORT=str(args.port),
               HALO_WORKSHOP_MAPS=str(int(args.workshop)), HALO_MCC_CAMPAIGNS=str(int(args.campaigns)),
               HALO_STEAM_ROOT=str(args.steam_root) if args.steam_root else "")
    # HALO_NULL_RENDERER is presence-only: even "0" disables rendering.
    return env


def linked(status):
    return stat.S_ISLNK(status.st_mode) or getattr(status, "st_file_attributes", 0) & 0x400


def cleanup_map_caches(output):
    """Only this fresh case's z/cacheNNN.map files; never traverse links."""
    report = dict(removed_files=0, removed_bytes=0, errors=[])
    root = output.resolve()
    cache = output / "saves" / "z"
    try:
        for path in (output, output / "saves", cache):
            try:
                status = path.lstat()
            except FileNotFoundError:
                return report
            if linked(status) or not stat.S_ISDIR(status.st_mode) or not path.resolve().is_relative_to(root):
                raise OSError(f"Refusing linked or uncontained cache directory: {path}")
        for path in cache.iterdir():
            if not re.fullmatch(r"cache[0-9]{3}\.map", path.name, re.IGNORECASE):
                continue
            status = path.lstat()
            if linked(status) or not stat.S_ISREG(status.st_mode) or path.resolve().parent != cache.resolve():
                report["errors"].append(f"Refusing nonregular or linked cache file: {path}")
                continue
            path.unlink()
            report["removed_files"] += 1
            report["removed_bytes"] += status.st_size
    except OSError as error:
        report["errors"].append(str(error))
    return report


def check_console_port(port):
    """Refuse a busy or inaccessible console before touching the data root."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
            probe.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        probe.bind(("127.0.0.1", port))


def verify_console_owner(connection, pid):
    # A successful preflight cannot reserve the port across process startup.
    # Check the accepted connection's exact tuple before sending any command.
    netstat = Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32" / "netstat.exe"
    try:
        report = subprocess.run([str(netstat), "-ano", "-p", "TCP"], capture_output=True,
                                text=True, check=True, timeout=5,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    except (OSError, subprocess.SubprocessError) as error:
        raise OSError(f"Cannot inspect console ownership: {error}") from error
    server = "{}:{}".format(*connection.getpeername())
    client = "{}:{}".format(*connection.getsockname())
    for line in report.stdout.splitlines():
        fields = line.split()
        if len(fields) == 5 and fields[:3] == ["TCP", server, client] and fields[4] == str(pid):
            return
    raise OSError(f"Console connection does not belong to launched process {pid}")


def replace_init(path, content):
    # Write beside init.txt, then replace atomically: a partial write must not
    # truncate the user's original startup file.
    descriptor, filename = tempfile.mkstemp(prefix=".mcc-smoke-", dir=path.parent)
    temporary = Path(filename)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(content)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def run_case(args, name, binary):
    name = name.replace("/", "\\")
    started = time.monotonic()
    args.output.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=name.replace("\\", "_") + "-", dir=args.output))
    data, init = args.data_root, args.data_root / "init.txt"
    original = init.read_bytes() if init.exists() else None
    debug = output / "debug.txt"
    marker = "mcc_smoke_" + uuid.uuid4().hex
    env = case_environment(args, output)
    transcript = ""
    launcher_errors = []
    process = connection = None
    def trace():
        if not debug.exists():
            return ""
        return debug.read_bytes().decode("latin1")
    try:
        check_console_port(args.port)
        if original is not None:
            (output / "init.original.txt").write_bytes(original)
        replace_init(init, f"map_name mcc_maps\\{name}\n".encode("ascii"))
        with (output / "stdout.log").open("wb") as stdout, (output / "stderr.log").open("wb") as stderr:
            process = subprocess.Popen([str(binary)], cwd=data, env=env, stdout=stdout, stderr=stderr,
                                       creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
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
                    except PermissionError:
                        raise
                    except OSError:
                        connection = None
                    if connection is not None:
                        verify_console_owner(connection, process.pid)
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
                    except PermissionError:
                        raise
                    except OSError:
                        connection.close()
                        connection = None
                time.sleep(.1)
    except OSError as error:
        launcher_errors.append(str(error))
    finally:
        if connection is not None:
            try:
                connection.close()
            except OSError as error:
                launcher_errors.append(f"Cannot close console: {error}")
        try:
            if process is not None and process.poll() is None:
                process.kill()
                process.wait(timeout=10)
        except (OSError, subprocess.SubprocessError) as error:
            launcher_errors.append(f"Cannot stop launched process: {error}")
        finally:
            try:
                current = init.read_bytes() if init.exists() else None
                if current != original:
                    if original is None:
                        init.unlink(missing_ok=True)
                    else:
                        replace_init(init, original)
            except OSError as error:
                launcher_errors.append(f"Cannot restore {init}: {error}")
            (output / "debug.log").write_text(trace(), encoding="utf-8")
            (output / "console.log").write_text(transcript, encoding="utf-8")
            process_stopped = process is None or process.poll() is not None
            if process_stopped:
                cleanup = cleanup_map_caches(output)
            else:
                cleanup = dict(removed_files=0, removed_bytes=0,
                               errors=[f"Process {process.pid} is still running; preserving {output}"])
    diagnostics = trace()
    for name_log in ("stdout.log", "stderr.log"):
        path = output / name_log
        if path.exists():
            diagnostics += "\n" + path.read_text(errors="replace")
    result = check_result(diagnostics, transcript, marker, name, process.returncode if process else None)
    result.update(output=str(output), launcher_error="; ".join(launcher_errors), cleanup=cleanup,
                  pid=process.pid if process else None, process_stopped=process_stopped,
                  settings={key: value for key, value in env.items() if key.startswith("HALO_")})
    result["passed"] = result["passed"] and not launcher_errors and not cleanup["errors"]
    result["elapsed_seconds"] = round(time.monotonic() - started, 2)
    result["executable_sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--data-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--seconds", type=int, default=45)
    parser.add_argument("--port", type=int, default=23251)
    parser.add_argument("--workshop", action="store_true", help="enable installed Steam Workshop discovery")
    parser.add_argument("--steam-root", type=Path, help="Steam install/library to check first; requires --workshop")
    parser.add_argument("--campaigns", action="store_true", help="enable explicit MCC campaign mission discovery")
    parser.add_argument("maps", nargs="+")
    args = parser.parse_args(argv)
    if args.seconds < 20 or args.seconds > 300:
        parser.error("--seconds must be between 20 and 300")
    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535")
    if args.steam_root:
        if not args.workshop or not args.steam_root.is_dir():
            parser.error("--steam-root requires --workshop and an existing directory")
        args.steam_root = args.steam_root.resolve()
    if not args.executable.is_file() or not (args.executable.parent / "SDL3.dll").is_file():
        parser.error("--executable must name a build with SDL3.dll beside it")
    if not (args.data_root / "maps" / "ui.map").is_file():
        parser.error("--data-root must contain maps/ui.map")
    init = args.data_root / "init.txt"
    try:
        status = init.lstat()
    except FileNotFoundError:
        pass
    else:
        if linked(status) or not stat.S_ISREG(status.st_mode):
            parser.error("the disposable data root's init.txt must be a regular file")
    for index, name in enumerate(args.maps):
        name = name.replace("\\", "/")
        if name.lower().startswith("mcc_maps/"):
            name = name[9:]
        if len("mcc_maps/" + name) > 63 or not re.fullmatch(r"[A-Za-z0-9_-]+(?:/[A-Za-z0-9_-]+)?", name):
            parser.error("supply loose or package/map names fitting the 63-byte MCC level name")
        if not (args.data_root / "mcc_maps" / (name + ".map")).is_file() and not (
                args.workshop and re.fullmatch(r"[0-9]{1,23}/[A-Za-z0-9_-]+", name)):
            parser.error("missing local map: Workshop item/map names require --workshop")
        args.maps[index] = name
    args.executable = args.executable.resolve()
    args.data_root, args.output = args.data_root.resolve(), args.output.resolve()
    return args


def main():
    args = parse_args()
    if os.name != "nt":
        raise SystemExit("this launcher currently supports Windows only")
    args.output.mkdir(parents=True, exist_ok=True)
    binary = Path(tempfile.mkdtemp(prefix="bin-", dir=args.output)) / "halo.exe"
    results = []
    process_stopped = True
    try:
        shutil.copy2(args.executable, binary)
        shutil.copy2(args.executable.parent / "SDL3.dll", binary.parent / "SDL3.dll")
        for name in args.maps:
            print(f"{name}: hidden startup and simulation check", flush=True)
            # An exception can prevent run_case from reporting child shutdown.
            # Retain its executable until completion is positively established.
            process_stopped = False
            result = run_case(args, name, binary)
            process_stopped = result.get("process_stopped", False)
            results.append(result)
            print(json.dumps(result), flush=True)
            (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
            if result.get("launcher_error") or not process_stopped:
                break
    finally:
        if process_stopped:
            binary.unlink(missing_ok=True)
            (binary.parent / "SDL3.dll").unlink(missing_ok=True)
    return 0 if process_stopped and all(result["passed"] for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
