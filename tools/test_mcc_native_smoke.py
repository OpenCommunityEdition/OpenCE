"""The startup checker must not repeat the old false-positive B40 result."""
import pytest
import os
import sys
from pathlib import Path
from types import SimpleNamespace
import mcc_native_smoke as smoke
from mcc_native_smoke import check_result

LOADED = "mcc: loaded mcc_maps\\b40 (5172 tags, 20224832 bytes of runtime tags)"
MARKER = "mcc_smoke_test"


@pytest.mark.parametrize("fatal", ["type is inconsistent with usage (you need to recompile scripts.)",
                                  "the scenario's scripts won't run", "EXCEPTION halt",
                                  "EXCEPTION assert in units.c,#1512: control_data->weapon_index",
                                  "backpack_1_argl: this is not a valid object name.",
                                  "timer_minutes: i expected a value of type long"])
def test_loader_success_cannot_hide_fatal_script_error(fatal):
    assert not check_result(LOADED + "\n" + fatal, MARKER, MARKER, "b40", 0)["passed"]


@pytest.mark.parametrize("console", ["", '(begin (sleep 150) (print "mcc_smoke_test"))',
                                    "previous_mcc_smoke_test"])
def test_requires_actual_delayed_output(console):
    assert not check_result(LOADED, console, MARKER, "b40", 0)["passed"]


def test_completed_simulation_and_normal_exit():
    assert check_result(LOADED, "\r\nmcc_smoke_test\r\n", MARKER, "b40", 0)["passed"]
    assert not check_result(LOADED, MARKER, MARKER, "b40", 1)["passed"]
    assert not check_result(LOADED, MARKER, MARKER, "a10", 0)["passed"]


def arguments(tmp_path, *maps, options=()):
    executable = tmp_path / "original" / "halo.exe"
    executable.parent.mkdir(exist_ok=True)
    executable.write_bytes(b"executable")
    (executable.parent / "SDL3.dll").write_bytes(b"library")
    data = tmp_path / "data"
    (data / "maps").mkdir(parents=True, exist_ok=True)
    (data / "maps" / "ui.map").write_bytes(b"menu")
    return ["--executable", str(executable), "--data-root", str(data),
            "--output", str(tmp_path / "runs"), *options, *maps]


def test_workshop_virtual_map_without_local_copy(tmp_path, monkeypatch):
    steam = tmp_path / "Steam"
    steam.mkdir()
    args = smoke.parse_args(arguments(tmp_path, r"mcc_maps\12345\a10",
                            options=("--workshop", "--steam-root", str(steam), "--campaigns")))
    assert args.maps == ["12345/a10"]
    assert not (args.data_root / "mcc_maps").exists()
    monkeypatch.setenv("HALO_NETWORK_TEST", "join")
    monkeypatch.setenv("HALO_STEAM_ROOT", "wrong installation")
    monkeypatch.setenv("HALO_NULL_RENDERER", "1")
    environment = smoke.case_environment(args, tmp_path / "case")
    assert "HALO_NETWORK_TEST" not in environment
    assert "HALO_NULL_RENDERER" not in environment
    assert environment["HALO_WORKSHOP_MAPS"] == "1"
    assert environment["HALO_MCC_CAMPAIGNS"] == "1"
    assert environment["HALO_STEAM_ROOT"] == str(steam.resolve())
    assert environment["HALO_LOG_ROOT"] == str(tmp_path / "case")


@pytest.mark.skipif(os.name != "nt", reason="Windows launcher")
def test_main_accepts_workshop_and_runs_cases_in_order(tmp_path, monkeypatch):
    argv = arguments(tmp_path, "12345/a10", "67890/b40", options=("--workshop",))
    calls = []
    def run(args, name, binary):
        assert binary.read_bytes() == b"executable"
        calls.append(name)
        return {"map": name, "passed": True, "process_stopped": True}
    monkeypatch.setattr(smoke, "run_case", run)
    monkeypatch.setattr(sys, "argv", ["mcc_native_smoke.py", *argv])
    assert smoke.main() == 0
    assert calls == ["12345/a10", "67890/b40"]
    assert not list((tmp_path / "runs").rglob("*.exe"))
    assert not list((tmp_path / "runs").rglob("*.dll"))
    assert (tmp_path / "original" / "halo.exe").read_bytes() == b"executable"


@pytest.mark.skipif(os.name != "nt", reason="Windows launcher")
def test_launcher_error_stops_following_cases(tmp_path, monkeypatch):
    argv = arguments(tmp_path, "12345/a10", "67890/b40", options=("--workshop",))
    calls = []
    def fail(args, name, binary):
        calls.append(name)
        return {"map": name, "passed": False, "process_stopped": True,
                "launcher_error": "cannot restore init"}
    monkeypatch.setattr(smoke, "run_case", fail)
    monkeypatch.setattr(sys, "argv", ["mcc_native_smoke.py", *argv])
    assert smoke.main() == 1
    assert calls == ["12345/a10"]
    assert not list((tmp_path / "runs").rglob("*.exe"))


@pytest.mark.parametrize("name", ["b40", "package/b40"])
def test_existing_local_invocation_keeps_discovery_disabled(tmp_path, name):
    argv = arguments(tmp_path, name)
    level = tmp_path / "data" / "mcc_maps" / (name + ".map")
    level.parent.mkdir(parents=True)
    level.write_bytes(b"map")
    args = smoke.parse_args(argv)
    environment = smoke.case_environment(args, tmp_path / "case")
    assert args.maps == [name]
    assert environment["HALO_WORKSHOP_MAPS"] == "0"
    assert environment["HALO_MCC_CAMPAIGNS"] == "0"
    assert environment["HALO_STEAM_ROOT"] == ""


@pytest.mark.parametrize("name,options", [
    ("12345/a10", ()),
    ("package/a10", ("--workshop",)),
    ("../a10", ("--workshop",)),
    ("12345/a10;quit", ("--workshop",)),
    ("12345/" + "a" * 50, ("--workshop",)),
    ("12345/a10", ("--port", "0", "--workshop")),
    ("12345/a10", ("--port", "65536", "--workshop")),
    ("12345/a10", ("--seconds", "19", "--workshop")),
    ("12345/a10", ("--steam-root", ".")),
])
def test_invalid_requests_fail_before_starting(tmp_path, monkeypatch, name, options):
    monkeypatch.setattr(smoke.subprocess, "Popen", lambda *a, **k: pytest.fail("started a game"))
    with pytest.raises(SystemExit):
        smoke.parse_args(arguments(tmp_path, name, options=options))
    assert not (tmp_path / "runs").exists()


def test_cleanup_removes_only_owned_regular_map_caches(tmp_path):
    case = tmp_path / "case"
    cache = case / "saves" / "z"
    cache.mkdir(parents=True)
    (cache / "cache000.map").write_bytes(b"abc")
    (cache / "cache001.map").write_bytes(b"de")
    (cache / "savegame.bin").write_bytes(b"save")
    (cache / "other.map").write_bytes(b"retained")
    report = smoke.cleanup_map_caches(case)
    assert report["removed_files"] == 2 and report["removed_bytes"] == 5
    assert (cache / "savegame.bin").read_bytes() == b"save"
    assert (cache / "other.map").read_bytes() == b"retained"
    assert not report["errors"]


@pytest.mark.parametrize("component", ["saves", "z", "cache000.map"])
def test_cleanup_refuses_linked_cache_paths(tmp_path, component):
    case = tmp_path / "case"
    case.mkdir()
    original = tmp_path / "original"
    (original / "z").mkdir(parents=True)
    source = original / "z" / "cache000.map"
    source.write_bytes(b"original")
    if component == "saves":
        link, target = case / "saves", original
    elif component == "z":
        (case / "saves").mkdir()
        link, target = case / "saves" / "z", original / "z"
    else:
        (case / "saves" / "z").mkdir(parents=True)
        link, target = case / "saves" / "z" / component, source
    try:
        link.symlink_to(target, target_is_directory=target.is_dir())
    except OSError:
        if os.name != "nt" or not target.is_dir():
            pytest.skip("symlinks unavailable")
        result = smoke.subprocess.run(["cmd", "/c", "mklink", "/J", str(link), str(target)],
                                      capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
    try:
        report = smoke.cleanup_map_caches(case)
        assert report["removed_files"] == 0 and report["errors"]
        assert source.read_bytes() == b"original"
    finally:
        if link.is_symlink():
            link.unlink()
        elif os.name == "nt":
            os.rmdir(link)


def test_cleanup_refuses_final_file_reparse_attribute(tmp_path, monkeypatch):
    # Exercise Windows' file reparse guard even without symlink privileges.
    case = tmp_path / "case"
    cache = case / "saves" / "z"
    cache.mkdir(parents=True)
    target = cache / "cache000.map"
    target.write_bytes(b"keep")
    lstat = Path.lstat
    def status(path):
        actual = lstat(path)
        if path == target:
            return SimpleNamespace(st_mode=actual.st_mode, st_size=actual.st_size,
                                   st_file_attributes=0x400)
        return actual
    monkeypatch.setattr(Path, "lstat", status)
    report = smoke.cleanup_map_caches(case)
    assert report["removed_files"] == 0 and report["errors"]
    assert target.read_bytes() == b"keep"


@pytest.mark.parametrize("original", [None, b"original startup\r\n"])
def test_launch_failure_restores_init_and_retains_diagnostics(tmp_path, monkeypatch, original):
    argv = arguments(tmp_path, "b40")
    level = tmp_path / "data" / "mcc_maps" / "b40.map"
    level.parent.mkdir()
    level.write_bytes(b"map")
    args = smoke.parse_args(argv)
    init = args.data_root / "init.txt"
    if original is not None:
        init.write_bytes(original)
    def fail(*a, **k):
        raise OSError("simulated launch failure")
    monkeypatch.setattr(smoke.subprocess, "Popen", fail)
    result = smoke.run_case(args, "b40", args.executable)
    assert not result["passed"]
    assert "simulated launch failure" in result["launcher_error"]
    assert init.read_bytes() == original if original is not None else not init.exists()
    assert level.read_bytes() == b"map"
    assert list(args.output.rglob("result.json"))


@pytest.mark.parametrize("operation", ["create_transport_endpoint", "bind_endpoint", "listen_endpoint"])
def test_console_initialization_failure_cannot_pass(operation):
    fatal = f"{operation}() failed on telnet console endpoint"
    assert not check_result(LOADED + "\n" + fatal, MARKER, MARKER, "b40", 0)["passed"]


def test_occupied_console_port_refuses_before_init_write_or_launch(tmp_path, monkeypatch):
    args = smoke.parse_args(arguments(tmp_path, "12345/a10", options=("--workshop",)))
    init = args.data_root / "init.txt"
    init.write_bytes(b"preserved startup")
    original_time = init.stat().st_mtime_ns
    monkeypatch.setattr(smoke.subprocess, "Popen", lambda *a, **k: pytest.fail("started a game"))
    with smoke.socket.socket(smoke.socket.AF_INET, smoke.socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        listener.listen()
        args.port = listener.getsockname()[1]
        result = smoke.run_case(args, args.maps[0], args.executable)
    assert not result["passed"] and result["launcher_error"]
    assert init.read_bytes() == b"preserved startup"
    assert init.stat().st_mtime_ns == original_time


@pytest.mark.parametrize("row,accepted", [
    ("TCP 127.0.0.1:23251 127.0.0.1:50000 ESTABLISHED 1234", True),
    ("TCP 127.0.0.1:23251 127.0.0.1:50000 ESTABLISHED 9999", False),
    ("TCP 127.0.0.1:23251 127.0.0.1:50001 ESTABLISHED 1234", False),
    ("TCP 127.0.0.1:50000 127.0.0.1:23251 ESTABLISHED 1234", False),
    ("TCP 0.0.0.0:23251 0.0.0.0:0 LISTENING 1234", False),
])
def test_console_connection_requires_exact_child_owned_tuple(monkeypatch, row, accepted):
    connection = SimpleNamespace(getpeername=lambda: ("127.0.0.1", 23251),
                                 getsockname=lambda: ("127.0.0.1", 50000))
    monkeypatch.setattr(smoke.subprocess, "run", lambda *a, **k: SimpleNamespace(stdout=row))
    if accepted:
        smoke.verify_console_owner(connection, 1234)
    else:
        with pytest.raises(OSError, match="does not belong"):
            smoke.verify_console_owner(connection, 1234)


def test_console_ownership_inspection_failure_is_terminal(monkeypatch):
    def fail(*a, **k):
        raise smoke.subprocess.TimeoutExpired("netstat", 5)
    monkeypatch.setattr(smoke.subprocess, "run", fail)
    with pytest.raises(OSError, match="inspect console ownership"):
        smoke.verify_console_owner(None, 1234)


@pytest.mark.parametrize("failure", ["connect", "owner", "send", "recv"])
def test_console_terminal_errors_retain_evidence(tmp_path, monkeypatch, failure):
    args = smoke.parse_args(arguments(tmp_path, "12345/a10", options=("--workshop",)))
    monkeypatch.setattr(smoke, "check_console_port", lambda port: None)
    monkeypatch.setattr(smoke.time, "sleep", lambda seconds: None)
    polls, sent = [], []
    process = SimpleNamespace(pid=1234, returncode=None)
    def poll():
        polls.append(None)
        if len(polls) > 6:
            process.returncode = 0
        return process.returncode
    process.poll = poll
    process.kill = lambda: setattr(process, "returncode", -1)
    process.wait = lambda **k: process.returncode
    def launch(*a, **k):
        (Path(k["env"]["HALO_LOG_ROOT"]) / "debug.txt").write_text(
            "mcc: loaded mcc_maps\\12345\\a10 (100 tags)")
        return process
    def denied(*a, **k):
        raise PermissionError("simulated socket permission denied")
    connection = SimpleNamespace(settimeout=lambda value: None, close=lambda: None,
                                 sendall=lambda data: sent.append(data), recv=lambda size: b"")
    monkeypatch.setattr(smoke.subprocess, "Popen", launch)
    monkeypatch.setattr(smoke.socket, "create_connection", denied if failure == "connect" else lambda *a: connection)
    monkeypatch.setattr(smoke, "verify_console_owner", denied if failure == "owner" else lambda *a: None)
    if failure in ("send", "recv"):
        setattr(connection, failure if failure == "recv" else "sendall", denied)
    result = smoke.run_case(args, args.maps[0], args.executable)
    assert "simulated socket permission denied" in result["launcher_error"]
    assert len(polls) <= 3 and not result["passed"]
    assert list(args.output.rglob("result.json"))
    if failure in ("connect", "owner"):
        assert not sent


def test_atomic_init_write_failure_preserves_original_and_diagnostics(tmp_path, monkeypatch):
    args = smoke.parse_args(arguments(tmp_path, "12345/a10", options=("--workshop",)))
    init = args.data_root / "init.txt"
    init.write_bytes(b"original startup")
    original_time = init.stat().st_mtime_ns
    monkeypatch.setattr(smoke, "check_console_port", lambda port: None)
    def fail(*a, **k):
        raise PermissionError("simulated init replacement failure")
    monkeypatch.setattr(smoke.os, "replace", fail)
    monkeypatch.setattr(smoke.subprocess, "Popen", lambda *a, **k: pytest.fail("started a game"))
    result = smoke.run_case(args, args.maps[0], args.executable)
    assert "simulated init replacement failure" in result["launcher_error"]
    assert init.read_bytes() == b"original startup" and init.stat().st_mtime_ns == original_time
    assert not list(args.data_root.glob(".mcc-smoke-*"))
    assert list(args.output.rglob("result.json"))


def test_partial_init_write_failure_never_truncates_original(tmp_path, monkeypatch):
    init = tmp_path / "init.txt"
    init.write_bytes(b"original startup")
    fdopen = smoke.os.fdopen
    class PartialWrite:
        def __init__(self, stream):
            self.stream = stream
        def __enter__(self):
            return self
        def __exit__(self, *args):
            self.stream.close()
        def write(self, content):
            self.stream.write(content[:3])
            raise OSError("simulated partial write")
    monkeypatch.setattr(smoke.os, "fdopen", lambda *a, **k: PartialWrite(fdopen(*a, **k)))
    with pytest.raises(OSError, match="partial write"):
        smoke.replace_init(init, b"replacement startup")
    assert init.read_bytes() == b"original startup"
    assert not list(tmp_path.glob(".mcc-smoke-*"))


def test_init_restore_failure_keeps_logs_and_cache_cleanup(tmp_path, monkeypatch):
    args = smoke.parse_args(arguments(tmp_path, "12345/a10", options=("--workshop",)))
    init = args.data_root / "init.txt"
    init.write_bytes(b"original startup")
    monkeypatch.setattr(smoke, "check_console_port", lambda port: None)
    replace = smoke.os.replace
    calls = []
    def replace_once(*a, **k):
        calls.append(None)
        if len(calls) == 2:
            raise PermissionError("simulated restore failure")
        return replace(*a, **k)
    monkeypatch.setattr(smoke.os, "replace", replace_once)
    def fail(*a, **k):
        cache = Path(k["env"]["HALO_SAVE_ROOT"]) / "z"
        cache.mkdir(parents=True)
        (cache / "cache000.map").write_bytes(b"cache")
        raise OSError("simulated launch failure")
    monkeypatch.setattr(smoke.subprocess, "Popen", fail)
    result = smoke.run_case(args, args.maps[0], args.executable)
    assert "simulated restore failure" in result["launcher_error"]
    assert result["cleanup"]["removed_files"] == 1
    assert list(args.output.rglob("result.json")) and list(args.output.rglob("console.log"))
    assert (Path(result["output"]) / "init.original.txt").read_bytes() == b"original startup"


@pytest.mark.parametrize("stop_failure", [None, "kill", "wait"])
def test_cache_cleanup_requires_observed_child_exit(tmp_path, monkeypatch, stop_failure):
    args = smoke.parse_args(arguments(tmp_path, "12345/a10", options=("--workshop",)))
    init = args.data_root / "init.txt"
    init.write_bytes(b"original startup")
    monkeypatch.setattr(smoke, "check_console_port", lambda port: None)
    process = SimpleNamespace(pid=1234, returncode=None)
    process.poll = lambda: process.returncode
    def kill():
        if stop_failure == "kill":
            raise PermissionError("simulated kill failure")
        if stop_failure is None:
            process.returncode = -1
    def wait(**kwargs):
        if stop_failure == "wait":
            raise smoke.subprocess.TimeoutExpired("halo", 10)
        return process.returncode
    process.kill, process.wait = kill, wait
    def launch(*a, **k):
        output = Path(k["env"]["HALO_LOG_ROOT"])
        (output / "debug.txt").write_text("EXCEPTION halt")
        cache = Path(k["env"]["HALO_SAVE_ROOT"]) / "z"
        cache.mkdir(parents=True)
        (cache / "cache000.map").write_bytes(b"owned cache")
        (cache / "savegame.bin").write_bytes(b"preserved save")
        return process
    monkeypatch.setattr(smoke.subprocess, "Popen", launch)
    result = smoke.run_case(args, args.maps[0], args.executable)
    cache = Path(result["output"]) / "saves" / "z"
    assert (cache / "cache000.map").exists() == (stop_failure is not None)
    assert (cache / "savegame.bin").read_bytes() == b"preserved save"
    assert init.read_bytes() == b"original startup"
    assert result["process_stopped"] == (stop_failure is None)
    assert result["pid"] == process.pid and not result["passed"]
    assert bool(result["cleanup"]["errors"]) == (stop_failure is not None)
    assert bool(result["launcher_error"]) == (stop_failure is not None)


@pytest.mark.parametrize("failure", [None, "stdout", "close"])
def test_console_completion_checks_all_logs_and_survives_close_error(tmp_path, monkeypatch, failure):
    args = smoke.parse_args(arguments(tmp_path, "12345/a10", options=("--workshop",)))
    init = args.data_root / "init.txt"
    init.write_bytes(b"original startup")
    monkeypatch.setattr(smoke, "check_console_port", lambda port: None)
    monkeypatch.setattr(smoke, "verify_console_owner", lambda *a: None)
    monkeypatch.setattr(smoke.time, "sleep", lambda seconds: None)
    process = SimpleNamespace(pid=1234, returncode=None)
    process.poll = lambda: process.returncode
    killed, commands = [], []
    def kill():
        killed.append(True)
        process.returncode = -1
    process.kill = kill
    process.wait = lambda **k: process.returncode
    output = []
    def launch(*a, **k):
        output.append(Path(k["env"]["HALO_LOG_ROOT"]))
        (output[0] / "debug.txt").write_text("mcc: loaded mcc_maps\\12345\\a10 (100 tags)")
        if failure == "stdout":
            k["stdout"].write(b"EXCEPTION halt\n")
        return process
    def receive(size):
        marker = commands[0].decode("ascii").split('"')[1]
        if failure == "close":
            (output[0] / "debug.txt").write_text("EXCEPTION halt")
        else:
            process.returncode = 0
        return ("\r\n" + marker + "\r\n").encode("ascii")
    def close():
        if failure == "close":
            raise OSError("simulated console close failure")
    connection = SimpleNamespace(settimeout=lambda value: None, close=close,
                                 sendall=commands.append, recv=receive)
    monkeypatch.setattr(smoke.subprocess, "Popen", launch)
    monkeypatch.setattr(smoke.socket, "create_connection", lambda *a: connection)
    result = smoke.run_case(args, args.maps[0], args.executable)
    assert result["passed"] == (failure is None)
    assert result["process_stopped"]
    assert init.read_bytes() == b"original startup"
    assert (Path(result["output"]) / "console.log").is_file()
    if failure == "stdout":
        assert "exception halt" in result["failures"]
    if failure == "close":
        assert killed and "simulated console close failure" in result["launcher_error"]


@pytest.mark.skipif(os.name != "nt", reason="Windows launcher")
@pytest.mark.parametrize("completion", ["survivor", "unknown", "exception"])
def test_main_retains_binary_until_child_completion_is_known(tmp_path, monkeypatch, completion):
    argv = arguments(tmp_path, "12345/a10", "67890/b40", options=("--workshop",))
    calls = []
    def run(args, name, binary):
        calls.append(name)
        if completion == "exception":
            raise OSError("simulated evidence write failure")
        result = {"map": name, "passed": True}
        if completion == "survivor":
            result["process_stopped"] = False
        return result
    monkeypatch.setattr(smoke, "run_case", run)
    monkeypatch.setattr(sys, "argv", ["mcc_native_smoke.py", *argv])
    if completion == "exception":
        with pytest.raises(OSError, match="evidence write failure"):
            smoke.main()
    else:
        assert smoke.main() == 1
    assert calls == ["12345/a10"]
    assert [path.read_bytes() for path in (tmp_path / "runs").rglob("*.exe")] == [b"executable"]
    assert [path.read_bytes() for path in (tmp_path / "runs").rglob("*.dll")] == [b"library"]
