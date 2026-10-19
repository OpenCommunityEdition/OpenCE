"""The guest runtime's services in the macOS host's VM (docs/macos_host.md).

port/macos/tests/vm_runtime.c is built as a guest image (tools/macos_guest_image.py)
and each of its cases is run in the host: threads with their own thread
pointers and TLS, futexes (mutexes, condition variables, timed waits), time,
files with Linux's flags and errno values, and memory. Needs `ninja macos` in
`nix develop`.
"""

import platform
import re
import shutil
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools import macos_guest_image  # noqa: E402

pytestmark = [
    pytest.mark.skipif(sys.platform != "darwin" or platform.machine() != "arm64", reason="arm64 macOS only"),
    pytest.mark.skipif(not macos_guest_image.available() or shutil.which("ninja") is None,
                       reason="needs `ninja macos` in nix develop"),
]


@pytest.fixture(scope="module")
def image(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_runtime")
    return macos_guest_image.build_guest_image([ROOT / "port/macos/tests/vm_runtime.c"], work, "vm_runtime")


def run_case(image, tmp_path, *arguments, environment=None):
    result = macos_guest_image.run_guest(image, arguments, tmp_path / "run", environment)
    return result.returncode, result.stdout, result.stderr


@pytest.mark.parametrize("case", ["threads", "condition", "timed_wait", "time", "memory", "sockets"])
def test_case(image, tmp_path, case):
    status, stdout, stderr = run_case(image, tmp_path, case)
    assert (status, stdout.strip()) == (0, "ok"), stdout + stderr


def test_files(image, tmp_path):
    folder = tmp_path / "files"
    folder.mkdir()
    status, stdout, stderr = run_case(image, tmp_path, "files", str(folder))
    assert (status, stdout.strip()) == (0, "ok"), stdout + stderr
    assert list(folder.iterdir()) == []


def test_the_guests_exit_status_is_the_hosts(image, tmp_path):
    status, stdout, stderr = run_case(image, tmp_path, "exit")
    assert status == 42, stdout + stderr


def test_the_host_stays_quiet_unless_asked(image, tmp_path):
    status, stdout, stderr = run_case(image, tmp_path, "time")
    assert status == 0 and "halo_host" not in stdout + stderr, stdout + stderr


def import_counts(stderr):
    return {name: int(count) for name, count in re.findall(r"halo_host: import (\w+): (\d+)", stderr)}


def test_the_thread_pointer_costs_no_exit(image, tmp_path):
    # In the VM, TPIDR_EL0 is the guest's own: the guest reads and sets its
    # thread pointer there, not through host_get_tp and host_set_tp, which
    # would each be an exit (musl reads it for every lock and errno)
    status, stdout, stderr = run_case(image, tmp_path, "threads", environment={"HALO_HOST_IMPORT_COUNTS": "1"})
    assert (status, stdout.strip()) == (0, "ok"), stdout + stderr
    counts = import_counts(stderr)
    assert counts.get("host_syscall", 0) > 0, stderr
    assert counts.get("host_get_tp", 0) == 0 and counts.get("host_set_tp", 0) == 0, counts


def test_the_hosts_system_calls_natively(tmp_path):
    # port/macos/host/host_syscall.c, built for this Mac with guest memory as
    # a buffer: its errno table, and calls made with Linux's numbers and flags
    import os
    import subprocess

    exe = tmp_path / "host_syscall_test"
    sdl_include = os.environ.get("HALO_MACOS_SDL_INCLUDE", "")
    build = subprocess.run([
        "cc", "-std=gnu11", "-Wall", "-Werror", "-D_DARWIN_C_SOURCE", "-mmacosx-version-min=26.0",
        "-Iport/macos/host", "-Ibuild/macos/runtime", "-Iport/android/include",
        "-Iport/android/guest/libc/arch/arm64_32", f"-I{sdl_include}",
        "port/macos/tests/host_syscall_test.c", "port/macos/host/host_syscall.c", "-o", str(exe),
    ], cwd=ROOT, capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    folder = tmp_path / "files"
    folder.mkdir()
    result = subprocess.run([str(exe), str(folder)], capture_output=True, text=True, timeout=30)
    assert (result.returncode, result.stdout.strip()) == (0, "ok"), result.stdout + result.stderr
    assert list(folder.iterdir()) == []


def test_the_main_threads_queue_natively(tmp_path):
    # port/macos/host/host_main_thread.c, built for this Mac: requests from
    # other threads run on the main thread's run loop, and fail cleanly once
    # the host shuts down
    import os
    import subprocess

    exe = tmp_path / "host_main_thread_test"
    sdl_include = os.environ.get("HALO_MACOS_SDL_INCLUDE", "")
    build = subprocess.run([
        "cc", "-std=gnu11", "-Wall", "-Werror", "-D_DARWIN_C_SOURCE", "-mmacosx-version-min=26.0",
        "-Iport/macos/host", "-Ibuild/macos/runtime", "-Iport/android/include",
        "-Iport/android/guest/libc/arch/arm64_32", f"-I{sdl_include}",
        "port/macos/tests/host_main_thread_test.c", "port/macos/host/host_main_thread.c",
        "-framework", "CoreFoundation", "-o", str(exe),
    ], cwd=ROOT, capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
    assert (result.returncode, result.stdout.strip()) == (0, "ok"), result.stdout + result.stderr


def test_the_window_imports_use_the_apps_windows_natively(tmp_path):
    # port/macos/host/host_window.c, built for this Mac: the window imports
    # go to the windows the app installs, else to SDL's
    import os
    import subprocess

    exe = tmp_path / "host_window_test"
    sdl_include = os.environ.get("HALO_MACOS_SDL_INCLUDE", "")
    build = subprocess.run([
        "cc", "-std=gnu11", "-Wall", "-Werror", "-D_DARWIN_C_SOURCE", "-mmacosx-version-min=26.0",
        "-Iport/macos/host", "-Ibuild/macos/runtime", "-Iport/android/include",
        "-Iport/android/guest/libc/arch/arm64_32", f"-I{sdl_include}",
        "port/macos/tests/host_window_test.c", "port/macos/host/host_window.c", "-o", str(exe),
    ], cwd=ROOT, capture_output=True, text=True)
    assert build.returncode == 0, build.stderr
    result = subprocess.run([str(exe)], capture_output=True, text=True, timeout=30)
    assert (result.returncode, result.stdout.strip()) == (0, "ok"), result.stdout + result.stderr


# ---------- SDL, headless

SDL_DRIVERS = {"SDL_AUDIO_DRIVER": "dummy", "SDL_VIDEO_DRIVER": "dummy"}


@pytest.fixture(scope="module")
def sdl_image(tmp_path_factory):
    work = tmp_path_factory.mktemp("vm_sdl")
    return macos_guest_image.build_guest_image([ROOT / "port/macos/tests/vm_sdl.c"], work, "vm_sdl")


@pytest.mark.parametrize("case", ["audio", "audio_then_wait", "events"])
def test_sdl(sdl_image, tmp_path, case):
    status, stdout, stderr = run_case(sdl_image, tmp_path, case, environment=SDL_DRIVERS)
    assert (status, stdout.strip()) == (0, "ok"), stdout + stderr


def test_the_audio_callback_is_guest_code_on_a_vcpu_of_its_own(sdl_image, tmp_path):
    # (the callback's calls into the host: its SDL_PutAudioStreamData)
    status, stdout, stderr = run_case(sdl_image, tmp_path, "audio",
                                      environment={**SDL_DRIVERS, "HALO_HOST_IMPORT_COUNTS": "1"})
    assert (status, stdout.strip()) == (0, "ok"), stdout + stderr
    assert import_counts(stderr).get("host_sdl_put_audio_stream_data", 0) >= 3, stderr


def test_opengl_es_is_refused_while_headless(sdl_image, tmp_path):
    status, stdout, stderr = run_case(sdl_image, tmp_path, "gl", environment=SDL_DRIVERS)
    assert (status, stdout.strip()) == (0, "ok"), stdout + stderr
    assert stderr.count("the guest called hostgl_glClear, which this host does not carry out") == 1, stderr


def test_window_imports_run_on_the_hosts_main_thread(sdl_image, tmp_path):
    # with the host's main thread running its run loop (as the app's does),
    # a guest thread's SDL_Init and SDL_CreateWindow are carried out there
    status, stdout, stderr = run_case(sdl_image, tmp_path, "window_from_thread",
                                      environment={**SDL_DRIVERS, "HALO_HOST_MAIN_LOOP": "1",
                                                   "HALO_HOST_TRACE_MAIN_THREAD": "1"})
    assert (status, stdout.strip()) == (0, "ok"), stdout + stderr
    assert "halo_host: host_sdl_init ran on the main thread" in stderr
    assert "halo_host: host_sdl_create_window ran on the main thread" in stderr
    # (an import that need not be there is not)
    assert "host_sdl_ticks ran on the main thread" not in stderr
