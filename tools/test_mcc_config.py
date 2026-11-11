"""MCC discovery defaults and one-time upgrades through the real config loader."""

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tomllib

import pytest

ROOT = Path(__file__).resolve().parents[1]
KEYS = ("workshop_maps", "mcc_campaigns", "mcc_collections")
OLD = "[game]\n" + "".join(f"{key} = false # keep comment\n" for key in KEYS)
DRIVER = r'''
#include "port_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static int argument_count;
static char **arguments;
void platform_log(const char *format, ...) {
    va_list args;
    va_start(args, format); vfprintf(stderr, format, args); va_end(args);
    fputc('\n', stderr);
}
int posix_command_line_argument(int index, char *text, unsigned long size) {
    if (index >= argument_count) return 0;
    snprintf(text, size, "%s", arguments[index]); return 1;
}
const char *SDL_GetBasePath(void) { return ""; }
int SDL_GetPathInfo(const char *path, void *info) { (void)path; (void)info; return 0; }
int main(int argc, char **argv) {
    char value[256];
    argument_count = argc; arguments = argv;
    if (argc == 4 && !strcmp(argv[1], "write"))
        return !config_write(argv[2], argv[3]);
    if (argc >= 3 && config_text(argv[2], value, sizeof(value)))
        return puts(value) < 0;
    return 1;
}
'''


@pytest.fixture(scope="module", params=["desktop", "android", "web"])
def config_program(request, tmp_path_factory):
    compiler = os.environ.get("CC") or shutil.which("clang") or shutil.which("cc")
    assert compiler, "C compiler required"
    work = tmp_path_factory.mktemp("mcc-config-" + request.param)
    stubs = work / "stubs"
    (stubs / "SDL3").mkdir(parents=True)
    (stubs / "platform.h").write_text("void platform_log(const char *, ...);\n")
    (stubs / "posix.h").write_text(
        "typedef unsigned long posix_ulong;\n"
        "int posix_command_line_argument(int, char *, posix_ulong);\n")
    (stubs / "posix_server.h").write_text("")
    (stubs / "SDL3/SDL.h").write_text(
        "typedef struct { long long size, modify_time; } SDL_PathInfo;\n"
        "const char *SDL_GetBasePath(void);\nint SDL_GetPathInfo(const char *, void *);\n")
    # File I/O uses the guest's stdio path; no SDL or game initialization.
    flags = ["-DHALO_ARM64_GUEST=1"]
    if request.param != "desktop":
        flags += ["-DHALO_" + request.param.upper() + "=1"]
    if sys.platform == "win32":
        flags += ["--target=i686-pc-windows-msvc", "-fuse-ld=lld", "-D_CRT_SECURE_NO_WARNINGS"]
        # The driver is single-threaded. Linux exercises the real pthread lock.
        (stubs / "pthread.h").write_text(
            "typedef int pthread_mutex_t;\n#define PTHREAD_MUTEX_INITIALIZER 0\n"
            "#define pthread_mutex_lock(p) ((void)(p))\n"
            "#define pthread_mutex_unlock(p) ((void)(p))\n")
    else:
        flags += ["-D_GNU_SOURCE", "-lpthread"]
    for name in ("port_config.c", "port_config.h", "mcc_config.c", "mcc_config.h"):
        shutil.copyfile(ROOT / "port/linux/src" / name, stubs / name)
    (work / "main.c").write_text(DRIVER)
    exe = work / ("config.exe" if sys.platform == "win32" else "config")
    result = subprocess.run(
        [compiler, *flags, "-std=gnu11", "-I" + str(stubs),
         "-I" + str(ROOT / "port/third_party/tomlc17"), str(work / "main.c"),
         str(stubs / "port_config.c"), str(stubs / "mcc_config.c"),
         str(ROOT / "port/third_party/tomlc17/tomlc17.c"), "-o", str(exe)],
        capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return exe, request.param


@pytest.fixture
def config(config_program, tmp_path):
    exe, platform = config_program
    path = tmp_path / "config.toml"

    def run(*args, overrides=None):
        env = {key: value for key, value in os.environ.items() if not key.startswith("HALO_")}
        env.update(HALO_DATA_ROOT=str(tmp_path), HALO_CONFIG_FILE=str(path))
        env.update(overrides or {})
        result = subprocess.run([str(exe), *args], env=env, cwd=tmp_path,
                                capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        return result.stdout.strip(), result.stderr
    return path, run, platform


def test_fresh_defaults_and_saved_opt_out(config):
    path, run, platform = config
    for key in KEYS:
        assert run("get", "game." + key)[0] == "true"
    saved = tomllib.loads(path.read_text())
    assert saved["game"]["mcc_discovery_defaults"] is True
    assert ("workshop_maps" in saved["game"]) == (platform != "android")
    for key in KEYS:
        run("write", "game." + key, "false")
    for key in KEYS:
        assert run("get", "game." + key)[0] == "false"


def test_old_defaults_upgrade_once_and_preserve_unrelated_settings(config):
    path, run, _ = config
    path.write_text(OLD + 'custom_edition = false\n[display]\nvsync = false\n'
                    '[network]\nmcc_map_share = false\n')
    for key in KEYS:
        assert run("get", "game." + key)[0] == "true"
    saved = tomllib.loads(path.read_text())
    assert all(saved["game"][key] is True for key in KEYS)
    assert saved["game"]["custom_edition"] is False
    assert saved["display"]["vsync"] is False
    assert saved["network"]["mcc_map_share"] is False
    assert path.read_text().count("# keep comment") == 3
    for key in KEYS:
        run("write", "game." + key, "false")
    for key in KEYS:
        assert run("get", "game." + key)[0] == "false"


def test_missing_keys_and_existing_completion_marker(config):
    path, run, _ = config
    path.write_text("[game]\nmcc_campaigns = false\nmcc_discovery_defaults = true\n")
    assert run("get", "game.mcc_campaigns")[0] == "false"
    assert run("get", "game.mcc_collections")[0] == "true"
    assert run("get", "game.workshop_maps")[0] == "true"


def test_environment_override_does_not_become_saved_preference(config):
    path, run, _ = config
    path.write_text(OLD)
    for key, variable in zip(KEYS, ("HALO_WORKSHOP_MAPS", "HALO_MCC_CAMPAIGNS", "HALO_MCC_COLLECTIONS")):
        assert run("get", "game." + key, overrides={variable: "0"})[0] == "false"
        assert run("get", "game." + key)[0] == "true"


def test_damaged_config_is_not_migrated_or_written(config):
    path, run, _ = config
    original = OLD + "console_log = unquoted\n"
    path.write_text(original)
    assert run("get", "game.mcc_campaigns")[0] == "false"
    assert path.read_text() == original
    path.write_text(OLD)
    assert run("get", "game.mcc_campaigns")[0] == "true"


def test_read_only_command_waits_for_normal_launch(config):
    path, run, platform = config
    if platform != "desktop":
        pytest.skip("command-only modes are desktop-only")
    path.write_text(OLD)
    assert run("get", "game.mcc_campaigns", "--version")[0] == "false"
    assert path.read_text() == OLD
    assert run("get", "game.mcc_campaigns")[0] == "true"


def test_quoted_keys_crlf_and_multiline_strings(config):
    path, run, _ = config
    original = ('[game]\r\n"mcc_campaigns" = false # campaign\r\n'
                'mcc_collections = false\r\nworkshop_maps = false\r\n'
                'console_log = """all\r\nmcc_campaigns = false\r\n"""\r\n')
    path.write_bytes(original.encode())
    assert run("get", "game.mcc_campaigns")[0] == "true"
    saved = tomllib.loads(path.read_text())
    assert all(saved["game"][key] is True for key in KEYS)
    assert "mcc_campaigns = false" in saved["game"]["console_log"]
    assert "# campaign" in path.read_text()


def test_duplicate_sections_upgrade_and_keep_values(config):
    path, run, _ = config
    path.write_text('[game]\nmcc_campaigns = false\n[display]\nvsync = false\n'
                    '[game]\nmcc_collections = false\nworkshop_maps = true\n')
    assert run("get", "game.mcc_campaigns")[0] == "true"
    saved = tomllib.loads(path.read_text())
    assert all(saved["game"][key] is True for key in KEYS)
    assert saved["display"]["vsync"] is False
