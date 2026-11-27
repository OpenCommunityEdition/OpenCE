"""The dedicated server's Pterodactyl egg (deploy/pterodactyl): its shape as
the panel imports it (PTDL_v2), that tools/egg_build.py would write it as
it is, and its installation script, run in a debian:bookworm-slim container
against a fake release served from this machine (skipped without a Docker
that works without sudo).

    python -m pytest -q tools/test_egg.py
"""

import http.server
import io
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import zipfile
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import egg_build  # noqa: E402

EGG = json.loads(egg_build.EGG.read_text(encoding="utf-8"))
VARIABLES = {variable["env_variable"]: variable for variable in EGG["variables"]}
INSTALLER_IMAGE = "opence-egg-test:bookworm"


# ---------- the egg's shape


def test_egg_is_what_egg_build_writes():
    built = egg_build.build(EGG, egg_build.INSTALL.read_text(encoding="utf-8"))
    assert egg_build.egg_text(built) == egg_build.EGG.read_text(encoding="utf-8"), \
        "run python tools/egg_build.py"


def test_egg_passes_the_checks():
    assert egg_build.check(EGG) == []


def test_egg_runs_the_server_and_waits_for_its_ready_line():
    assert EGG["meta"]["version"] == "PTDL_v2"
    assert EGG["config"]["stop"] == "^C"
    # the ready line in the text log's form and in the JSON log's
    # (server.log_format = "json": {"time":...,"cat":"server","msg":"ready (...)"})
    assert json.loads(EGG["config"]["startup"]) == {"done": ["server: ready", '"msg":"ready (']}
    text_line = "2026-10-25T12:00:00Z INFO server: ready (tunnel 2302/udp, web 127.0.0.1:8080, listing on)"
    json_line = json.dumps({"time": "2026-10-25T12:00:00Z", "level": "info", "cat": "server",
                            "msg": "ready (tunnel 2302/udp, web 127.0.0.1:8080, listing on)"}, separators=(",", ":"))
    for line in (text_line, json_line):
        assert any(done in line for done in json.loads(EGG["config"]["startup"])["done"]), line
    assert json.loads(EGG["config"]["files"]) == {}
    assert "HALO_NET_TUNNEL_PORT={{SERVER_PORT}}" in EGG["startup"]
    assert re.search(r"(^|\s)\./halo --server$", EGG["startup"])
    assert EGG["scripts"]["installation"]["entrypoint"] == "bash"
    assert EGG["scripts"]["installation"]["container"].startswith("ghcr.io/")
    assert all(image.endswith("/games:source") for image in EGG["docker_images"].values())


def test_variables_are_the_servers_settings():
    # every HALO_ variable is one the settings know (port/linux/src/port_config.c)
    names = set(re.findall(r'"(HALO_[A-Z0-9_]+)"', (ROOT / "port/linux/src/port_config.c").read_text(encoding="utf-8")))
    names |= set(re.findall(r"\b(HALO_[A-Z0-9_]+)\b", (ROOT / "port/linux/src/port_config.c").read_text(encoding="utf-8")))
    for env in VARIABLES:
        if env.startswith("HALO_"):
            assert env in names, f"{env} is no setting's environment variable"
    for env in ("HALO_SERVER_NAME", "HALO_SERVER_ROTATION", "HALO_SERVER_MAX_PLAYERS", "HALO_SERVER_PASSWORD",
                "HALO_SERVER_PUBLIC", "HALO_SERVER_WEB_PORT", "HALO_SERVER_WEB_PASSWORD", "HALO_SERVER_PUBLISHER_KEY",
                "OPENCE_RELEASE", "OPENCE_REPOSITORY"):
        assert env in VARIABLES
    # (the server's name fits the game's 15 characters)
    assert len(VARIABLES["HALO_SERVER_NAME"]["default_value"]) <= 15
    # the web port is off unless the owner gives a second allocation
    assert VARIABLES["HALO_SERVER_WEB_PORT"]["default_value"] == "0"
    assert VARIABLES["HALO_SERVER_WEB_BIND"]["user_editable"] is False


@pytest.mark.parametrize("rules", [
    "required|string|max:15",
    "nullable|string|regex:/^[0-9a-f]{64}$/",
    "required|integer|between:2,127",
    "required|string|in:true,false",
    "nullable|string|alpha_dash|max:16",
])
def test_rules_that_parse(rules):
    egg_build.parse_rules(rules)


@pytest.mark.parametrize("rules", [
    "",
    "required|strin",
    "required|max",
    "required|max:ten",
    "required|between:5",
    "required|between:9,2",
    "required|regex:^abc$",
    "required|regex:/(/",
    "required|nullable|string",
    "string|string",
    "required|string:5",
])
def test_rules_that_do_not(rules):
    with pytest.raises(Exception):
        egg_build.parse_rules(rules)


@pytest.mark.parametrize("value, rules, passes", [
    ("16", "required|integer|between:2,127", True),
    ("128", "required|integer|between:2,127", False),
    ("x", "required|integer|between:2,127", False),
    ("", "required|string|max:15", False),
    ("", "nullable|string|min:8|max:64", True),
    ("short", "nullable|string|min:8|max:64", False),
    ("OpenCE Dedicated", "required|string|max:15", False),
    ("yes", "required|string|in:true,false", False),
    ("na-east", "nullable|string|alpha_dash|max:16", True),
    ("na east", "nullable|string|alpha_dash|max:16", False),
    ("ab" * 32, "nullable|string|regex:/^[0-9a-f]{64}$/", True),
    ("AB" * 32, "nullable|string|regex:/^[0-9a-f]{64}$/", False),
])
def test_values_against_rules(value, rules, passes):
    assert (egg_build.value_passes(value, egg_build.parse_rules(rules)) == "") == passes


def broken(change):
    egg = json.loads(json.dumps(EGG))
    change(egg)
    return egg_build.check(egg)


def test_check_finds_what_the_panel_or_wings_refuse():
    assert broken(lambda egg: egg["meta"].update(version="PTDL_v1"))
    assert broken(lambda egg: egg.update(author="nobody"))
    assert broken(lambda egg: egg.update(startup="./halo --server"))
    assert broken(lambda egg: egg.update(startup="HALO_NET_TUNNEL_PORT={{server_port}} ./halo --server {{SERVER_PORT}}"))
    assert broken(lambda egg: egg.update(startup="./halo --server {{SERVER_PORT}} {{NOT_A_VARIABLE}}"))
    assert broken(lambda egg: egg["config"].update(startup="done: server: ready"))
    assert broken(lambda egg: egg["config"].update(startup='{"done": []}'))
    assert broken(lambda egg: egg["config"].update(startup='{"done": ["server: ready", ""]}'))
    assert broken(lambda egg: egg["variables"][0].update(env_variable="HOME"))
    assert broken(lambda egg: egg["variables"][0].update(env_variable="lower_case"))
    assert broken(lambda egg: egg["variables"].append(dict(egg["variables"][0])))
    assert broken(lambda egg: egg["variables"][0].update(rules="required|wat"))
    assert broken(lambda egg: egg["variables"][0].update(user_viewable=False))
    assert broken(lambda egg: egg["scripts"]["installation"].update(script="echo hi"))
    assert broken(lambda egg: egg["docker_images"].clear())


# ---------- the installation script in a container


def docker_works():
    """Why Docker cannot run the test, or "" if it can (without sudo, not
    rootless: the container reaches this machine's loopback)."""
    if not shutil.which("docker"):
        return "no docker"
    try:
        info = subprocess.run(["docker", "info", "-f", "{{json .SecurityOptions}}"], capture_output=True,
                              text=True, timeout=30)
    except (OSError, subprocess.TimeoutExpired):
        return "docker info did not answer"
    if info.returncode != 0:
        return "docker info failed (it may need root): " + info.stderr.strip()[:200]
    if "rootless" in info.stdout:
        return "rootless Docker: --network host does not reach this machine's loopback"
    return ""


def fake_release(build):
    """A halo-linux-release.zip of build: a halo that prints its version,
    and the other files a release has."""
    data = io.BytesIO()
    with zipfile.ZipFile(data, "w") as archive:
        def add(name, text, mode=0o644):
            info = zipfile.ZipInfo(name)
            info.external_attr = (0o100000 | mode) << 16
            archive.writestr(info, text)
        add("halo", "#!/bin/sh\n"
                    f"case \"$1\" in --version) echo 'OpenCE build-{build} (network version 38)'; exit 0;; esac\n"
                    "exit 1\n", 0o644)
        add("halo-server", '#!/bin/sh\nexec "$(dirname "$0")/halo" --server "$@"\n', 0o755)
        add("libSDL3.so.0", "not a library")
        add("brokers.txt", "tcp://broker.example.org:1883\n")
        add("masters.txt", "# none\n")
        add("LICENSE.txt", "AGPL\n")
        add("init.txt.example", f"# build {build}\n")
        add("gametypes/btb_slayer.toml", f'base = "team_slayer"\nname = "BTB {build}"\n')
        add("gametypes/king_moving.toml", 'base = "king"\nname = "Moving Hill"\n')
        add("deploy/halo-server", '#!/bin/sh\nexec "$(dirname "$0")/halo" --server "$@"\n', 0o755)
    return data.getvalue()


class Handler(http.server.BaseHTTPRequestHandler):
    """GitHub, as the installation script asks it: releases/latest/download,
    the API's releases/tags/<tag>, and releases/download/<tag>."""

    files = {}

    def do_GET(self):  # noqa: N802
        body = self.files.get(self.path)
        if body is None:
            self.send_response(404)
            self.end_headers()
            return
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass


@pytest.fixture(scope="module")
def github():
    why = docker_works()
    if why:
        pytest.skip(why)
    build = subprocess.run(["docker", "build", "-q", "-t", INSTALLER_IMAGE, "-"], input=(
        "FROM debian:bookworm-slim\n"
        "RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl jq unzip "
        "&& rm -rf /var/lib/apt/lists/*\n"), capture_output=True, text=True, timeout=900)
    if build.returncode != 0:
        pytest.skip("cannot build the installer's image: " + build.stderr.strip()[-300:])
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    port = server.server_address[1]
    base = f"http://127.0.0.1:{port}"
    Handler.files = {
        "/owner/opence/releases/latest/download/halo-linux-release.zip": fake_release(300),
        "/owner/opence/releases/download/build-299/halo-linux-release.zip": fake_release(299),
        "/api/repos/owner/opence/releases/tags/build-299": json.dumps({"tag_name": "build-299", "assets": [
            {"name": "halo-windows-release.zip", "browser_download_url": f"{base}/nothing.zip"},
            {"name": "halo-linux-release.zip",
             "browser_download_url": f"{base}/owner/opence/releases/download/build-299/halo-linux-release.zip"},
        ]}).encode(),
        "/api/repos/owner/opence/releases/tags/build-298": json.dumps({"tag_name": "build-298", "assets": []}).encode(),
    }
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    yield base
    server.shutdown()


def install(github, folder, release):
    """Runs install.sh as the panel does, into folder; its output."""
    script = EGG["scripts"]["installation"]["script"]
    result = subprocess.run([
        "docker", "run", "--rm", "-i", "--network", "host", "--user", f"{os.getuid()}:{os.getgid()}",
        "-v", f"{folder}:/mnt/server",
        "-e", f"OPENCE_RELEASE={release}", "-e", "OPENCE_REPOSITORY=owner/opence",
        "-e", f"OPENCE_GITHUB_URL={github}", "-e", f"OPENCE_GITHUB_API={github}/api",
        INSTALLER_IMAGE, "bash", "-s"], input=script, capture_output=True, text=True, timeout=300)
    return result.stdout + result.stderr


def test_install_latest_then_reinstall_a_tag(github, tmp_path):
    server = tmp_path / "server"
    server.mkdir()

    output = install(github, server, "latest")
    assert "Installation completed" in output, output
    assert "OpenCE build-300 (network version 38)" in output, output
    assert "upload your maps/ folder" in output
    for name in ("halo", "halo-server", "libSDL3.so.0", "brokers.txt", "masters.txt", "init.txt.example",
                 "gametypes/btb_slayer.toml", "deploy/halo-server"):
        assert (server / name).is_file(), name
    for name in ("maps", "custom_maps", "gametypes", "logs"):
        assert (server / name).is_dir(), name
    assert os.access(server / "halo", os.X_OK) and os.access(server / "halo-server", os.X_OK)
    assert (server / "halo-server").read_text() == '#!/bin/sh\nexec "$(dirname "$0")/halo" --server "$@"\n'

    # the owner's files, which a reinstall keeps
    (server / "config.toml").write_text("[server]\nname = \"Mine\"\n")
    (server / "init.txt").write_text("sv_motd hello\n")
    (server / "publisher.key").write_text("00" * 32 + "\n")
    (server / "gametypes/btb_slayer.toml").write_text('base = "team_slayer"\nname = "My BTB"\n')
    (server / "gametypes/mine.toml").write_text('base = "slayer"\nname = "Mine"\n')
    (server / "gametypes/king_moving.toml").unlink()
    (server / "custom_maps/arena.map").write_text("a map")
    (server / "maps/ui.map").write_text("the game's")
    (server / "logs/debug.txt").write_text("a log")

    output = install(github, server, "build-299")
    assert "Installation completed" in output, output
    assert "OpenCE build-299 (network version 38)" in output, output
    assert "upload your maps/ folder" not in output
    assert (server / "config.toml").read_text() == "[server]\nname = \"Mine\"\n"
    assert (server / "init.txt").read_text() == "sv_motd hello\n"
    assert (server / "publisher.key").read_text() == "00" * 32 + "\n"
    assert (server / "gametypes/btb_slayer.toml").read_text() == 'base = "team_slayer"\nname = "My BTB"\n'
    assert (server / "gametypes/mine.toml").is_file()
    assert (server / "gametypes/king_moving.toml").is_file()
    assert (server / "custom_maps/arena.map").read_text() == "a map"
    assert (server / "maps/ui.map").read_text() == "the game's"
    assert (server / "logs/debug.txt").read_text() == "a log"
    assert (server / "init.txt.example").read_text() == "# build 299\n"


@pytest.mark.parametrize("release, says", [
    ("build-1", "no release build-1"),
    ("build-298", "has no halo-linux-release.zip"),
    ("../etc", "is not a tag name"),
])
def test_install_says_why_it_failed(github, tmp_path, release, says):
    output = install(github, tmp_path, release)
    assert "INSTALL FAILED" in output and says in output, output
    assert "Installation completed" not in output
    assert not (tmp_path / "halo").exists()
